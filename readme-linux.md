# Tiny Network Explorer — Linux Build Guide

This project builds on **Linux** (tested on **Debian 13**) using the system **GCC** toolchain, **Ninja**, and **CMake**. The GUI uses **wxWidgets 3.2** (GTK, linked dynamically).

## Prerequisites

Install the build tools and wxWidgets development packages.

### Debian 13 / Ubuntu

```bash
sudo apt update
sudo apt install --no-install-recommends \
  build-essential \
  cmake \
  ninja-build \
  libwxgtk3.2-dev
```

| Component | Package |
|-----------|---------|
| GCC, make | `build-essential` |
| CMake | `cmake` |
| Ninja | `ninja-build` |
| wxWidgets 3.2 (GTK) | `libwxgtk3.2-dev` |

### Fedora / RHEL

```bash
sudo dnf install gcc gcc-c++ cmake ninja-build wxGTK3-devel
```

Package names may differ slightly on other distributions; you need a C/C++ compiler, CMake (≥ 3.16), Ninja, and wxWidgets 3.2 with GTK.

## Build

From the project root:

```bash
cmake --preset linux-release
cmake --build --preset linux-release
```

Debug build:

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
```

Build output directory: `build/`

## Build outputs

| Target | Path | Description |
|--------|------|-------------|
| GUI | `build/src/tnexpl` | Main application |
| GUI resources | `build/src/resources/` | Icons and bitmaps (required at runtime) |
| Console tool | `build/tools/tnp_engine_console` | Command-line discovery test tool |

During the build, `src/resources/` is copied to `build/src/resources/` automatically. This includes `app.png` and sized PNG icons used for the window and taskbar.

A desktop entry file is generated at `build/src/tnexpl.desktop` for launcher integration.

## Running

### GUI

Run from the directory that contains the `resources/` folder:

```bash
./build/src/tnexpl
```

Or:

```bash
cd build/src
./tnexpl
```

The `resources/` directory must stay next to the `tnexpl` binary (same folder). It contains icons (`app.png`), bitmaps for the About dialog, and other GUI assets.

### Desktop launcher (optional)

After building, install the generated desktop file locally:

```bash
desktop-file-install --dir="$HOME/.local/share/applications" build/src/tnexpl.desktop
update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
```

You can then start **Tiny Network Explorer** from your application menu with the correct icon.

### Console test tool

```bash
./build/tools/tnp_engine_console -t 10
```

Options:

```
-t, --timeout SEC   Wait for responses (default: 10 seconds)
-q, --quiet         Summary only, no live events
-h, --help          Show help
```

## CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `TNE_BUILD_WX` | `ON` | Build the wxWidgets GUI |
| `TNE_BUILD_CONSOLE` | `ON` | Build `tnp_engine_console` |

Example — console tool only:

```bash
cmake --preset linux-release -DTNE_BUILD_WX=OFF
cmake --build --preset linux-release
```

## Deploying the GUI

Copy both the executable and the resources directory:

```bash
install -d /opt/tnexpl
install -m 755 build/src/tnexpl /opt/tnexpl/
cp -a build/src/resources /opt/tnexpl/
```

Then run `/opt/tnexpl/tnexpl`.

wxWidgets is linked dynamically; target systems need the wxGTK runtime libraries (e.g. `libwxgtk3.2-0` on Debian).

## Notes

- Only one instance of the application can bind UDP port **54321** at a time.
- Discovery uses UDP broadcast on the local subnet. The machine must be on the **same LAN** as the Tiny devices (e.g. `192.168.1.x`).
- **WSL2** is fine for building and testing the GUI, but UDP broadcast discovery usually does **not** reach devices on your home network unless you use advanced WSL networking (mirrored mode). Use a real Linux machine on the LAN for reliable device discovery tests.
- Under Linux/GTK, empty rows in the device list do not show grid lines (cosmetic difference from Windows). Device rows, sorting, and double-click actions work as expected.

## Troubleshooting

**Build skips recompilation or behaves oddly after copying/updating sources**

If you sync the project from another machine (or restore files from an archive), source file timestamps can end up **older** than object files already in `build/`. Ninja then assumes everything is up to date and may not rebuild, even though sources changed. Running `touch` on the sources forces a rebuild, which is why that workaround helps.

Prefer a clean build instead:

```bash
rm -rf build
cmake --preset linux-release
cmake --build --preset linux-release
```

After large updates, reconfigure as well:

```bash
cmake --preset linux-release
cmake --build --preset linux-release
```

**`Could not find wxWidgets`**

Install `libwxgtk3.2-dev` (or the wxWidgets 3.2 development package for your distribution).

**GUI starts but shows an icon error**

Ensure `wxInitAllImageHandlers()` is used (already in the project) and that `build/src/resources/app.ico` exists after the build.

**No devices found, but Wireshark shows responses**

The engine binds UDP sockets with `INADDR_ANY` on Linux so broadcast replies to `255.255.255.255` are received. If problems persist, check firewall rules for UDP port **54321** and confirm you are on the correct network interface.

**`Failed to bind UDP sockets`**

Another process (or a second copy of TNE) is already using port 54321. Close it and try again.
