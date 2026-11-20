#pragma once
// Shared by every native (WinUI 3) widget: add CoreClient.h/.cpp to the widget's project with
// PrecompiledHeader=NotUsing (see fancy-toolbar-winui/FancyToolbar.vcxproj). Self-contained on purpose,
// it does not rely on the widget's pch.
#include <windows.h>
#undef GetCurrentTime

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Networking.Sockets.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Microsoft.UI.Dispatching.h>

namespace Seelen
{
    // Talks to the Rust core: commands through `POST /native/invoke`, events through the `/events`
    // websocket. The core spawns the widget and writes its token on stdin.
    //
    // When the websocket drops, it reconnects with backoff, subscribes again and refetches the
    // initial state, so no change lost while disconnected stays on screen.
    //
    // Lives as long as the process: the reconnect loop keeps a pointer to it.
    class CoreClient
    {
    public:
        using Handler = std::function<void(winrt::Windows::Data::Json::IJsonValue const&)>;

        // nullptr when stdin carries no token (started by hand, no core to talk to)
        static std::unique_ptr<CoreClient> FromStdin();

        explicit CoreClient(std::wstring token);
        CoreClient(CoreClient const&) = delete;
        CoreClient& operator=(CoreClient const&) = delete;

        // `subscriptions` maps an event name to its handler; `initial` maps a command to the event
        // whose handler receives its result (same payload as the event). Handlers run on the thread
        // that calls Start, which must have a DispatcherQueue (the UI thread). Call it once.
        void Start(std::map<std::wstring, Handler> subscriptions, std::map<std::wstring, std::wstring> initial);

        // Runs a command of the core the widget is allowed to (allowlist in src/background/widgets/native).
        // `args` are named in camelCase, like `invoke` on the webviews. Throws hresult_error on failure.
        winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Data::Json::IJsonValue> Invoke(
            winrt::hstring command,
            winrt::Windows::Data::Json::JsonObject args = winrt::Windows::Data::Json::JsonObject());

    private:
        std::wstring token_;
        std::map<std::wstring, Handler> handlers_;
        std::map<std::wstring, std::wstring> initial_;
        winrt::Microsoft::UI::Dispatching::DispatcherQueue ui_{ nullptr };
        winrt::Windows::Web::Http::HttpClient http_;
        // only touched by the Run loop
        winrt::Windows::Networking::Sockets::MessageWebSocket socket_{ nullptr };
        // bumped on each connection, so events of an old socket are ignored
        std::atomic<unsigned> generation_{ 0 };
        // signaled when the current websocket is gone
        winrt::handle lost_;

        winrt::fire_and_forget Run();
        winrt::Windows::Foundation::IAsyncAction Connect();
        void Lost(unsigned generation);
        void Dispatch(std::wstring const& event, winrt::Windows::Data::Json::IJsonValue const& payload);
    };

    // Next reconnect delay: doubles up to a cap.
    std::chrono::milliseconds NextBackoff(std::chrono::milliseconds current);
}
