#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
qt_root=${QT_ROOT:-"$HOME/Qt/6.12.0/macos"}
if command -v cmake >/dev/null 2>&1; then
    cmake_bin=$(command -v cmake)
else
    cmake_bin="$HOME/Qt/Tools/CMake/CMake.app/Contents/bin/cmake"
fi
if [ ! -x "$cmake_bin" ] || [ ! -f "$qt_root/lib/cmake/Qt6/Qt6Config.cmake" ]; then
    printf '%s\n' '未找到 CMake 或 Qt 开发库。可用 Qt Creator 打开 CMakeLists.txt，或用 QT_ROOT 指定 Qt 安装路径。' >&2
    exit 1
fi
"$cmake_bin" -S "$project_dir" -B "$project_dir/build-gui" \
    -DCMAKE_PREFIX_PATH="$qt_root" -DCMAKE_BUILD_TYPE=Release
"$cmake_bin" --build "$project_dir/build-gui" --parallel 2
if [ -d "$project_dir/build-gui/Project1.app" ]; then
    open "$project_dir/build-gui/Project1.app"
else
    exec "$project_dir/build-gui/Project1"
fi
