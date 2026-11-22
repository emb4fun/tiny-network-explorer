# Native Linux toolchain (GCC/Clang from PATH, wxWidgets via wx-config/pkg-config).
#
# Usage:
#   cmake --preset linux-release
#   cmake --build --preset linux-release
#
# Dependencies (Debian 13 / Ubuntu example):
#   sudo apt install build-essential cmake ninja-build libwxgtk3.2-dev
#
# Run the GUI from build/src/ (resources/ is copied there during the build).

set(CMAKE_CXX_EXTENSIONS OFF)
