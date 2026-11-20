//! Native (non-webview) widgets, see `native/`: separate processes that talk to the core through
//! `POST /native/invoke` (`server::native`) and the `/events` websocket.

use std::{
    io::Write,
    process::{Child, Command, Stdio},
    time::{Duration, Instant},
};

use crate::{
    error::{Result, ResultLogExt},
    platform::process::Process,
    server::events::tokens,
};

pub const NATIVE_WIDGETS: &[NativeWidget] = &[NativeWidget {
    label: "@seelen/fancy-toolbar-native",
    exe: r"native-toolbar\FancyToolbar.exe",
    enable_env: "SLU_NATIVE_TOOLBAR",
    commands: &["get_focused_app", "get_virtual_desktops"],
}];

/// A crash this soon after the start counts as a failed start.
const MIN_HEALTHY_UPTIME: Duration = Duration::from_secs(10);
/// Failed starts in a row before the widget is left stopped.
const MAX_FAILED_STARTS: u32 = 5;

pub struct NativeWidget {
    /// Identifies the widget on the events websocket and `/native/invoke`, through its token.
    pub label: &'static str,
    /// Relative to the folder of the core executable.
    pub exe: &'static str,
    /// shortcut: opt-in by env var while native widgets are a prototype, make it a widget setting once accepted
    pub enable_env: &'static str,
    /// Commands of `slu_commands_declaration!` the widget may run through `/native/invoke`.
    pub commands: &'static [&'static str],
}

impl NativeWidget {
    pub fn by_label(label: &str) -> Option<&'static NativeWidget> {
        NATIVE_WIDGETS.iter().find(|widget| widget.label == label)
    }

    pub fn can_invoke(&self, command: &str) -> bool {
        self.commands.contains(&command)
    }

    /// Starts every enabled native widget, each one restarted when it crashes.
    pub fn start_enabled() {
        for widget in NATIVE_WIDGETS
            .iter()
            .filter(|widget| std::env::var_os(widget.enable_env).is_some())
        {
            std::thread::Builder::new()
                .name(format!("native widget {}", widget.label))
                .spawn(move || widget.supervise())
                .log_error();
        }
    }

    fn supervise(&self) {
        let mut failed_starts = 0;
        loop {
            let started = Instant::now();
            let clean_exit = match self.spawn().and_then(|mut child| Ok(child.wait()?)) {
                Ok(status) => {
                    log::info!("Native widget {} exited with {status}", self.label);
                    status.success()
                }
                Err(err) => {
                    log::error!("Native widget {} failed: {err}", self.label);
                    false
                }
            };

            let Some(delay) = restart_delay(clean_exit, started.elapsed(), &mut failed_starts)
            else {
                log::warn!("Native widget {} left stopped", self.label);
                return;
            };
            std::thread::sleep(delay);
        }
    }

    fn spawn(&self) -> Result<Child> {
        let exe = std::env::current_exe()?.with_file_name(self.exe);
        let mut child = Command::new(exe).stdin(Stdio::piped()).spawn()?;

        // shortcut: the child runs untied for the instant before this, spawn it suspended if that ever matters
        if let Err(err) = Process::from_id(child.id()).kill_on_current_exit() {
            // never leave a widget that would outlive the core
            child.kill().log_error();
            return Err(err);
        }

        // the token travels on stdin: the command line and environment are readable by other processes
        let token = tokens::get_or_create(self.label);
        let mut stdin = child.stdin.take().ok_or("native widget has no stdin")?;
        stdin.write_all(token.to_string().as_bytes())?;
        // dropping stdin closes the pipe, the widget reads the token up to EOF
        Ok(child)
    }
}

/// Delay before restarting a widget that exited, `None` to leave it stopped.
/// A clean exit means the widget closed on purpose; failed starts in a row back off, then give up.
fn restart_delay(clean_exit: bool, uptime: Duration, failed_starts: &mut u32) -> Option<Duration> {
    if clean_exit {
        return None;
    }
    if uptime >= MIN_HEALTHY_UPTIME {
        *failed_starts = 0;
    }
    *failed_starts += 1;
    if *failed_starts > MAX_FAILED_STARTS {
        return None;
    }
    Some(Duration::from_secs(1 << (*failed_starts - 1)))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::tauri_handlers::ALL_COMMANDS;

    #[test]
    fn allowlists_name_declared_commands() {
        for widget in NATIVE_WIDGETS {
            for command in widget.commands {
                assert!(
                    ALL_COMMANDS.contains(command),
                    "{} allows unknown command {command}",
                    widget.label
                );
            }
        }
    }

    #[test]
    fn allowlist_is_per_widget() {
        let toolbar = NativeWidget::by_label("@seelen/fancy-toolbar-native").unwrap();
        assert!(toolbar.can_invoke("get_focused_app"));
        assert!(!toolbar.can_invoke("set_settings"));
        assert!(NativeWidget::by_label("@seelen/fancy-toolbar").is_none());
    }

    #[test]
    fn crashes_back_off_then_give_up() {
        let quick = Duration::from_secs(1);
        let mut failed = 0;
        let delays: Vec<_> = (0..6)
            .map(|_| restart_delay(false, quick, &mut failed))
            .collect();
        assert_eq!(
            delays,
            [1, 2, 4, 8, 16]
                .map(|s| Some(Duration::from_secs(s)))
                .into_iter()
                .chain([None])
                .collect::<Vec<_>>()
        );

        // a crash after running fine starts over
        assert_eq!(
            restart_delay(false, MIN_HEALTHY_UPTIME, &mut failed),
            Some(Duration::from_secs(1))
        );
        // closed on purpose
        assert_eq!(restart_delay(true, quick, &mut 0), None);
    }
}
