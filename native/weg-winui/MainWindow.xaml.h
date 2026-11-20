#pragma once
#include "MainWindow.g.h"
#include "pch.h"

namespace winrt::Weg::implementation
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
        std::unique_ptr<::Seelen::CoreClient> core_;
        Windows::Data::Json::JsonArray items_{ nullptr };
        Windows::Data::Json::JsonArray windows_{ nullptr };
        Windows::Data::Json::JsonObject settings_{ nullptr };
        int64_t focusedHwnd_{ 0 };

        void SetupWindow();
        void SetupBackdrop();
        void RegisterAppbar();
        void PlaceAppbar();
        void UnregisterAppbar();
        void Render();
        void OnItems(Windows::Data::Json::IJsonValue const&);
        void OnWindows(Windows::Data::Json::IJsonValue const&);
        void OnFocused(Windows::Data::Json::IJsonValue const&);
        void OnSettings(Windows::Data::Json::IJsonValue const&);
        void ActivateItem(Windows::Data::Json::JsonObject const&);
        void CloseItem(Windows::Data::Json::JsonObject const&);
        void SetMenu(Microsoft::UI::Xaml::Controls::Button const&, Windows::Data::Json::JsonObject const&);

        static LRESULT CALLBACK SubclassProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    };
}

namespace winrt::Weg::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
