#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source "$project_dir/scripts/qt_environment.sh"
# Rediscover SDK-dependent paths when a local SDK replaces an old/deleted prefix.
build_options=(-S "$project_dir" -B "$project_dir/build" -DCMAKE_BUILD_TYPE=Release
    -U 'Qt6*_DIR' -U 'OPENGL_*' -U 'Vulkan_*')
if [[ -n "$qt_prefix" ]]; then build_options+=("-DCMAKE_PREFIX_PATH=$qt_prefix"); fi
cmake "${build_options[@]}" "$@"
cmake --build "$project_dir/build" -j2
