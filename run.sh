#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$project_dir/scripts/qt_environment.sh"
# Reconfigure cached paths and rebuild changed sources. CMake's incremental build
# makes normal relaunches quick, and prevents silently running an outdated binary.
if ! "$project_dir/scripts/build.sh"; then
    printf '\nBuild failed. Install Qt 6 or run python3 scripts/setup_local_qt.py.\n' >&2
    exit 1
fi
exec "$project_dir/build/typatype" "$@"
