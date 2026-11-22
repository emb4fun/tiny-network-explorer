# Tiny Network Explorer — Windows Build Guide

This project builds on **Windows 10/11** using **MSYS2 UCRT64**, **GCC**, **Ninja**, and **CMake**. The GUI uses **wxWidgets** (linked statically). Icons and bitmaps are embedded in `tnexpl.exe`, so you can distribute the GUI as a single executable.

## Prerequisites

### 1. Install MSYS2

Download and install MSYS2 from [https://www.msys2.org/](https://www.msys2.org/).

Default installation path:

```
C:\msys64
```

If you use a different path, set the `MSYS2_ROOT` environment variable before configuring CMake.

### 2. Install build tools and dependencies

Open the **MSYS2 UCRT64** terminal (not MSYS or MINGW64) and run:

```bash
pacman -Syu
pacman -S --needed \
  mingw-w64-ucrt-x86_64-toolchain \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-wxwidgets3.2-msw
```

This installs:

| Component | Package |
|-----------|---------|
| GCC (C/C++) | `mingw-w64-ucrt-x86_64-toolchain` |
| CMake | `mingw-w64-ucrt-x86_64-cmake` |
| Ninja | `mingw-w64-ucrt-x86_64-ninja` |
| wxWidgets 3.2 | `mingw-w64-ucrt-x86_64-wxwidgets3.2-msw` |

### 3. CMake on PATH (optional)

CMake presets expect MSYS2 UCRT64 tools on `PATH`. You can either:

- run builds from the **MSYS2 UCRT64** shell, or
- add `C:\msys64\ucrt64\bin` and `C:\msys64\usr\bin` to your Windows `PATH` (as the presets already do when using `cmake --preset`).

## Build

From the project root:

```bash
cmake --preset ucrt64
cmake --build --preset ucrt64
```

Debug build:

```bash
cmake --preset ucrt64-debug
cmake --build --preset ucrt64-debug
```

Build output directory: `build/`

## Build outputs

| Target | Path | Description |
|--------|------|-------------|
| GUI | `build/src/tnexpl.exe` | Main application (portable, resources embedded) |
| Console tool | `build/tools/tnp_engine_console.exe` | Command-line discovery test tool |

## Running

### GUI

```bash
build/src/tnexpl.exe
```

No `resources/` folder is required next to the executable on Windows.

### Console test tool

```bash
build/tools/tnp_engine_console.exe -t 10
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
cmake --preset ucrt64 -DTNE_BUILD_WX=OFF
cmake --build --preset ucrt64
```

## Notes

- Only one instance of the application can bind UDP port **54321** at a time.
- Discovery uses UDP broadcast on the local subnet; your PC must be on the same network as the Tiny devices.
- Executables are linked **statically** (MinGW `-static`), so they do not depend on MSYS2 DLLs at runtime.

## Troubleshooting

**`cmake --preset ucrt64` fails to find wxWidgets**

Ensure wxWidgets is installed in the UCRT64 environment and that `C:\msys64\ucrt64` exists. CMake looks there automatically on Windows.

**Linker error: `Permission denied` on `tnexpl.exe`**

Close the running `tnexpl.exe` and rebuild.

**MSYS2 installed elsewhere**

```bash
export MSYS2_ROOT=/d/Path/To/msys64
cmake --preset ucrt64
cmake --build --preset ucrt64
```
