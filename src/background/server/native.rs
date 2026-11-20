//! Endpoint of the native (non-webview) widgets, see `widgets::native`.

use salvo::prelude::*;
use uuid::Uuid;

use crate::{server::events::tokens, widgets::native::NativeWidget};

#[derive(serde::Deserialize)]
struct InvokeRequest {
    command: String,
    /// Arguments by name, camelCase like `invoke` on the webviews.
    #[serde(default)]
    args: serde_json::Map<String, serde_json::Value>,
}

/// `POST /native/invoke` with `X-Seelen-Token` and `{"command": "<name>", "args": {...}}`.
///
/// Runs any command of `slu_commands_declaration!` the widget owning the token is allowed to.
/// Answers its json result, or 400 (bad body), 401 (bad token), 403 (not allowed, or an Origin),
/// 404 (unknown command), 500 (the command failed, the reason in the body).
#[handler]
pub async fn native_invoke(req: &mut Request, res: &mut Response) -> Result<(), StatusError> {
    // web pages can reach localhost too and always send an Origin on a cross-site POST, native clients never do
    if req.headers().contains_key("origin") {
        return Err(StatusError::forbidden());
    }

    let widget = req
        .header::<String>("x-seelen-token")
        .and_then(|token| Uuid::parse_str(&token).ok())
        .and_then(|token| tokens::resolve(&token))
        .and_then(|label| NativeWidget::by_label(&label))
        .ok_or_else(StatusError::unauthorized)?;

    let body: InvokeRequest = req
        .parse_json()
        .await
        .map_err(|_| StatusError::bad_request())?;

    if !widget.can_invoke(&body.command) {
        return Err(StatusError::forbidden());
    }

    let args = serde_json::Value::Object(body.args);
    match crate::tauri_handlers::native_dispatch(&body.command, args).await {
        Some(Ok(value)) => {
            res.render(Json(value));
            Ok(())
        }
        Some(Err(err)) => Err(StatusError::internal_server_error().brief(err.to_string())),
        None => Err(StatusError::not_found()),
    }
}
