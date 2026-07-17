#!/usr/bin/env bash
# Build RidgeDash for desktop Linux and package into dist/artifacts/.
# Produces a self-contained .tar.gz — unpack anywhere and run ./RidgeDash.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BIN_NAME="RidgeDash"
APP_NAME="RidgeDash"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build/desktop}"
DIST_DIR="${DIST_DIR:-${ROOT_DIR}/dist/artifacts}"
PKG_STAGE="${PKG_STAGE:-${ROOT_DIR}/build/linux-pkg}"
CMAKE_BIN="${CMAKE:-cmake}"
CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"
PARALLEL="${PARALLEL:-$(nproc 2>/dev/null || echo 4)}"

for tool in "${CMAKE_BIN}" tar; do
    command -v "${tool}" >/dev/null 2>&1 || { echo "Required tool not found: ${tool}" >&2; exit 1; }
done

VERSION="$(grep -E 'project\(RidgeDash VERSION' "${ROOT_DIR}/CMakeLists.txt" | sed -E 's/.*VERSION ([0-9.]+).*/\1/')"
VERSION="${VERSION:-0.0.0}"

ARCH="$(uname -m)"
case "${ARCH}" in
    x86_64)   ARCH="x86_64" ;;
    aarch64)  ARCH="arm64" ;;
    arm64)    ARCH="arm64" ;;
    *)        echo "Unsupported architecture: ${ARCH}" >&2; exit 1 ;;
esac

# 1. Build the desktop binary (produces dist/RidgeDash + dist/assets).
"${CMAKE_BIN}" -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
    -DRIDGEDASH_RAYLIB_PLATFORM=Desktop
"${CMAKE_BIN}" --build "${BUILD_DIR}" -j"${PARALLEL}"

BUILT_BIN="${ROOT_DIR}/dist/${BIN_NAME}"
BUILT_ASSETS="${ROOT_DIR}/dist/assets"
[[ -f "${BUILT_BIN}" ]] || { echo "Built binary missing: ${BUILT_BIN}" >&2; exit 1; }
[[ -d "${BUILT_ASSETS}" ]] || { echo "Built assets missing: ${BUILT_ASSETS}" >&2; exit 1; }

# 2. Assemble the self-contained package directory.
rm -rf "${PKG_STAGE}"
mkdir -p "${PKG_STAGE}/${APP_NAME}"

install -m 755 "${BUILT_BIN}" "${PKG_STAGE}/${APP_NAME}/${BIN_NAME}"
cp -R "${BUILT_ASSETS}" "${PKG_STAGE}/${APP_NAME}/assets"

ICON_FILE="${SCRIPT_DIR}/images/ridgedash.png"
if [[ -f "${ICON_FILE}" ]]; then
    install -m 644 "${ICON_FILE}" "${PKG_STAGE}/${APP_NAME}/icon.png"
fi

# 3. .desktop entry so users can integrate with their launcher.
cat >"${PKG_STAGE}/${APP_NAME}/${APP_NAME}.desktop" <<EOF
[Desktop Entry]
Name=${APP_NAME}
Exec=./${BIN_NAME}
Icon=icon
Terminal=false
Type=Application
Categories=Game;ArcadeGame;
EOF

# 4. Tar.gz into dist/artifacts/.
mkdir -p "${DIST_DIR}"
TAR_PATH="${DIST_DIR}/${APP_NAME}-linux-${ARCH}.tar.gz"
rm -f "${TAR_PATH}"
tar -czf "${TAR_PATH}" -C "${PKG_STAGE}" "${APP_NAME}"

echo "Packaged ${TAR_PATH}"
