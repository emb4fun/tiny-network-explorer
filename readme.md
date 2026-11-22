# Tiny Network Explorer

**Tiny Network Explorer (TNE)** discovers and configures Tiny network devices on the local LAN. It sends UDP broadcast requests, lists responding devices (MAC address, IP, name, location), and lets you change settings or open a device in the browser.

This repository is a modern port of the original Win32 application. The networking engine is platform-neutral C; the GUI uses **wxWidgets 3.2**.

**Supported platforms:** Windows and Linux only.

The porting to MSYS2-UCRT64 and Linux was done with AI assistance.

## Quick start

| Platform | Build guide |
|----------|-------------|
| Windows (MSYS2 UCRT64) | [readme-win.md](readme-win.md) |
| Linux (Debian, Fedora, …) | [readme-linux.md](readme-linux.md) |

**Windows**

```bash
cmake --preset ucrt64
cmake --build --preset ucrt64
```

**Linux**

```bash
cmake --preset linux-release
cmake --build --preset linux-release
```

Build output is written to `build/`. The GUI executable is `tnexpl` (`tnexpl.exe` on Windows).

## Legacy reference

The original **Win32/C** sources are kept unchanged under [`legacy/`](legacy/) for reference. That code includes the old dialog resources, ListView UI, and Windows-specific networking glue.

The new implementation does not modify `legacy/`; it reimplements behaviour in:

- [`core/`](core/) — portable `tnp_engine` (protocol, device list, UDP discovery)
- [`src/`](src/) — wxWidgets GUI (`tnexpl`)
- [`tools/`](tools/) — console test tool (`tnp_engine_console`)

When comparing behaviour or UI details, check the legacy tree first.

## Project layout

```
tne/
├── CMakeLists.txt          Root build; options TNE_BUILD_WX, TNE_BUILD_CONSOLE
├── CMakePresets.json       Presets for Windows (ucrt64) and Linux
├── cmake/                  Toolchain hints (MSYS2, Linux)
├── core/                   tnp_engine — shared C library
├── src/                    wxWidgets GUI (MainFrame, dialogs, resources)
├── tools/                  tnp_engine_console — CLI discovery test
└── legacy/                 Original Win32 application (reference only)
```

## Build targets

| Target | Description |
|--------|-------------|
| `tnexpl` | Main GUI application |
| `tnp_engine_console` | Command-line tool to test discovery without the GUI |

CMake options (see root `CMakeLists.txt`):

| Option | Default | Description |
|--------|---------|-------------|
| `TNE_BUILD_WX` | `ON` | Build the wxWidgets GUI |
| `TNE_BUILD_CONSOLE` | `ON` | Build the console test tool |

## Runtime notes

- Only **one instance** can use UDP port **54321** at a time.
- Discovery uses **UDP broadcast** on the local subnet; the host must be on the same LAN as the devices.
- On **Linux**, keep the `resources/` folder next to the `tnexpl` binary (copied automatically during build). On **Windows**, icons and bitmaps are embedded in the executable.
- If a Linux build seems stale after syncing sources, remove `build/` and configure again (see [readme-linux.md](readme-linux.md#troubleshooting)).

For platform-specific packaging, troubleshooting, and dependencies, use the dedicated guides linked above.

## License

See the BSD-style license header in the source files (e.g. [`src/license-header.txt`](src/license-header.txt)).

Copyright (c) Michael Fischer — [www.emb4fun.de](https://www.emb4fun.de)
