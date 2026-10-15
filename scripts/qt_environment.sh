# Shared by build.sh/run.sh. It performs no network access or package installation.
qt_prefix="${TYPATYPE_QT_PREFIX:-}"
if [[ -z "$qt_prefix" && -d "$project_dir/.deps/qt/usr" ]]; then
    qt_prefix="$project_dir/.deps/qt/usr"
fi
if [[ -n "$qt_prefix" ]]; then
    if [[ ! -d "$qt_prefix" ]]; then
        printf 'Qt SDK does not exist: %s\n' "$qt_prefix" >&2
        exit 1
    fi
    # Debian/Ubuntu use multiarch directories; official Qt SDKs use lib/ and plugins/.
    qt_lib="$qt_prefix/lib"
    if command -v dpkg-architecture >/dev/null 2>&1; then
        qt_triplet="$(dpkg-architecture -qDEB_HOST_MULTIARCH)"
        if [[ -d "$qt_prefix/lib/$qt_triplet" ]]; then qt_lib="$qt_prefix/lib/$qt_triplet"; fi
    fi
    export LD_LIBRARY_PATH="$qt_lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    if [[ -d "$qt_prefix/plugins" ]]; then
        export QT_PLUGIN_PATH="$qt_prefix/plugins${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
    elif [[ -d "$qt_lib/qt6/plugins" ]]; then
        export QT_PLUGIN_PATH="$qt_lib/qt6/plugins${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
    fi
fi
