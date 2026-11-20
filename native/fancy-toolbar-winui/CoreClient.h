#pragma once
#include "pch.h"
#include <functional>
#include <map>

namespace FancyToolbar
{
    // Talks to the Rust core: events through the existing `/events` websocket, initial state
    // through `POST /native/invoke`. The token arrives on stdin when the core spawns us.
    class CoreClient
    {
    public:
        using Handler = std::function<void(winrt::Windows::Data::Json::IJsonValue const&)>;

        // nullptr when stdin carries no token (started by hand, no core to talk to)
        static std::unique_ptr<CoreClient> FromStdin();

        explicit CoreClient(std::wstring token);

        // `subscriptions` maps an event name to its handler; `initial` maps a command to the
        // handler of the event that carries the same payload. Handlers run on the UI thread.
        winrt::fire_and_forget Start(std::map<std::wstring, Handler> subscriptions,
                                     std::map<std::wstring, std::wstring> initial);

    private:
        std::wstring token_;
        std::map<std::wstring, Handler> handlers_;
        winrt::Microsoft::UI::Dispatching::DispatcherQueue ui_{ nullptr };
        winrt::Windows::Networking::Sockets::MessageWebSocket socket_{ nullptr };
        winrt::Windows::Web::Http::HttpClient http_{ nullptr };

        void Dispatch(std::wstring const& event, winrt::Windows::Data::Json::IJsonValue const& payload);
        winrt::Windows::Foundation::IAsyncAction Send(winrt::hstring const& text);
        winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Data::Json::IJsonValue> Invoke(winrt::hstring command);
    };
}
