#pragma once
#include "MainWindow.g.h"
#include "pch.h"
#include "CoreClient.h"

namespace winrt::FancyToolbar::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

    private:
        HWND hwnd_{ nullptr };
        UINT appbarMessage_{ 0 };
        Microsoft::UI::Composition::SystemBackdrops::MicaController mica_{ nullptr };
        Microsoft::UI::Composition::SystemBackdrops::DesktopAcrylicController acrylic_{ nullptr };
        Microsoft::UI::Composition::SystemBackdrops::SystemBackdropConfiguration backdropConfig_{ nullptr };
        Microsoft::UI::Xaml::DispatcherTimer clockTimer_{ nullptr };
        std::shared_ptr<::FancyToolbar::CoreClient> core_;

        void SetupWindow();
        void SetupBackdrop();
        void RegisterAppbar();
        void PlaceAppbar();
        void UnregisterAppbar();
        void UpdateClock();
        void OnFocusedApp(Windows::Data::Json::JsonObject const& app);
        void OnVirtualDesktops(Windows::Data::Json::JsonObject const& desktops);

        static LRESULT CALLBACK SubclassProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    };
}

namespace winrt::FancyToolbar::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
