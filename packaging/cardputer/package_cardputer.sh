#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
PACKAGE_NAME="${PACKAGE_NAME:-m5cardputerzero-ridgedash}"
PACKAGE_SUFFIX="${PACKAGE_SUFFIX:-m5stack1}"
DEB_ARCH="arm64"
MAINTAINER="${MAINTAINER:-m5stack <m5stack@m5stack.com>}"
PARALLEL="${PARALLEL:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build/package}"
STAGE_DIR="${STAGE_DIR:-${ROOT_DIR}/build/deb-root}"
# Assemble the launcher + per-backend binaries under build/ so we never clobber the
# desktop dev build's dist/RidgeDash. Only the final .deb lands in dist/artifacts/.
PKG_STAGE="${PKG_STAGE:-${ROOT_DIR}/build/cardputer-pkg}"
DIST_DIR="${DIST_DIR:-${ROOT_DIR}/dist/artifacts}"
BIN_NAME="RidgeDash"
CMAKE_BIN="${CMAKE:-cmake}"
CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"
RAYLIB_PLATFORM="${RIDGEDASH_PACKAGE_PLATFORM:-AUTO}"

if ! command -v dpkg-deb >/dev/null 2>&1; then
    echo "dpkg-deb not found. Install dpkg-dev first." >&2
    exit 1
fi

if ! command -v "${CMAKE_BIN}" >/dev/null 2>&1; then
    echo "cmake not found. Install CMake or set CMAKE=/path/to/cmake." >&2
    exit 1
fi

read_cmake_cache_value() {
    local name="$1"
    local cache_dir="${2:-${BUILD_DIR}}"
    local cache_file="${cache_dir}/CMakeCache.txt"
    local line=""

    if [[ ! -f "${cache_file}" ]]; then
        echo "CMake cache not found: ${cache_file}" >&2
        return 1
    fi

    line="$(grep -E "^${name}(:[^=]*)?=" "${cache_file}" | tail -n 1 || true)"
    if [[ -z "${line}" ]]; then
        echo "CMake cache value not found: ${name}" >&2
        return 1
    fi

    printf "%s\n" "${line#*=}"
}

host_arch="$(uname -m)"
TOOLCHAIN_ARGS=()
if [[ "${host_arch}" != "aarch64" && "${host_arch}" != "arm64" ]]; then
    if [[ ! -x "$(command -v aarch64-linux-gnu-gcc || true)" || ! -x "$(command -v aarch64-linux-gnu-g++ || true)" ]]; then
        echo "aarch64-linux-gnu-gcc/g++ not found. Install the GNU aarch64 toolchain for cp0 packaging." >&2
        exit 1
    fi
    TOOLCHAIN_ARGS=(-DCMAKE_TOOLCHAIN_FILE="${ROOT_DIR}/cmake/aarch64-linux-gnu.cmake")
fi

configure_and_build() {
    local platform="$1"
    local build_dir="$2"

    "${CMAKE_BIN}" \
        -S "${ROOT_DIR}" \
        -B "${build_dir}" \
        -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
        -DRIDGEDASH_RAYLIB_PLATFORM="${platform}" \
        "${TOOLCHAIN_ARGS[@]}"

    if [[ "${platform}" != "FBDEV" ]] && grep -Eq "(GLESV2|EGL|DRM|GBM).*NOTFOUND" "${build_dir}/CMakeCache.txt"; then
        cat >&2 <<EOF
Invalid package build: raylib DRM target libraries are missing.

For native CardputerZero builds, install:
  sudo apt install libdrm-dev libgbm-dev libegl1-mesa-dev libgles2-mesa-dev

For x86 cross builds, install the arm64 development packages if your apt
sources support them, for example:
  sudo dpkg --add-architecture arm64
  sudo apt update
  sudo apt install libdrm-dev:arm64 libgbm-dev:arm64 libegl1-mesa-dev:arm64 libgles2-mesa-dev:arm64

Alternatively build this package on the target device or set:
  RIDGEDASH_PACKAGE_PLATFORM=FBDEV
EOF
        exit 1
    fi

    "${CMAKE_BIN}" --build "${build_dir}" -j"${PARALLEL}"
}

rm -rf "${PKG_STAGE}"
mkdir -p "${PKG_STAGE}"

if [[ "${RAYLIB_PLATFORM}" == "AUTO" ]]; then
    configure_and_build "DRM" "${BUILD_DIR}-drm"
    PACKAGE_VERSION="$(read_cmake_cache_value CMAKE_PROJECT_VERSION "${BUILD_DIR}-drm")"
    cp "${ROOT_DIR}/dist/${BIN_NAME}" "${PKG_STAGE}/${BIN_NAME}.drm"

    configure_and_build "FBDEV" "${BUILD_DIR}-fbdev"
    cp "${ROOT_DIR}/dist/${BIN_NAME}" "${PKG_STAGE}/${BIN_NAME}.fbdev"

    cat >"${PKG_STAGE}/${BIN_NAME}" <<EOF
#!/bin/sh
set -u
DIR="\$(CDPATH= cd -- "\$(dirname -- "\$0")" && pwd)"
DRM="\${DIR}/${BIN_NAME}.drm"
FBDEV="\${DIR}/${BIN_NAME}.fbdev"

