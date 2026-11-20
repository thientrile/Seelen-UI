use seelen_core::state::UpdateChannel;
use slu_ipc::{IPC, ServiceIpc, messages::SvcAction};
use tauri_plugin_updater::{Update, UpdaterExt};

use crate::{
    app::get_app_handle,
    error::Result,
    platform::info::{has_fixed_runtime, is_running_as_appx},
    state::FULL_STATE,
};

/// Fork signing key (thientrile/Seelen-UI); also verifies SHA256SUMS.sig in the integrity check.
pub static SIGN_PUB_KEY: &str = "dW50cnVzdGVkIGNvbW1lbnQ6IG1pbmlzaWduIHB1YmxpYyBrZXk6IDQ0RTg3NDc1M0Q3NjY4NDkKUldSSmFIWTlkWFRvUkl3R1AzSFFONi9aVHQ3RW4zYzIwdGFXRWJnYmRQZ1k1RHQrQ1hRVmxaTnQK";

/// Updates come from the fork's GitHub Releases, not upstream.
const RELEASES_URL: &str = "https://github.com/thientrile/Seelen-UI/releases";

pub async fn check_for_updates() -> Result<Option<Update>> {
    if tauri::is_dev() || has_fixed_runtime() || is_running_as_appx() {
        return Ok(None);
    }

    let state = FULL_STATE.load();
    let channel = state.settings.updater.channel;
    let mut update = None;

    if channel == UpdateChannel::Nightly {
        let updater: tauri_plugin_updater::Updater = get_app_handle()
            .updater_builder()
            .pubkey(SIGN_PUB_KEY)
            .endpoints(vec![
                format!("{RELEASES_URL}/download/nightly/latest.json")
                    .parse()
                    .expect("Failed to parse url"),
            ])?
            .build()?;
        // the fork publishes no nightly builds yet: fall back to the release channel
        update = updater.check().await.unwrap_or_else(|err| {
            log::warn!("Nightly update check failed: {err}");
            None
        });
    }

    // Release Channel
    if update.is_none() {
        let updater: tauri_plugin_updater::Updater = get_app_handle()
            .updater_builder()
            .pubkey(SIGN_PUB_KEY)
            .endpoints(vec![
                format!("{RELEASES_URL}/latest/download/latest.json")
                    .parse()
                    .expect("Failed to parse url"),
            ])?
            .build()?;
        update = updater.check().await?;
    }

    Ok(update)
}

pub async fn trace_update_intallation(update: Update) -> Result<()> {
    log::trace!("Update: downloading");
    let bytes = update
        .download(
            |_chunk_length, _content_length| {},
            || log::trace!("Update: download finished"),
        )
        .await?;

    // Send Stop synchronously and wait for the service to actually exit before
    // handing off to the NSIS installer. Using fire-and-forget here means the
    // Tokio runtime can shut down before the IPC message is delivered, leaving
    // the service's app-monitoring loop alive to fight the installer.
    if let Err(err) = ServiceIpc::send(SvcAction::Stop).await {
        log::warn!("Failed to send stop to service before update: {err}");
    } else {
        // Poll until the service IPC is no longer reachable (process exited)
        // or we time out after ~2 s and let the NSIS PREINSTALL handle it.
        let mut waited_ms = 0u32;
        while ServiceIpc::can_stablish_connection() && waited_ms < 2000 {
            tokio::time::sleep(std::time::Duration::from_millis(100)).await;
            waited_ms += 100;
        }
    }

    update.install(bytes)?;
    log::trace!("Update: intallation finished");
    Ok(())
}
