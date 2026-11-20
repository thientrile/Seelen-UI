#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
#include <microsoft.ui.xaml.window.h>

using namespace winrt;
using namespace Microsoft::UI::Composition::SystemBackdrops;
using namespace Microsoft::UI::Windowing;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Data::Json;

namespace
{
    constexpr int kDefaultSize = 40;
    constexpr int kDefaultPadding = 8;
    constexpr int kDefaultMargin = 8;

    int Number(JsonObject const& object, wchar_t const* name, int fallback)
    {
        return object.HasKey(name) ? static_cast<int>(object.GetNamedNumber(name)) : fallback;
    }

    bool IsVertical(JsonObject const& settings)
    {
        if (!settings) return false;
        auto position = settings.GetNamedString(L"position", L"Bottom");
        return position == L"Left" || position == L"Right";
    }
}

namespace winrt::Weg::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
        Window window = *this;
        window.as<IWindowNative>()->get_WindowHandle(&hwnd_);
        SetupWindow();
        SetupBackdrop();
        RegisterAppbar();

        Closed([this](auto&&, auto&&) {
            UnregisterAppbar();
            if (mica_) mica_.Close();
            if (acrylic_) acrylic_.Close();
        });

        core_ = ::Seelen::CoreClient::FromStdin();
        if (core_)
        {
            core_->Start(
                {
                    { L"weg::items-changed", [this](IJsonValue const& value) { OnItems(value); } },
                    { L"user::windows-changed", [this](IJsonValue const& value) { OnWindows(value); } },
                    { L"global-focus-changed", [this](IJsonValue const& value) { OnFocused(value); } },
                    { L"native::settings", [this](IJsonValue const& value) { OnSettings(value); } },
                },
                {
                    { L"state_get_weg_items", L"weg::items-changed" },
                    { L"get_user_app_windows", L"user::windows-changed" },
                    { L"get_focused_app", L"global-focus-changed" },
                    { L"state_get_settings", L"native::settings" },
                });
        }
    }

    void MainWindow::SetupWindow()
    {
        auto appWindow = AppWindow();
        appWindow.IsShownInSwitchers(false);
        auto presenter = OverlappedPresenter::Create();
        presenter.SetBorderAndTitleBar(false, false);
        presenter.IsResizable(false);
        presenter.IsMaximizable(false);
        presenter.IsMinimizable(false);
        presenter.IsAlwaysOnTop(true);
        appWindow.SetPresenter(presenter);

        COLORREF none = DWMWA_COLOR_NONE;
        DwmSetWindowAttribute(hwnd_, DWMWA_BORDER_COLOR, &none, sizeof(none));
        DWM_WINDOW_CORNER_PREFERENCE corners = DWMWCP_DONOTROUND;
        DwmSetWindowAttribute(hwnd_, DWMWA_WINDOW_CORNER_PREFERENCE, &corners, sizeof(corners));
    }

    void MainWindow::SetupBackdrop()
    {
        backdropConfig_ = SystemBackdropConfiguration();
        backdropConfig_.IsInputActive(true);
        backdropConfig_.Theme(SystemBackdropTheme::Default);
        Window window = *this;
        auto target = window.try_as<Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop>();
        if (MicaController::IsSupported())
        {
            mica_ = MicaController();
            mica_.SetSystemBackdropConfiguration(backdropConfig_);
            mica_.AddSystemBackdropTarget(target);
        }
        else if (DesktopAcrylicController::IsSupported())
        {
            acrylic_ = DesktopAcrylicController();
            acrylic_.SetSystemBackdropConfiguration(backdropConfig_);
            acrylic_.AddSystemBackdropTarget(target);
        }
    }

    void MainWindow::RegisterAppbar()
    {
        appbarMessage_ = RegisterWindowMessageW(L"SeelenUI.Weg.Appbar");
        APPBARDATA abd{ sizeof(abd) };
        abd.hWnd = hwnd_;
        abd.uCallbackMessage = appbarMessage_;
        SHAppBarMessage(ABM_NEW, &abd);
        SetWindowSubclass(hwnd_, SubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
        PlaceAppbar();
    }

    void MainWindow::PlaceAppbar()
    {
        if (!settings_) return;
        MONITORINFO mi{ sizeof(mi) };
        GetMonitorInfoW(MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &mi);
        auto position = settings_.GetNamedString(L"position", L"Bottom");
        int size = Number(settings_, L"size", kDefaultSize);
        int padding = Number(settings_, L"padding", kDefaultPadding);
        int margin = Number(settings_, L"margin", kDefaultMargin);
        int pixels = MulDiv(size + padding * 2 + margin * 2, GetDpiForWindow(hwnd_), 96);

        APPBARDATA abd{ sizeof(abd) };
        abd.hWnd = hwnd_;
        abd.uEdge = position == L"Left" ? ABE_LEFT : position == L"Right" ? ABE_RIGHT
                      : position == L"Top" ? ABE_TOP : ABE_BOTTOM;
        abd.rc = mi.rcMonitor;
        SHAppBarMessage(ABM_QUERYPOS, &abd);
        if (abd.uEdge == ABE_LEFT) abd.rc.right = abd.rc.left + pixels;
        if (abd.uEdge == ABE_RIGHT) abd.rc.left = abd.rc.right - pixels;
        if (abd.uEdge == ABE_TOP) abd.rc.bottom = abd.rc.top + pixels;
        if (abd.uEdge == ABE_BOTTOM) abd.rc.top = abd.rc.bottom - pixels;
        SHAppBarMessage(ABM_SETPOS, &abd);
        SetWindowPos(hwnd_, HWND_TOPMOST, abd.rc.left, abd.rc.top, abd.rc.right - abd.rc.left,
                     abd.rc.bottom - abd.rc.top, SWP_NOACTIVATE);
    }

    void MainWindow::UnregisterAppbar()
    {
        RemoveWindowSubclass(hwnd_, SubclassProc, 1);
        APPBARDATA abd{ sizeof(abd) };
        abd.hWnd = hwnd_;
        SHAppBarMessage(ABM_REMOVE, &abd);
    }

    LRESULT CALLBACK MainWindow::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR,
                                               DWORD_PTR ref)
    {
        auto self = reinterpret_cast<MainWindow*>(ref);
        APPBARDATA abd{ sizeof(abd) };
        abd.hWnd = hwnd;
        if (msg == self->appbarMessage_)
        {
            if (wParam == ABN_POSCHANGED) self->PlaceAppbar();
            return 0;
        }
        if (msg == WM_DISPLAYCHANGE || msg == WM_DPICHANGED) self->PlaceAppbar();
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    void MainWindow::OnItems(IJsonValue const& value)
    {
        auto object = value.GetObject();
        items_ = JsonArray();
        for (auto const& name : { L"left", L"center", L"right" })
        {
            for (auto const& item : object.GetNamedArray(name, JsonArray())) items_.Append(item);
            JsonObject separator;
            separator.SetNamedValue(L"type", JsonValue::CreateStringValue(L"Separator"));
            items_.Append(separator);
        }
        Render();
    }

    void MainWindow::OnWindows(IJsonValue const& value)
    {
        windows_ = value.GetArray();
        Render();
    }

    void MainWindow::OnFocused(IJsonValue const& value)
    {
        focusedHwnd_ = static_cast<int64_t>(value.GetObject().GetNamedNumber(L"hwnd", 0));
        Render();
    }

    void MainWindow::OnSettings(IJsonValue const& value)
    {
        auto settings = value.GetObject().GetNamedObject(L"byWidget", JsonObject());
        settings_ = settings.GetNamedObject(L"@seelen/weg", JsonObject());
        Items().Orientation(IsVertical(settings_) ? Orientation::Vertical : Orientation::Horizontal);
        PlaceAppbar();
        Render();
    }

    void MainWindow::Render()
    {
        if (!items_ || !windows_) return;
        Items().Children().Clear();
        for (auto const& value : items_)
        {
            auto item = value.GetObject();
            if (item.GetNamedString(L"type", L"") == L"Separator")
            {
                Border separator;
                separator.Width(IsVertical(settings_) ? 1 : 8);
                separator.Height(IsVertical(settings_) ? 8 : 1);
                Items().Children().Append(separator);
                continue;
            }
            if (item.GetNamedString(L"type", L"") != L"AppOrFile") continue;
            Button button;
            button.Padding(Thickness(6));
            button.Content(box_value(item.GetNamedString(L"displayName", L"App")));
            button.Click([this, item](auto&&, auto&&) { ActivateItem(item); });
            button.PointerPressed([this, item, button](auto&&, PointerRoutedEventArgs const& args) {
                if (args.GetCurrentPoint(nullptr).Properties().PointerUpdateKind() ==
                    Windows::Devices::Input::PointerUpdateKind::RightButtonPressed)
                    SetMenu(button, item);
            });
            Items().Children().Append(button);
        }
    }

    void MainWindow::ActivateItem(JsonObject const& item)
    {
        int64_t hwnd = 0;
        auto path = item.GetNamedString(L"path", L"");
        auto umid = item.GetNamedString(L"umid", L"");
        for (auto const& value : windows_)
        {
            auto window = value.GetObject();
            if ((!umid.empty() && window.GetNamedString(L"umid", L"") == umid) ||
                (umid.empty() && window.GetNamedObject(L"process", JsonObject()).GetNamedString(L"path", L"") == path))
            {
                hwnd = static_cast<int64_t>(window.GetNamedNumber(L"hwnd", 0));
                break;
            }
        }
        if (hwnd && core_)
        {
            auto args = JsonObject();
            args.SetNamedValue(L"hwnd", JsonValue::CreateNumberValue(hwnd));
            args.SetNamedValue(L"wasFocused", JsonValue::CreateBooleanValue(hwnd == focusedHwnd_));
            auto operation = core_->Invoke(L"weg_native_toggle_window_state", args);
            operation.Completed([](auto&&, auto&&) {});
            return;
        }
        if (core_)
        {
            auto args = JsonObject();
            hstring program = umid.empty() ? path : hstring(L"shell:AppsFolder\\") + umid;
            args.SetNamedValue(L"program", JsonValue::CreateStringValue(program));
            auto operation = core_->Invoke(L"weg_native_run", args);
            operation.Completed([](auto&&, auto&&) {});
        }
    }

    void MainWindow::CloseItem(JsonObject const& item)
    {
        if (!core_ || !windows_) return;
        auto path = item.GetNamedString(L"path", L"");
        auto umid = item.GetNamedString(L"umid", L"");
        for (auto const& value : windows_)
        {
            auto window = value.GetObject();
            if ((!umid.empty() && window.GetNamedString(L"umid", L"") == umid) ||
                (umid.empty() && window.GetNamedObject(L"process", JsonObject()).GetNamedString(L"path", L"") == path))
            {
                auto args = JsonObject();
                args.SetNamedValue(L"hwnd", window.GetNamedValue(L"hwnd"));
                auto operation = core_->Invoke(L"weg_native_close_app", args);
                operation.Completed([](auto&&, auto&&) {});
            }
        }
    }

    void MainWindow::SetMenu(Button const& button, JsonObject const& item)
    {
        MenuFlyout menu;
        MenuFlyoutItem close;
        close.Text(L"Close");
        close.Click([this, item](auto&&, auto&&) { CloseItem(item); });
        menu.Items().Append(close);
        button.ContextFlyout(menu);
        menu.ShowAt(button);
    }
}
