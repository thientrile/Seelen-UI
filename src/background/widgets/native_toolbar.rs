//! Prototype: launches the WinUI 3 toolbar (`native/fancy-toolbar-winui`) as a separate process.

use std::{
    io::Write,
    process::{Command, Stdio},
};

use crate::{error::Result, server::events::tokens};

pub const NATIVE_TOOLBAR_LABEL: &str = "@seelen/fancy-toolbar-native";

/// shortcut: opt-in by env var for the RAM comparison, turn it into a widget setting if the prototype is accepted
pub fn is_enabled() -> bool {
    std::env::var_os("SLU_NATIVE_TOOLBAR").is_some()
}

pub fn spawn() -> Result<()> {
    let exe = std::env::current_exe()?
        .with_file_name("native-toolbar")
        .join("FancyToolbar.exe");
    let token = tokens::get_or_create(NATIVE_TOOLBAR_LABEL);

    let mut child = Command::new(exe).stdin(Stdio::piped()).spawn()?;
    // the token travels on stdin: the command line and environment are readable by other processes
    let mut stdin = child.stdin.take().ok_or("native toolbar has no stdin")?;
    stdin.write_all(token.to_string().as_bytes())?;
    // shortcut: the child is not tied to the core's lifetime, use a job object before shipping
    Ok(())
}
