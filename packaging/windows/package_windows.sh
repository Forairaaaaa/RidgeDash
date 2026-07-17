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
MINGW_CC="x86_64-w64-mingw32-gcc-posix"
MINGW_CXX="x86_64-w64-mingw32-g++-posix"

for tool in "${CMAKE_BIN}" "${MINGW_CC}" "${MINGW_CXX}" x86_64-w64-mingw32-objdump zip; do
    command -v "${tool}" >/dev/null 2>&1 || { echo "Required tool not found: ${tool}" >&2; exit 1; }
done

# 1. Cross-compile for Windows.
"${CMAKE_BIN}" -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
    -DRIDGEDASH_RAYLIB_PLATFORM=Desktop \
    -DRIDGEDASH_COPY_TO_DIST=OFF \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}"
"${CMAKE_BIN}" --build "${BUILD_DIR}" -j"${PARALLEL}"

BUILT_BIN="${BUILD_DIR}/runtime/${BIN_NAME}.exe"
BUILT_ASSETS="${ROOT_DIR}/assets"
[[ -f "${BUILT_BIN}" ]] || { echo "Built binary missing: ${BUILT_BIN}" >&2; exit 1; }
[[ -d "${BUILT_ASSETS}" ]] || { echo "Built assets missing: ${BUILT_ASSETS}" >&2; exit 1; }

# 2. Assemble the portable package directory.
rm -rf "${PKG_STAGE}"
mkdir -p "${PKG_STAGE}/${APP_NAME}"

install -m 755 "${BUILT_BIN}" "${PKG_STAGE}/${APP_NAME}/${BIN_NAME}.exe"
cp -R "${BUILT_ASSETS}" "${PKG_STAGE}/${APP_NAME}/assets"
"${CMAKE_BIN}" \
    -DDIST_ASSETS="${PKG_STAGE}/${APP_NAME}/assets" \
    -P "${ROOT_DIR}/cmake/strip_dist_assets.cmake"

# Include MinGW runtime DLLs if the selected compiler variant still links any
# dynamically (for example libwinpthread-1.dll).
DLL_IMPORTS="$(x86_64-w64-mingw32-objdump -p "${BUILT_BIN}" | sed -n 's/^[[:space:]]*DLL Name: //p')"
for dll in libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
    if grep -Fqi "${dll}" <<<"${DLL_IMPORTS}"; then
        dll_path="$("${MINGW_CXX}" -print-file-name="${dll}")"
        [[ -f "${dll_path}" ]] || { echo "Required MinGW runtime DLL not found: ${dll}" >&2; exit 1; }
        install -m 755 "${dll_path}" "${PKG_STAGE}/${APP_NAME}/${dll}"
    fi
done

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
