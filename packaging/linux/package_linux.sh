#!/usr/bin/env bash
# Build RidgeDash for desktop Linux and package into dist/artifacts/.
# Produces a portable directory archive — unpack it and run ./RidgeDash.
# The executable still relies on the target Linux system's glibc, graphics, and
# audio libraries; build on the oldest distribution you intend to support.
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

for tool in "${CMAKE_BIN}" tar ldd readelf; do
    command -v "${tool}" >/dev/null 2>&1 || { echo "Required tool not found: ${tool}" >&2; exit 1; }
done

ARCH="$(uname -m)"
case "${ARCH}" in
    x86_64)   ARCH="x86_64" ;;
    aarch64)  ARCH="arm64" ;;
    arm64)    ARCH="arm64" ;;
    *)        echo "Unsupported architecture: ${ARCH}" >&2; exit 1 ;;
esac

# 1. Build the desktop binary.
"${CMAKE_BIN}" -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
    -DRIDGEDASH_RAYLIB_PLATFORM=Desktop \
    -DRIDGEDASH_COPY_TO_DIST=OFF
"${CMAKE_BIN}" --build "${BUILD_DIR}" -j"${PARALLEL}"

BUILT_BIN="${BUILD_DIR}/runtime/${BIN_NAME}"
BUILT_ASSETS="${ROOT_DIR}/assets"
[[ -f "${BUILT_BIN}" ]] || { echo "Built binary missing: ${BUILT_BIN}" >&2; exit 1; }
[[ -d "${BUILT_ASSETS}" ]] || { echo "Built assets missing: ${BUILT_ASSETS}" >&2; exit 1; }

if ldd "${BUILT_BIN}" 2>&1 | grep -q 'not found'; then
    echo "Linux runtime dependency check failed:" >&2
    ldd "${BUILT_BIN}" >&2
    exit 1
fi
GLIBC_REQUIRED="$(readelf --version-info "${BUILT_BIN}" \
    | sed -n 's/.*Name: GLIBC_\([0-9][0-9.]*\).*/\1/p' \
    | sort -Vu \
    | tail -n 1)"

# 2. Assemble the portable package directory.
rm -rf "${PKG_STAGE}"
mkdir -p "${PKG_STAGE}/${APP_NAME}"

install -m 755 "${BUILT_BIN}" "${PKG_STAGE}/${APP_NAME}/${BIN_NAME}"
cp -R "${BUILT_ASSETS}" "${PKG_STAGE}/${APP_NAME}/assets"
"${CMAKE_BIN}" \
    -DDIST_ASSETS="${PKG_STAGE}/${APP_NAME}/assets" \
    -P "${ROOT_DIR}/cmake/strip_dist_assets.cmake"

ICON_FILE="${SCRIPT_DIR}/images/ridgedash.png"
if [[ -f "${ICON_FILE}" ]]; then
    install -m 644 "${ICON_FILE}" "${PKG_STAGE}/${APP_NAME}/icon.png"
fi

# 3. Install a launcher helper that resolves this package's absolute path.
install -m 755 "${SCRIPT_DIR}/install_desktop.sh" "${PKG_STAGE}/${APP_NAME}/install_desktop.sh"

# 4. Tar.gz into dist/artifacts/.
mkdir -p "${DIST_DIR}"
TAR_PATH="${DIST_DIR}/${APP_NAME}-linux-${ARCH}.tar.gz"
rm -f "${TAR_PATH}"
tar -czf "${TAR_PATH}" -C "${PKG_STAGE}" "${APP_NAME}"

echo "Packaged ${TAR_PATH}"
if [[ -n "${GLIBC_REQUIRED}" ]]; then
    echo "Highest required glibc version: ${GLIBC_REQUIRED}"
fi
