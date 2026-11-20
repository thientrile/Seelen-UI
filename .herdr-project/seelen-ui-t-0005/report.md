PR: https://github.com/thientrile/Seelen-UI/pull/5

## Report

Implemented the native Weg prototype on top of the IPC branch:

- Added `native/weg-winui/`, an unpackaged self-contained WinUI 3 C++/WinRT app
  using the shared `native/shared/CoreClient.{h,cpp}`.
- Added native Weg commands for window activation, close, and launch, plus a
  minimal per-widget allowlist and `SLU_NATIVE_WEG=1` opt-in.
- Added Weg build, lifecycle, screenshots, and RAM configurations to the
  GitHub-hosted Windows workflow. RAM sampling now includes all
  `msedgewebview2.exe` processes, even when they are not descendants of
  `seelen-ui.exe`.
- Updated the generated command enum manually because `deno` is unavailable in
  this WSL environment; CI should regenerate/verify bindings.

Current Weg feature coverage:

| Feature | This PR |
|---|---|
| Pinned apps | Partial: reads and renders app/file items |
| Open windows | Partial: groups by UMID/path and refreshes on events |
| Groups | Partial: grouped activation/close behavior |
| Icons | Not yet: buttons currently show display names |
| Click | Ported: activate/minimize or launch |
| Middle-click | Not yet |
| Context menu | Partial: native Close menu only |
| Drag/drop reorder | Not yet |
| Edge position | Ported: primary-monitor appbar placement |
| Auto-hide | Not yet |

Weg JavaScript plugins are intentionally not ported; the owner decision is to
replace them with declarative JSON/YAML plugins in a later task. The native
process also leaves media, previews, full context-menu actions, and drag/drop
to the existing WebView implementation for now.

Validation is blocked locally: the configured Rust toolchain is missing
(`nightly-2026-05-22-x86_64-unknown-linux-gnu`), `deno` is not installed, and
Windows C++/WinUI compilation is only available in GitHub Actions.

## Next

Run the Windows CI build and native Weg RAM measurement
Review native Weg UI gaps before expanding feature coverage
Merge PR #5 after its base reaches master and checks pass

## Remember

Native WebView2 memory must include every `msedgewebview2.exe`, not only the
`seelen-ui.exe` process tree.
