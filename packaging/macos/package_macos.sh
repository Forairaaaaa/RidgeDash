#!/usr/bin/env bash
# Build RidgeDash.app for macOS (Apple Silicon) and zip it into dist/artifacts/.
# No code signing — the app is ad-hoc; first launch needs right-click -> Open.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BIN_NAME="RidgeDash"
APP_NAME="RidgeDash"
BUNDLE_ID="net.forairaaaaa.ridgedash"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build/macos}"
DIST_DIR="${DIST_DIR:-${ROOT_DIR}/dist/artifacts}"
APP_STAGE="${APP_STAGE:-${ROOT_DIR}/build/macos-app}"
CMAKE_BIN="${CMAKE:-cmake}"
CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"
PARALLEL="${PARALLEL:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This script must run on macOS (needs sips/iconutil)." >&2
    exit 1
fi
for tool in "${CMAKE_BIN}" sips iconutil ditto; do
    command -v "${tool}" >/dev/null 2>&1 || { echo "Required tool not found: ${tool}" >&2; exit 1; }
done

VERSION="$(grep -E 'project\(RidgeDash VERSION' "${ROOT_DIR}/CMakeLists.txt" | sed -E 's/.*VERSION ([0-9.]+).*/\1/')"
VERSION="${VERSION:-0.0.0}"

# 1. Build the desktop binary (produces dist/RidgeDash + dist/assets).
"${CMAKE_BIN}" -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
    -DRIDGEDASH_RAYLIB_PLATFORM=Desktop
"${CMAKE_BIN}" --build "${BUILD_DIR}" -j"${PARALLEL}"

BUILT_BIN="${ROOT_DIR}/dist/${BIN_NAME}"
BUILT_ASSETS="${ROOT_DIR}/dist/assets"
[[ -f "${BUILT_BIN}" ]] || { echo "Built binary missing: ${BUILT_BIN}" >&2; exit 1; }
[[ -d "${BUILT_ASSETS}" ]] || { echo "Built assets missing: ${BUILT_ASSETS}" >&2; exit 1; }

# 2. Assemble the .app bundle.
APP="${APP_STAGE}/${APP_NAME}.app"
rm -rf "${APP_STAGE}"
mkdir -p "${APP}/Contents/MacOS" "${APP}/Contents/Resources"

install -m 755 "${BUILT_BIN}" "${APP}/Contents/MacOS/${BIN_NAME}"
ditto "${BUILT_ASSETS}" "${APP}/Contents/Resources/assets"

# 3. Icon: PNG -> .icns via an iconset.
ICONSET="${APP_STAGE}/${APP_NAME}.iconset"
rm -rf "${ICONSET}"; mkdir -p "${ICONSET}"
SRC_ICON="${SCRIPT_DIR}/icon.png"
for size in 16 32 64 128 256 512; do
    sips -z "${size}" "${size}" "${SRC_ICON}" --out "${ICONSET}/icon_${size}x${size}.png" >/dev/null
    double=$((size * 2))
    sips -z "${double}" "${double}" "${SRC_ICON}" --out "${ICONSET}/icon_${size}x${size}@2x.png" >/dev/null
done
iconutil -c icns "${ICONSET}" -o "${APP}/Contents/Resources/${APP_NAME}.icns"
rm -rf "${ICONSET}"

# 4. Info.plist.
cat >"${APP}/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key><string>${APP_NAME}</string>
    <key>CFBundleDisplayName</key><string>${APP_NAME}</string>
    <key>CFBundleIdentifier</key><string>${BUNDLE_ID}</string>
    <key>CFBundleVersion</key><string>${VERSION}</string>
    <key>CFBundleShortVersionString</key><string>${VERSION}</string>
    <key>CFBundleExecutable</key><string>${BIN_NAME}</string>
    <key>CFBundleIconFile</key><string>${APP_NAME}</string>
    <key>CFBundlePackageType</key><string>APPL</string>
    <key>LSMinimumSystemVersion</key><string>11.0</string>
    <key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
EOF

# 5. Zip into dist/artifacts/ (ditto preserves the bundle + exec bits).
mkdir -p "${DIST_DIR}"
ZIP_PATH="${DIST_DIR}/${APP_NAME}-macos-arm64.zip"
rm -f "${ZIP_PATH}"
( cd "${APP_STAGE}" && ditto -c -k --sequesterRsrc --keepParent "${APP_NAME}.app" "${ZIP_PATH}" )

echo "Built ${APP}"
echo "Packaged ${ZIP_PATH}"
