//! Prototype endpoint for native (non-webview) widgets, see `native/fancy-toolbar-winui`.

use salvo::prelude::*;
use uuid::Uuid;

use crate::{server::events::tokens, widgets::native_toolbar::NATIVE_TOOLBAR_LABEL};

#[derive(serde::Deserialize)]
struct InvokeRequest {
    command: String,
}

/// `POST /native/invoke` with `X-Seelen-Token` and `{"command": "<name>"}`.
#[handler]
pub async fn native_invoke(req: &mut Request, res: &mut Response) -> Result<(), StatusError> {
    // web pages can reach localhost too and always send an Origin on a cross-site POST, native clients never do
    if req.headers().contains_key("origin") {
        return Err(StatusError::forbidden());
    }

    let authorized = req
        .header::<String>("x-seelen-token")
        .and_then(|token| Uuid::parse_str(&token).ok())
        .and_then(|token| tokens::resolve(&token))
        .is_some_and(|label| label == NATIVE_TOOLBAR_LABEL);
    if !authorized {
        return Err(StatusError::unauthorized());
    }

    let body: InvokeRequest = req
        .parse_json()
        .await
        .map_err(|_| StatusError::bad_request())?;

    // shortcut: allowlist of what the prototype toolbar needs, replace with the macro generated dispatch
    let value = match body.command.as_str() {
        "get_focused_app" => {
            serde_json::to_value(crate::tauri_handlers::Handlers::get_focused_app())
        }
        "get_virtual_desktops" => {
            serde_json::to_value(crate::tauri_handlers::Handlers::get_virtual_desktops())
        }
        _ => return Err(StatusError::not_found()),
    }
    .map_err(|_| StatusError::internal_server_error())?;

    res.render(Json(value));
    Ok(())
}
