#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
qt_root=${QT_ROOT:-"$HOME/Qt/6.12.0/macos"}
source_app="$project_dir/build-gui/Project1.app"
target_app="$project_dir/dist/Project1.app"
if [ ! -d "$source_app" ] || [ ! -x "$qt_root/bin/macdeployqt" ]; then
    printf '%s\n' '请先编译 GUI，并确保 QT_ROOT 下包含 macdeployqt。' >&2
    exit 1
fi
mkdir -p "$project_dir/dist"
ditto "$source_app" "$target_app"
"$qt_root/bin/macdeployqt" "$target_app" -always-overwrite -verbose=1
printf '已生成包含 Qt 运行库的应用：%s\n' "$target_app"
