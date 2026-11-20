# Samples the memory of the Seelen UI process tree (seelen-ui.exe and every descendant: WebView2
# browser/renderer/GPU processes, the native toolbar) and appends one CSV row per sample.
# slu-service is started by the task scheduler, not by seelen-ui.exe, so it is reported in its own columns.
#
#   ./measure-ram.ps1 -Config A -Csv ram.csv            # sample
#   ./measure-ram.ps1 -Summarize -Csv ram.csv           # medians per config, markdown on stdout
param(
    [string]$Config,
    [Parameter(Mandatory)] [string]$Csv,
    [int]$Samples = 10,
    [int]$IntervalSeconds = 5,
    [switch]$Summarize
)
$ErrorActionPreference = 'Stop'

function Get-Median([double[]]$values) {
    $sorted = $values | Sort-Object
    $n = $sorted.Count
    if ($n % 2) { return $sorted[($n - 1) / 2] }
    return ($sorted[$n / 2 - 1] + $sorted[$n / 2]) / 2
}

if ($Summarize) {
    $rows = Import-Csv $Csv
    $byConfig = @{}
    "| config | samples | Working Set (MB) | Private Bytes (MB) | WS Private (MB) | processes | slu-service WS / PB (MB) |"
    "|---|---|---|---|---|---|---|"
    foreach ($group in $rows | Group-Object config) {
        $m = @{}
        foreach ($col in 'ws_mb', 'private_mb', 'ws_private_mb', 'procs', 'service_ws_mb', 'service_private_mb') {
            $m[$col] = [math]::Round((Get-Median ($group.Group | ForEach-Object { [double]$_.$col })), 1)
        }
        $byConfig[$group.Name] = $m
        $min = [math]::Round(($group.Group | Measure-Object ws_mb -Minimum).Minimum, 1)
        $max = [math]::Round(($group.Group | Measure-Object ws_mb -Maximum).Maximum, 1)
        "| $($group.Name) | $($group.Count) | $($m.ws_mb) ($min-$max) | $($m.private_mb) | $($m.ws_private_mb) | $($m.procs) | $($m.service_ws_mb) / $($m.service_private_mb) |"
    }
    # the toolbar's own cost is the difference against the run with every widget disabled
    if ($byConfig.B) {
        ""
        "| toolbar cost vs B | Working Set (MB) | Private Bytes (MB) | WS Private (MB) |"
        "|---|---|---|---|"
        foreach ($name in 'A', 'W') {
            if (-not $byConfig[$name]) { continue }
            $d = @{}
            foreach ($col in 'ws_mb', 'private_mb', 'ws_private_mb') {
                $d[$col] = [math]::Round($byConfig[$name][$col] - $byConfig.B[$col], 1)
            }
            "| $name - B | $($d.ws_mb) | $($d.private_mb) | $($d.ws_private_mb) |"
        }
    }
    return
}

if (-not $Config) { throw '-Config is required when sampling' }

for ($i = 1; $i -le $Samples; $i++) {
    $all = Get-CimInstance Win32_Process | Select-Object ProcessId, ParentProcessId, Name
    $roots = @($all | Where-Object Name -eq 'seelen-ui.exe' | ForEach-Object ProcessId)
    if (-not $roots) { throw "seelen-ui.exe is not running (config $Config)" }

    # walk down from seelen-ui.exe; a pid reused by an unrelated process cannot point back into the tree
    $tree = [System.Collections.Generic.HashSet[uint32]]::new()
    $queue = [System.Collections.Generic.Queue[uint32]]::new()
    foreach ($r in $roots) { [void]$tree.Add($r); $queue.Enqueue($r) }
    while ($queue.Count) {
        $parent = $queue.Dequeue()
        foreach ($child in $all | Where-Object { $_.ParentProcessId -eq $parent -and $_.ProcessId -ne $parent }) {
            if ($tree.Add($child.ProcessId)) { $queue.Enqueue($child.ProcessId) }
        }
    }

    # one query gives Working Set, Private Bytes and Working Set - Private for every process
    $perf = Get-CimInstance Win32_PerfRawData_PerfProc_Process | Where-Object { $_.IDProcess -ne 0 }
    $inTree = @($perf | Where-Object { $tree.Contains([uint32]$_.IDProcess) })
    $service = @($perf | Where-Object { $_.Name -like 'slu-service*' })

    $mb = 1MB
    [pscustomobject]@{
        config             = $Config
        sample             = $i
        time               = (Get-Date).ToString('o')
        procs              = $inTree.Count
        ws_mb              = [math]::Round(($inTree | Measure-Object WorkingSet -Sum).Sum / $mb, 2)
        private_mb         = [math]::Round(($inTree | Measure-Object PrivateBytes -Sum).Sum / $mb, 2)
        ws_private_mb      = [math]::Round(($inTree | Measure-Object WorkingSetPrivate -Sum).Sum / $mb, 2)
        service_ws_mb      = [math]::Round(($service | Measure-Object WorkingSet -Sum).Sum / $mb, 2)
        service_private_mb = [math]::Round(($service | Measure-Object PrivateBytes -Sum).Sum / $mb, 2)
    } | Export-Csv $Csv -Append -NoTypeInformation

    if ($i -eq $Samples) {
        # per process breakdown of the last sample, to see where the memory goes
        Write-Host "config $Config, last sample:"
        $inTree | Sort-Object WorkingSet -Descending |
            Format-Table Name, IDProcess,
                @{ n = 'WS MB'; e = { [math]::Round($_.WorkingSet / $mb, 1) } },
                @{ n = 'PB MB'; e = { [math]::Round($_.PrivateBytes / $mb, 1) } },
                @{ n = 'WSP MB'; e = { [math]::Round($_.WorkingSetPrivate / $mb, 1) } } |
            Out-String | Write-Host
    }
    else {
        Start-Sleep -Seconds $IntervalSeconds
    }
}
