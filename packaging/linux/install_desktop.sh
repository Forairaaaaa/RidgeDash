#!/usr/bin/env bash
# Install or remove a per-user launcher entry for this unpacked RidgeDash directory.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -n "${XDG_DATA_HOME:-}" ]]; then
    DATA_HOME="${XDG_DATA_HOME}"
elif [[ -n "${HOME:-}" ]]; then
    DATA_HOME="${HOME}/.local/share"
else
    echo "Neither XDG_DATA_HOME nor HOME is set" >&2
    exit 1
fi
DESKTOP_FILE="${DATA_HOME}/applications/net.forairaaaaa.ridgedash.desktop"

if [[ "${1:-}" == "--uninstall" ]]; then
    rm -f "${DESKTOP_FILE}"
    echo "Removed RidgeDash launcher"
    exit 0
fi

for path in "${SCRIPT_DIR}/RidgeDash" "${SCRIPT_DIR}/icon.png"; do
    [[ -f "${path}" ]] || { echo "Required file not found: ${path}" >&2; exit 1; }
done

mkdir -p "$(dirname "${DESKTOP_FILE}")"

cat >"${DESKTOP_FILE}" <<EOF
[Desktop Entry]
Name=RidgeDash
Comment=Retro side-scrolling driving game
Exec="${SCRIPT_DIR}/RidgeDash"
Icon=${SCRIPT_DIR}/icon.png
Terminal=false
Type=Application
Categories=Game;ArcadeGame;
EOF

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$(dirname "${DESKTOP_FILE}")" >/dev/null 2>&1 || true
fi

echo "Installed RidgeDash launcher: ${DESKTOP_FILE}"
echo "If you move this directory, run install_desktop.sh again."
