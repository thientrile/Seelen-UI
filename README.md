<h1 align="center">
  <img src="src/static/icons/app/icon.svg" width="44" align="top" alt="Seelen UI Logo" />
  Seelen UI
</h1>

<h2 align="center">
  Fully Customizable Desktop Environment for Windows
  <br/>
  Available in 70+ Languages
</h2>

<div align="center">

[![Contributors](https://img.shields.io/github/contributors/thientrile/Seelen-UI.svg)](https://github.com/thientrile/Seelen-UI/graphs/contributors)
[![Last Commit](https://img.shields.io/github/last-commit/thientrile/Seelen-UI.svg)](https://github.com/thientrile/Seelen-UI/commits/master)
[![Version](https://img.shields.io/github/v/release/thientrile/Seelen-UI.svg)](https://github.com/thientrile/Seelen-UI/releases)
[![Downloads](https://img.shields.io/github/downloads/thientrile/Seelen-UI/total.svg)](https://github.com/thientrile/Seelen-UI/releases)

</div>

<img src="./documentation/images/preview.png" width="100%" alt="Screenshot of Seelen UI desktop showing a customized desktop environment">

<table align="center">
  <tr>
    <td align="center" width="50%">
      <a
        href="https://discord.seelen.io"
        target="_blank"
        rel="noopener noreferrer"
        aria-label="Join the Seelen UI Discord community">
        <img src="./documentation/images/discord-alt.png" width="100%" alt="Join the Seelen UI Discord community">
      </a>
    </td>
    <td align="center" width="50%">
      <a
        href='https://www.digitalocean.com/?refcode=955c7335abf5&utm_campaign=Referral_Invite&utm_medium=Referral_Program&utm_source=badge'
        target='_blank'
        rel="noopener noreferrer"
        aria-label="DigitalOcean Referral Badge"
      >
        <img src='https://web-platforms.sfo2.cdn.digitaloceanspaces.com/WWW/Badge%201.svg' width="100%" alt='DigitalOcean Referral Badge' />
      </a>
    </td>
  </tr>
</table>

---

## What is Seelen UI?

[Seelen UI](https://github.com/thientrile/Seelen-UI) is a fork of the original
[Seelen UI project](https://github.com/eythaann/Seelen-UI) by eythaann. This fork keeps the original project's Rust core
and license; see [LICENSE](LICENSE) for the full license text.

[Seelen UI](https://seelen.io/apps/seelen-ui) is a full desktop environment replacement for Windows — not a theme pack,
not a skin, but a complete rethink of how your desktop looks and works. Every element you interact with daily — the
taskbar, the dock, the app launcher, the window manager, the notification flyouts, the virtual desktops — can be
replaced, restyled, and extended to exactly match your vision and workflow.

Whether you want a minimal, distraction-free setup or a feature-rich power-user environment, Seelen UI gives you the
primitives to build it. With a CSS/JSON-based theming engine, a plugin widget system, and deep Windows integration, the
only limit is your imagination.

---

## Feature Showcase

### Themes & Visual Identity

Make Windows look the way you always wanted. Seelen UI ships with a powerful theming engine that lets you craft or
install community themes covering every surface of the shell — colors, fonts, borders, animations, and more.

![Seelen UI Custom Theme](./documentation/images/theme_preview.png)

<br/>

### Dynamic Accent Color

Set your accent color once and forget it. Seelen UI analyzes your wallpaper in real time and automatically derives a
harmonious accent color, keeping every shell element visually in sync with your background — no manual tweaking needed.

<table>
  <tr>
    <td><img src="./documentation/images/dynamic_accent_1.png" alt="Dynamic accent example 1"/></td>
    <td><img src="./documentation/images/dynamic_accent_2.png" alt="Dynamic accent example 2"/></td>
  </tr>
  <tr>
    <td><img src="./documentation/images/dynamic_accent_3.png" alt="Dynamic accent example 3"/></td>
    <td><img src="./documentation/images/dynamic_accent_4.png" alt="Dynamic accent example 4"/></td>
  </tr>
</table>

<br/>

### Tiling Window Manager

Stop dragging windows. Seelen UI's tiling window manager automatically arranges your open applications into efficient,
gap-controlled layouts. Resize, swap, and navigate entirely from the keyboard — your hands never leave the keys.

![Seelen UI Tiling Window Manager](./documentation/images/twm_preview.png)

<br/>

### Desktop Widgets

Bring information to your desktop without opening apps. Place clocks, system monitors, media controls, weather panels,
and any custom widget directly on your wallpaper layer — fully theme-aware and always within sight.

![Seelen UI Desktop Widgets](./documentation/images/desktop_widgets.png)

<br/>

### App Launcher

Launch anything instantly with a keyboard-driven launcher inspired by [Rofi](https://github.com/davatorium/rofi). Apps,
files, shell commands, custom scripts — surfaced with fuzzy search in milliseconds.

![Seelen UI App Launcher](./documentation/images/app_launcher_preview.png)

<br/>

### Media Controls

Control your music without switching windows. The integrated media module works with virtually every player and lets you
play, pause, skip, and scrub from any context, always accessible in the toolbar or as a dedicated widget.

![Seelen UI Media Module](./documentation/images/media_module_preview.png)

<br/>

### Settings

One unified place to configure everything. Themes, layouts, keybindings, widget positions, workspace rules, per-app
overrides — all accessible through a clean, searchable settings interface with live preview.

![Seelen UI Settings](./documentation/images/settings_preview.png)

---

## Complete Feature List

### Shell & Desktop

| Feature                    | Description                                                                                       |
| -------------------------- | ------------------------------------------------------------------------------------------------- |
| **Custom Toolbar**         | A fully themeable, widget-powered taskbar that replaces the Windows taskbar                       |
| **Custom Dock**            | macOS-style application dock with launch, focus, and badge support                                |
| **Desktop Widgets**        | Interactive widgets rendered directly on the desktop layer                                        |
| **Custom Flyouts**         | Replace Windows' built-in volume, brightness, and system flyouts with themed, customizable panels |
| **Start Menu Replacement** | Quick-access launcher replacing the native Start menu                                             |

### Window Management

| Feature                   | Description                                                     |
| ------------------------- | --------------------------------------------------------------- |
| **Tiling Window Manager** | Automatic tiling layouts: BSP, columns, stacks, and more        |
| **Custom Alt + Tab**      | A fully skinnable task switcher with richer window previews     |
| **Per-App Rules**         | Define layout, workspace, and floating behavior per application |
| **Floating Mode**         | Opt any window out of tiling while keeping the rest managed     |
| **Gaps & Padding**        | Configurable inner and outer gaps for any layout                |

### Workspaces

| Feature                     | Description                                                                   |
| --------------------------- | ----------------------------------------------------------------------------- |
| **Custom Virtual Desktops** | Full virtual desktop implementation with smooth animated transitions          |
| **Workspaces Viewer**       | A visual overview panel showing all workspaces and their content at a glance  |
| **Wallpaper per Workspace** | Assign a unique wallpaper to each workspace — changes automatically on switch |
| **Workspace Rules**         | Pin apps or app groups to specific workspaces on launch                       |
| **Named Workspaces**        | Give each workspace a custom name and icon                                    |

### Theming & Appearance

| Feature                     | Description                                                         |
| --------------------------- | ------------------------------------------------------------------- |
| **CSS/JSON Theming Engine** | Style every shell element with scoped CSS and JSON configuration    |
| **Dynamic Accent Color**    | Auto-extract accent color from the current wallpaper                |
| **Community Themes**        | Install and share themes from the community                         |
| **Per-Widget Theming**      | Each widget can carry its own independent style                     |
| **Dark / Light Mode**       | Full support for Windows dark and light mode, themeable per-variant |

### Productivity

| Feature                   | Description                                                 |
| ------------------------- | ----------------------------------------------------------- |
| **Keyboard-First Design** | Every feature reachable without touching the mouse          |
| **App Launcher**          | Fuzzy-search launcher for apps, files, and commands         |
| **Media Module**          | Universal media controls compatible with all major players  |
| **Multi-Monitor Support** | Independent configuration and layouts per connected display |
| **i18n — 70+ Languages**  | Fully localized interface covering major world languages    |

### Developer & Power User

| Feature             | Description                                                                       |
| ------------------- | --------------------------------------------------------------------------------- |
| **Widget SDK**      | Build custom widgets with Svelte, TypeScript, and full IPC access to system state |
| **Portable Config** | All configuration lives in plain files — version-control and sync friendly        |

---

## Installation

> [!CAUTION]
> The current fork build uses WebView2. On Windows 11, it comes pre-installed with the system. On Windows 10, the
> WebView2 runtime is included with the `setup.exe` installer. Microsoft Edge is also required.

> [!NOTE]
> On fresh installations of Windows, the app might show a white or dark screen. You only need to update your Windows
> through Windows Update and restart your PC.

The fork distributes its installer through GitHub Releases.

### Cài bản fork

1. Tải `Seelen.UI_<version>_x64-setup.exe` ở [Releases của fork](https://github.com/thientrile/Seelen-UI/releases) (bản
   mới nhất ở đầu trang) và chạy nó.
2. Installer không được ký bằng chứng chỉ code-signing Windows, nên SmartScreen sẽ báo "Windows protected your PC". Bấm
   **More info** → **Run anyway**. Nếu trình duyệt chặn file tải về, chọn **Keep** / **Keep anyway**.
3. Sau khi cài, app tự kiểm tra và cài bản mới từ Releases của fork (không theo bản gốc `eythaann/Seelen-UI`). Bản fork
   được đánh số `X.Y.Z-fork.N`, với `X.Y.Z` là phiên bản gốc mà fork đang dựa trên.

Phát hành bản mới (maintainer): push tag `vX.Y.Z-fork.N` lên fork; workflow `release-fork.yml` sẽ build installer, tạo
Release và đính kèm `latest.json` cho updater.

The Microsoft Store and Winget packages belong to the original upstream project and are not fork distribution channels.

## WinUI 3 direction

The fork is developing a WinUI 3 UI in C++/WinRT while keeping the Rust core, with the goal of reducing RAM usage
compared with WebView2. The native toolbar prototype is enabled with `SLU_NATIVE_TOOLBAR=1` and communicates with the
core over localhost HTTP and a websocket.

This work is not on `master` yet:

- [#1](https://github.com/thientrile/Seelen-UI/pull/1) is developing the WinUI 3 toolbar prototype and RAM measurement.
- [#3](https://github.com/thientrile/Seelen-UI/pull/3) is developing hardened HTTP and websocket IPC for native widgets.

The current `master` build remains the WebView2 build described above.

---

## Getting Started

Once installed, open Seelen UI and the settings interface will guide you through the initial setup. Enable the
components you want — toolbar, dock, tiling manager, widgets — and start customizing from there. The
[official documentation](https://seelen.io/apps/seelen-ui) and the [Discord community](https://discord.seelen.io) are
the best places to go deeper.

---

## Contributing

We welcome contributions!

- Read the [Contribution Guidelines](CONTRIBUTING) to get started with terms.

Fork builds and release CI use GitHub-hosted Windows runners. Do not use a self-hosted runner for this public fork. Fork
releases are created by pushing tags in the form `vX.Y.Z-fork.N`; the `release-fork.yml` workflow builds the Windows
installer and publishes the GitHub Release.

## License

See the [LICENSE](LICENSE) file for details.

## Contact

For inquiries and support, join us on [Discord](https://discord.seelen.io).

## Sponsors

We're grateful for the support of our sponsors who help make Seelen UI possible.

|                                                                                                         Sponsor                                                                                                          | Description                                                                                                  |
| :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------: | :----------------------------------------------------------------------------------------------------------- |
| [![DigitalOcean](https://web-platforms.sfo2.cdn.digitaloceanspaces.com/WWW/Badge%201.svg)](https://www.digitalocean.com/?refcode=955c7335abf5&utm_campaign=Referral_Invite&utm_medium=Referral_Program&utm_source=badge) | **DigitalOcean** provides cloud infrastructure services that power our development and testing environments. |
|                                                                [![SignPath](https://avatars.githubusercontent.com/u/34448643?s=60)](https://signpath.io/)                                                                | **SignPath** provides free code signing certificates, ensuring secure and trusted releases for our users.    |

---

```
                 .      .&     _,x&"``
                  & .   &'  ;.&&'
            &.  . &.&     .0&&&;&""`
       .    '&  &.&&&  .&&&&&'
     .&         ;&&& &&&&&'
    &&          &&&&&&&&     &&&
   0&    .     &&&&&&&&""
  &&   .0     &&&&&&&
 0&& .&'     &&&&&&
:&&&&&    . &&&&& 
0&&&&    & &&&&&
&&&&'   &&&&&&&               .&&&x&
&&&&   :&&&&&0.&'        , .&&&&&&&&&&;.
&&&&.  &&&&&&&&        .&&&&&&&&&&'               .
0&&&&  &&&&&&&       ,&&&&&&&&&&&&                &
:&&&&; &&&&&0       ,;&&&&&&&&&&&             ;  .0
 0&&&&&&&&&&0     ,;&&&&&&&&&&&&&             &  &;
  0&&&&&&&&&&0   :',;".&&&&&&".&             && &0
   0&&&&&&&&&0  ',;',&&&&&" ,&'             &&&&0
    0&&&&&&&&&0 ,x&&&&" .&&&              &&&&0
      0&&&&&& .&&&&"'''"&&"&&            &&&&&0
       0&& .&&;``       `&: :&         &&&&&&0
          &"' &&&&&&&&   &"& &"&   &&&&&&&&0
            0&&&&&&&&&&&&&&&&&&&&&&&&&0
               0&&&&&&&&&&&&&&&&&&&0         Seelen
                    0&&&&&&&&&0
```

📌 **Official Website**: [https://seelen.io](https://seelen.io)

Seelen Inc © 2026 - All rights reserved
