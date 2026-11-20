#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
#include <microsoft.ui.xaml.window.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

using namespace winrt;
using namespace Microsoft::UI::Composition::SystemBackdrops;
using namespace Microsoft::UI::Windowing;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Windows::Data::Json;

namespace
{
    // effective pixels, same default as the WebView2 toolbar
    constexpr int kHeight = 30;

    Brush ThemeBrush(wchar_t const* key)
    {
        return Application::Current().Resources().Lookup(box_value(key)).as<Brush>();
    }
}

namespace winrt::FancyToolbar::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
        Window window = *this;
        window.as<IWindowNative>()->get_WindowHandle(&hwnd_);

        SetupWindow();
        SetupBackdrop();
        RegisterAppbar();

        clockTimer_ = DispatcherTimer();
        clockTimer_.Interval(std::chrono::seconds(1));
        clockTimer_.Tick([this](auto&&, auto&&) { UpdateClock(); });
        clockTimer_.Start();
        UpdateClock();

        Closed([this](auto&&, auto&&) {
            UnregisterAppbar();
            if (mica_) mica_.Close();
            if (acrylic_) acrylic_.Close();
        });

        core_ = ::FancyToolbar::CoreClient::FromStdin();
        if (core_)
        {
            core_->Start(
                {
                    { L"global-focus-changed", [this](IJsonValue const& v) { OnFocusedApp(v.GetObject()); } },
                    { L"virtual-desktops::changed", [this](IJsonValue const& v) { OnVirtualDesktops(v.GetObject()); } },
                },
                {
                    { L"get_focused_app", L"global-focus-changed" },
                    { L"get_virtual_desktops", L"virtual-desktops::changed" },
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

        // Windows 11 still draws a 1px frame and rounds the corners of a borderless window
        COLORREF none = DWMWA_COLOR_NONE;
        DwmSetWindowAttribute(hwnd_, DWMWA_BORDER_COLOR, &none, sizeof(none));
        DWM_WINDOW_CORNER_PREFERENCE corners = DWMWCP_DONOTROUND;
        DwmSetWindowAttribute(hwnd_, DWMWA_WINDOW_CORNER_PREFERENCE, &corners, sizeof(corners));
    }

    void MainWindow::SetupBackdrop()
    {
        backdropConfig_ = SystemBackdropConfiguration();
        // a toolbar is never the active window; without this Mica falls back to a flat colour
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
        appbarMessage_ = RegisterWindowMessageW(L"SeelenUI.FancyToolbar.Appbar");
        APPBARDATA abd{ sizeof(abd) };
        abd.hWnd = hwnd_;
        abd.uCallbackMessage = appbarMessage_;
        SHAppBarMessage(ABM_NEW, &abd);
        SetWindowSubclass(hwnd_, SubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
        PlaceAppbar();
    }

    void MainWindow::PlaceAppbar()
    {
        // shortcut: primary monitor only; the WebView2 toolbar is one instance per monitor, spawn one per monitor before shipping
        MONITORINFO mi{ sizeof(mi) };
        GetMonitorInfoW(MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &mi);
        int height = MulDiv(kHeight, GetDpiForWindow(hwnd_), 96);

        APPBARDATA abd{ sizeof(abd) };
        abd.hWnd = hwnd_;
        abd.uEdge = ABE_TOP;
        abd.rc = mi.rcMonitor;
        SHAppBarMessage(ABM_QUERYPOS, &abd);
        abd.rc.bottom = abd.rc.top + height;
        SHAppBarMessage(ABM_SETPOS, &abd);

        SetWindowPos(hwnd_, HWND_TOPMOST, abd.rc.left, abd.rc.top, abd.rc.right - abd.rc.left, height,
                     SWP_NOACTIVATE);
    }

    void MainWindow::UnregisterAppbar()
    {
        RemoveWindowSubclass(hwnd_, SubclassProc, 1);
        APPBARDATA abd{ sizeof(abd) };
        abd.hWnd = hwnd_;
        SHAppBarMessage(ABM_REMOVE, &abd);
    }

    LRESULT CALLBACK MainWindow::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR ref)
    {
        auto self = reinterpret_cast<MainWindow*>(ref);
        APPBARDATA abd{ sizeof(abd) };
        abd.hWnd = hwnd;
        if (msg == self->appbarMessage_)
        {
            if (wParam == ABN_POSCHANGED)
            {
                self->PlaceAppbar();
            }
            return 0;
        }
        switch (msg)
        {
        case WM_ACTIVATE:
            SHAppBarMessage(ABM_ACTIVATE, &abd);
            break;
        case WM_WINDOWPOSCHANGED:
            SHAppBarMessage(ABM_WINDOWPOSCHANGED, &abd);
            break;
        case WM_DISPLAYCHANGE:
        case WM_DPICHANGED:
            self->PlaceAppbar();
            break;
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    void MainWindow::UpdateClock()
    {
        wchar_t date[64]{}, time[64]{};
        GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_SHORTDATE, nullptr, nullptr, date, 64, nullptr);
        GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, TIME_NOSECONDS, nullptr, nullptr, time, 64);
        Clock().Text(hstring(date) + L"  " + time);
    }

    void MainWindow::OnFocusedApp(JsonObject const& app)
    {
        FocusedTitle().Text(app.GetNamedString(L"title", L""));
    }

    void MainWindow::OnVirtualDesktops(JsonObject const& desktops)
    {
        // shortcut: shows the first monitor's workspaces; match the toolbar's monitor id once there is one toolbar per monitor
        auto monitors = desktops.GetNamedObject(L"monitors", JsonObject());
        if (monitors.Size() == 0)
        {
            return;
        }
        auto monitor = monitors.First().Current().Value().GetObject();
        // VirtualDesktopMonitor has no serde rename, keep the camelCase spelling as fallback
        auto active = monitor.HasKey(L"active_workspace") ? monitor.GetNamedString(L"active_workspace")
                                                          : monitor.GetNamedString(L"activeWorkspace", L"");

        auto dots = Workspaces().Children();
        dots.Clear();
        for (auto const& row : monitor.GetNamedArray(L"workspaces", JsonArray()))
        {
            for (auto const& workspace : row.GetArray())
            {
                bool isActive = workspace.GetObject().GetNamedString(L"id", L"") == active;
                Microsoft::UI::Xaml::Shapes::Ellipse dot;
                dot.Width(isActive ? 16 : 8);
                dot.Height(8);
                dot.Fill(ThemeBrush(isActive ? L"AccentFillColorDefaultBrush" : L"TextFillColorTertiaryBrush"));
                dots.Append(dot);
            }
        }
    }
}