if [ "\${RIDGEDASH_RENDER:-}" = "fbdev" ] || [ "\${RIDGEDASH_SKIP_DRM:-0}" = "1" ]; then
    exec "\${FBDEV}" "\$@"
fi

if [ -x "\${DRM}" ]; then
    start=\$(date +%s)
    "\${DRM}" "\$@"
    status=\$?
    elapsed=\$(( \$(date +%s) - start ))
    if [ "\${status}" -eq 2 ] || { [ "\${status}" -eq 0 ] && [ "\${elapsed}" -lt 2 ]; }; then
        echo "RidgeDash: DRM unavailable, falling back to fbdev" >&2
        exec "\${FBDEV}" "\$@"
    fi
    exit "\${status}"
fi

exec "\${FBDEV}" "\$@"
EOF
    chmod 755 "${PKG_STAGE}/${BIN_NAME}"
elif [[ "${RAYLIB_PLATFORM}" == "DRM" || "${RAYLIB_PLATFORM}" == "Desktop" || "${RAYLIB_PLATFORM}" == "FBDEV" ]]; then
    configure_and_build "${RAYLIB_PLATFORM}" "${BUILD_DIR}"
    PACKAGE_VERSION="$(read_cmake_cache_value CMAKE_PROJECT_VERSION)"
    cp "${ROOT_DIR}/dist/${BIN_NAME}" "${PKG_STAGE}/${BIN_NAME}"
else
    cat >&2 <<EOF
Unsupported RIDGEDASH_PACKAGE_PLATFORM=${RAYLIB_PLATFORM}
Expected AUTO, DRM, FBDEV, or Desktop.
EOF
    exit 1
fi

EXECUTABLE="${PKG_STAGE}/${BIN_NAME}"
DESKTOP_TEMPLATE="${SCRIPT_DIR}/ridgedash.desktop.in"
ICON_FILE="${SCRIPT_DIR}/images/ridgedash.png"

for path in "${EXECUTABLE}" "${DESKTOP_TEMPLATE}" "${ICON_FILE}"; do
    if [[ ! -f "${path}" ]]; then
        echo "Required file not found: ${path}" >&2
        exit 1
    fi
done

rm -rf "${STAGE_DIR}"
mkdir -p \
    "${STAGE_DIR}/DEBIAN" \
    "${STAGE_DIR}/usr/share/APPLaunch/bin" \
    "${STAGE_DIR}/usr/share/APPLaunch/applications" \
    "${STAGE_DIR}/usr/share/APPLaunch/share/ridgedash" \
    "${STAGE_DIR}/usr/share/APPLaunch/share/images" \
    "${DIST_DIR}"

install -m 755 "${EXECUTABLE}" "${STAGE_DIR}/usr/share/APPLaunch/bin/${BIN_NAME}"
if [[ "${RAYLIB_PLATFORM}" == "AUTO" ]]; then
    install -m 755 "${PKG_STAGE}/${BIN_NAME}.drm" "${STAGE_DIR}/usr/share/APPLaunch/bin/${BIN_NAME}.drm"
    install -m 755 "${PKG_STAGE}/${BIN_NAME}.fbdev" "${STAGE_DIR}/usr/share/APPLaunch/bin/${BIN_NAME}.fbdev"
fi
install -m 644 "${DESKTOP_TEMPLATE}" "${STAGE_DIR}/usr/share/APPLaunch/applications/ridgedash.desktop"
install -m 644 "${ICON_FILE}" "${STAGE_DIR}/usr/share/APPLaunch/share/images/ridgedash.png"
cp -R "${ROOT_DIR}/assets/sprites" "${STAGE_DIR}/usr/share/APPLaunch/share/ridgedash/sprites"
cp -R "${ROOT_DIR}/assets/audio" "${STAGE_DIR}/usr/share/APPLaunch/share/ridgedash/audio"

INSTALLED_SIZE="$(du -sk "${STAGE_DIR}/usr" | awk '{print $1}')"
cat >"${STAGE_DIR}/DEBIAN/control" <<EOF
Package: ${PACKAGE_NAME}
Version: ${PACKAGE_VERSION}
Section: games
Priority: optional
Architecture: ${DEB_ARCH}
Maintainer: ${MAINTAINER}
Depends: libc6, libstdc++6, libgcc-s1$(if [[ "${RAYLIB_PLATFORM}" == "AUTO" || "${RAYLIB_PLATFORM}" == "DRM" || "${RAYLIB_PLATFORM}" == "Desktop" ]]; then printf ", libdrm2, libgbm1, libegl1, libgles2"; fi)
Installed-Size: ${INSTALLED_SIZE}
Description: RidgeDash game for M5CardputerZero APPLaunch
 Raylib and Box2D hill driving game prototype with launcher entry.
EOF

DEB_PATH="${DIST_DIR}/${PACKAGE_NAME}_${PACKAGE_VERSION}_${PACKAGE_SUFFIX}_${DEB_ARCH}.deb"
dpkg-deb --build --root-owner-group "${STAGE_DIR}" "${DEB_PATH}"

echo "Generated Debian package: ${DEB_PATH}"
