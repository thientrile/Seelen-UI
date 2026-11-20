#include "CoreClient.h"

#include <algorithm>
#include <cctype>

#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Web.Http.Headers.h>

using namespace winrt;
using namespace Windows::Data::Json;
using namespace Windows::Foundation;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace Windows::Web::Http;
using namespace std::chrono_literals;

namespace
{
    // mirrors LOCAL_API_PORT in src/background/server/http/mod.rs
    constexpr wchar_t kBaseUrl[] = L"127.0.0.1:37074";
    constexpr std::chrono::milliseconds kMinBackoff = 500ms;
    constexpr std::chrono::milliseconds kMaxBackoff = 30s;

    void Log(std::wstring const& message)
    {
        OutputDebugStringW((L"[seelen-core] " + message + L"\n").c_str());
    }

    IAsyncAction Send(MessageWebSocket const& socket, hstring const& text)
    {
        DataWriter writer(socket.OutputStream());
        writer.UnicodeEncoding(UnicodeEncoding::Utf8);
        writer.WriteString(text);
        co_await writer.StoreAsync();
        writer.DetachStream();
    }
}

namespace Seelen
{
    std::chrono::milliseconds NextBackoff(std::chrono::milliseconds current)
    {
        return (std::min)(current * 2, kMaxBackoff);
    }

    std::unique_ptr<CoreClient> CoreClient::FromStdin()
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
        return std::make_unique<CoreClient>(std::wstring(token.begin(), token.end()));
    }

    CoreClient::CoreClient(std::wstring token)
        : token_(std::move(token)), lost_(check_pointer(CreateEventW(nullptr, TRUE, FALSE, nullptr)))
    {
    }

    void CoreClient::Start(std::map<std::wstring, Handler> subscriptions, std::map<std::wstring, std::wstring> initial)
    {
        handlers_ = std::move(subscriptions);
        initial_ = std::move(initial);
        ui_ = Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
        Run();
    }

    fire_and_forget CoreClient::Run()
    {
        auto delay = kMinBackoff;
        while (true)
        {
            ResetEvent(lost_.get());
            bool connected = false;
            try
            {
                co_await Connect();
                connected = true;
            }
            catch (hresult_error const& err)
            {
                Log(L"connection failed: " + std::wstring(err.message()));
            }

            if (connected)
            {
                delay = kMinBackoff;
                co_await resume_on_signal(lost_.get());
                Log(L"events socket lost, reconnecting");
            }
            co_await resume_after(delay);
            delay = NextBackoff(delay);
        }
    }

    IAsyncAction CoreClient::Connect()
    {
        if (socket_)
        {
            try
            {
                socket_.Close();
            }
            catch (hresult_error const&)
            {
            }
        }

        auto generation = ++generation_;
        socket_ = MessageWebSocket();
        socket_.Control().MessageType(SocketMessageType::Utf8);
        socket_.MessageReceived([this, generation](MessageWebSocket const&, MessageWebSocketMessageReceivedEventArgs const& args) {
            DataReader reader{ nullptr };
            try
            {
                // throws when the connection broke (e.g. the core went away without closing)
                reader = args.GetDataReader();
            }
            catch (hresult_error const&)
            {
                Lost(generation);
                return;
            }
            try
            {
                reader.UnicodeEncoding(UnicodeEncoding::Utf8);
                auto frame = JsonObject::Parse(reader.ReadString(reader.UnconsumedBufferLength()));
                if (frame.GetNamedString(L"type", L"") == L"event")
                {
                    Dispatch(std::wstring(frame.GetNamedString(L"event")), frame.GetNamedValue(L"payload"));
                }
            }
            catch (hresult_error const& err)
            {
                Log(L"bad event frame: " + std::wstring(err.message()));
            }
        });
        socket_.Closed([this, generation](auto&&, auto&&) { Lost(generation); });

        // the websocket authenticates by query (browsers cannot set headers on it)
        co_await socket_.ConnectAsync(Uri(hstring(L"ws://") + kBaseUrl + L"/events?token=" + token_));
        // subscribe before fetching the state, so no change falls in between
        for (auto const& [event, _] : handlers_)
        {
            co_await Send(socket_, hstring(L"{\"type\":\"subscribe\",\"event\":\"") + event + L"\"}");
        }
        for (auto const& [command, event] : initial_)
        {
            Dispatch(event, co_await Invoke(hstring(command)));
        }
    }

    void CoreClient::Lost(unsigned generation)
    {
        if (generation == generation_)
        {
            SetEvent(lost_.get());
        }
    }

    void CoreClient::Dispatch(std::wstring const& event, IJsonValue const& payload)
    {
        auto it = handlers_.find(event);
        if (it == handlers_.end())
        {
            return;
        }
        ui_.TryEnqueue([handler = it->second, payload]() { handler(payload); });
    }

    IAsyncOperation<IJsonValue> CoreClient::Invoke(hstring command, JsonObject args)
    {
        JsonObject body;
        body.SetNamedValue(L"command", JsonValue::CreateStringValue(command));
        body.SetNamedValue(L"args", args);
        HttpStringContent content(body.Stringify(), UnicodeEncoding::Utf8, L"application/json");
        // the token goes in a header here: unlike the websocket, nothing forces it into the url
        HttpRequestMessage request(HttpMethod::Post(), Uri(hstring(L"http://") + kBaseUrl + L"/native/invoke"));
        request.Headers().Insert(L"X-Seelen-Token", token_);
        request.Content(content);
        auto response = co_await http_.SendRequestAsync(request);
        if (!response.IsSuccessStatusCode())
        {
            throw hresult_error(E_FAIL, hstring(L"core answered ") + to_hstring(static_cast<int32_t>(response.StatusCode())) + L" to " + command);
        }
        co_return JsonValue::Parse(co_await response.Content().ReadAsStringAsync());
    }
}
