#include "pch.h"
#include "CoreClient.h"

using namespace winrt;
using namespace Windows::Data::Json;
using namespace Windows::Foundation;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace Windows::Web::Http;

namespace
{
    // mirrors LOCAL_API_PORT in src/background/server/http/mod.rs
    constexpr wchar_t kBaseUrl[] = L"127.0.0.1:37074";
}

namespace FancyToolbar
{
    std::shared_ptr<CoreClient> CoreClient::FromStdin()
    {
        HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
        if (in == nullptr || in == INVALID_HANDLE_VALUE || GetFileType(in) != FILE_TYPE_PIPE)
        {
            return nullptr;
        }

        // the core writes the token and closes the pipe, so this read ends at EOF
        std::string token;
        char buffer[128];
        DWORD read = 0;
        while (token.size() < 256 && ReadFile(in, buffer, sizeof(buffer), &read, nullptr) && read > 0)
        {
            token.append(buffer, read);
        }
        while (!token.empty() && isspace(static_cast<unsigned char>(token.back())))
        {
            token.pop_back();
        }
        // a uuid: anything else is not from the core
        if (token.size() != 36)
        {
            return nullptr;
        }
        return std::make_shared<CoreClient>(std::wstring(token.begin(), token.end()));
    }

    CoreClient::CoreClient(std::wstring token) : token_(std::move(token))
    {
    }

    CoreClient::~CoreClient()
    {
        Stop();
    }

    void CoreClient::Stop()
    {
        if (stopped_.exchange(true) || !socket_)
        {
            return;
        }
        socket_.MessageReceived(receivedToken_);
        socket_.Closed(closedToken_);
        try
        {
            socket_.Close();
        }
        catch (hresult_error const&)
        {
            // not connected yet: the pending ConnectAsync fails and Start ends
        }
    }

    fire_and_forget CoreClient::Start(std::map<std::wstring, Handler> subscriptions,
                                      std::map<std::wstring, std::wstring> initial)
    {
        // keeps the client alive across the awaits below
        auto self = shared_from_this();
        handlers_ = std::move(subscriptions);
        ui_ = Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
        http_ = HttpClient();
        socket_ = MessageWebSocket();
        socket_.Control().MessageType(SocketMessageType::Utf8);
        receivedToken_ = socket_.MessageReceived(
            [weak = weak_from_this()](MessageWebSocket const&, MessageWebSocketMessageReceivedEventArgs const& args) {
                auto client = weak.lock();
                if (!client || client->stopped_)
                {
                    return;
                }
                try
                {
                    auto reader = args.GetDataReader();
                    reader.UnicodeEncoding(UnicodeEncoding::Utf8);
                    auto frame = JsonObject::Parse(reader.ReadString(reader.UnconsumedBufferLength()));
                    if (frame.GetNamedString(L"type", L"") == L"event")
                    {
                        client->Dispatch(std::wstring(frame.GetNamedString(L"event")), frame.GetNamedValue(L"payload"));
                    }
                }
                catch (hresult_error const& err)
                {
                    OutputDebugStringW((L"[toolbar] bad event frame: " + err.message() + L"\n").c_str());
                }
            });
        // shortcut: no reconnect, the toolbar keeps the last state if the core goes away; add a backoff loop before shipping
        closedToken_ = socket_.Closed([](auto&&, auto&&) { OutputDebugStringW(L"[toolbar] events socket closed\n"); });

        try
        {
            // the existing websocket authenticates by query (browsers cannot set headers on it)
            co_await socket_.ConnectAsync(Uri(hstring(L"ws://") + kBaseUrl + L"/events?token=" + token_));
            // subscribe before fetching the initial state, so no change falls in between
            for (auto const& [event, _] : handlers_)
            {
                if (stopped_) co_return;
                co_await Send(hstring(L"{\"type\":\"subscribe\",\"event\":\"") + event + L"\"}");
            }
            for (auto const& [command, event] : initial)
            {
                if (stopped_) co_return;
                auto payload = co_await Invoke(hstring(command));
                Dispatch(event, payload);
            }
        }
        catch (hresult_error const& err)
        {
            OutputDebugStringW((L"[toolbar] core connection failed: " + err.message() + L"\n").c_str());
        }
    }

    void CoreClient::Dispatch(std::wstring const& event, IJsonValue const& payload)
    {
        auto it = handlers_.find(event);
        if (it == handlers_.end())
        {
            return;
        }
        // Stop runs on the UI thread too, so a handler never runs after it
        ui_.TryEnqueue([weak = weak_from_this(), handler = it->second, payload]() {
            auto client = weak.lock();
            if (client && !client->stopped_)
            {
                handler(payload);
            }
        });
    }

    IAsyncAction CoreClient::Send(hstring const& text)
    {
        DataWriter writer(socket_.OutputStream());
        writer.UnicodeEncoding(UnicodeEncoding::Utf8);
        writer.WriteString(text);
        co_await writer.StoreAsync();
        writer.DetachStream();
    }

    IAsyncOperation<IJsonValue> CoreClient::Invoke(hstring command)
    {
        JsonObject body;
        body.SetNamedValue(L"command", JsonValue::CreateStringValue(command));
        HttpStringContent content(body.Stringify(), UnicodeEncoding::Utf8, L"application/json");
        // the token goes in a header here: unlike the websocket, nothing forces it into the url
        HttpRequestMessage request(HttpMethod::Post(), Uri(hstring(L"http://") + kBaseUrl + L"/native/invoke"));
        request.Headers().Insert(L"X-Seelen-Token", token_);
        request.Content(content);
        auto response = co_await http_.SendRequestAsync(request);
        response.EnsureSuccessStatusCode();
        co_return JsonValue::Parse(co_await response.Content().ReadAsStringAsync());
    }
}
