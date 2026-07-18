#!/usr/bin/env bash
# Build RidgeDash with an existing devkitPro 3DS toolchain and create a
# versioned .3dsx release artifact.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
source "${ROOT_DIR}/packaging/package_helpers.sh"

VERSION="$(ridgedash_project_version "${ROOT_DIR}/CMakeLists.txt")"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build/3ds}"
DIST_DIR="${DIST_DIR:-${ROOT_DIR}/dist/artifacts}"
CMAKE_BIN="${CMAKE:-cmake}"
PARALLEL="${PARALLEL:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
DEVKITPRO_DIR="${DEVKITPRO:-/opt/devkitpro}"
TOOLCHAIN_FILE="${DEVKITPRO_DIR}/cmake/3DS.cmake"

for tool in "${CMAKE_BIN}" tex3ds; do
    command -v "${tool}" >/dev/null 2>&1 || {
        echo "Required 3DS build tool not found: ${tool}" >&2
        exit 1
    }
done
[[ -f "${TOOLCHAIN_FILE}" ]] || {
    echo "3DS CMake toolchain not found: ${TOOLCHAIN_FILE}" >&2
    exit 1
}

"${CMAKE_BIN}" -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
    -DRIDGEDASH_RAYLIB_PLATFORM=3DS \
    -DRIDGEDASH_COPY_TO_DIST=OFF \
    -DCMAKE_BUILD_TYPE=Release
"${CMAKE_BIN}" --build "${BUILD_DIR}" -j"${PARALLEL}"

BUILT_APP="${BUILD_DIR}/runtime/RidgeDash.3dsx"
ARTIFACT="${DIST_DIR}/RidgeDash-${VERSION}-3ds.3dsx"
[[ -f "${BUILT_APP}" ]] || {
    echo "Built 3DS application missing: ${BUILT_APP}" >&2
    exit 1
}

mkdir -p "${DIST_DIR}"
install -m 644 "${BUILT_APP}" "${ARTIFACT}"
echo "Packaged ${ARTIFACT}"
