#!/usr/bin/env bash
# Cross-compile RidgeDash for Windows (x86_64) from Linux using MinGW-w64,
# then package into dist/artifacts/ as a portable .zip.
# Unpack anywhere on Windows and double-click RidgeDash.exe to play.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BIN_NAME="RidgeDash"
APP_NAME="RidgeDash"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build/windows}"
DIST_DIR="${DIST_DIR:-${ROOT_DIR}/dist/artifacts}"
PKG_STAGE="${PKG_STAGE:-${ROOT_DIR}/build/windows-pkg}"
CMAKE_BIN="${CMAKE:-cmake}"
CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"
PARALLEL="${PARALLEL:-$(nproc 2>/dev/null || echo 4)}"
TOOLCHAIN_FILE="${ROOT_DIR}/cmake/x86_64-w64-mingw32.cmake"

for tool in "${CMAKE_BIN}" x86_64-w64-mingw32-gcc x86_64-w64-mingw32-g++ zip; do
    command -v "${tool}" >/dev/null 2>&1 || { echo "Required tool not found: ${tool}" >&2; exit 1; }
done

VERSION="$(grep -E 'project\(RidgeDash VERSION' "${ROOT_DIR}/CMakeLists.txt" | sed -E 's/.*VERSION ([0-9.]+).*/\1/')"
VERSION="${VERSION:-0.0.0}"

# 1. Cross-compile for Windows (produces dist/RidgeDash + dist/assets).
"${CMAKE_BIN}" -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
    -DRIDGEDASH_RAYLIB_PLATFORM=Desktop \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}"
"${CMAKE_BIN}" --build "${BUILD_DIR}" -j"${PARALLEL}"

# The dist target copies $<TARGET_FILE> (which is RidgeDash.exe) to dist/RidgeDash
# (no .exe suffix).  We rename it back to RidgeDash.exe in the package.
BUILT_BIN="${ROOT_DIR}/dist/${BIN_NAME}"
BUILT_ASSETS="${ROOT_DIR}/dist/assets"
[[ -f "${BUILT_BIN}" ]] || { echo "Built binary missing: ${BUILT_BIN}" >&2; exit 1; }
[[ -d "${BUILT_ASSETS}" ]] || { echo "Built assets missing: ${BUILT_ASSETS}" >&2; exit 1; }

# 2. Assemble the portable package directory.
rm -rf "${PKG_STAGE}"
mkdir -p "${PKG_STAGE}/${APP_NAME}"

install -m 755 "${BUILT_BIN}" "${PKG_STAGE}/${APP_NAME}/${BIN_NAME}.exe"
cp -R "${BUILT_ASSETS}" "${PKG_STAGE}/${APP_NAME}/assets"

ICON_FILE="${SCRIPT_DIR}/images/ridgedash.png"
if [[ -f "${ICON_FILE}" ]]; then
    install -m 644 "${ICON_FILE}" "${PKG_STAGE}/${APP_NAME}/icon.png"
fi

# 3. Zip into dist/artifacts/.
mkdir -p "${DIST_DIR}"
ZIP_PATH="${DIST_DIR}/${APP_NAME}-windows-x86_64.zip"
rm -f "${ZIP_PATH}"
( cd "${PKG_STAGE}" && zip -rq "${ZIP_PATH}" "${APP_NAME}" )

echo "Packaged ${ZIP_PATH}"
