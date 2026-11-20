# Native widgets (WinUI 3)

Widgets drawn with WinUI 3 (C++/WinRT) instead of a WebView2, each one a separate process spawned by the core.

## IPC

- Commands: `POST http://127.0.0.1:37074/native/invoke`, header `X-Seelen-Token`, body
  `{"command": "<fn name>", "args": {...camelCase}}`. Every command of `slu_commands_declaration!`
  (`libs/core/src/handlers/commands.rs`) is reachable, dispatched by the generated `native_dispatch`, except `@webview`
  ones. A widget may only run the commands of its allowlist.
- Events: the `/events` websocket the webviews use, `ws://127.0.0.1:37074/events?token=<token>`.
- The token reaches the widget on stdin, never on the command line or environment. Requests with an `Origin` header are
  refused (web pages can reach localhost too).
- `shared/CoreClient.{h,cpp}`: the client every widget uses (invoke + subscribe, reconnects with backoff and refetches
  the initial state). Owned by a `shared_ptr`; call `Stop()` on the UI thread before the window that owns the handlers
  closes.

## Lifecycle

The core puts every widget in a job object that kills it when the core exits, crashes included, and restarts a widget
that crashes (backoff 1, 2, 4, 8, 16 s, then left stopped). A widget exiting with code 0 is not restarted.

## Adding a widget

1. Add an entry to `NATIVE_WIDGETS` in `src/background/widgets/native/mod.rs`: label, exe path, enabling env var, and
   the commands it may run.
2. Add `shared/CoreClient.h/.cpp` to its `.vcxproj` like `fancy-toolbar-winui/FancyToolbar.vcxproj` does.
3. Build it in `.github/workflows/winui-toolbar.yml`.

## Not done yet

- Multi-monitor: the toolbar shows on the primary monitor only. Doing it right means one window per monitor in the same
  process (one process per monitor would multiply the RAM we are trying to save), plus a monitor id to pick the
  workspaces, a change of the toolbar itself rather than of the IPC.
- Events have no per-widget allowlist: any token holder may subscribe to any event, as the webviews can.
