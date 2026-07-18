#!/usr/bin/env bash
# Build RidgeDash and launch it through Homebrew Launcher's 3dslink NetLoader.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "${ROOT_DIR}/packaging/package_helpers.sh"

TARGET_IP="${1:-${RIDGEDASH_3DS_IP:-}}"
CONTAINER_BIN="${CONTAINER:-container}"
IMAGE="${RIDGEDASH_3DS_IMAGE:-docker.io/devkitpro/devkitarm:20260610}"
VERSION="$(ridgedash_project_version "${ROOT_DIR}/CMakeLists.txt")"
ARTIFACT="dist/artifacts/RidgeDash-${VERSION}-3ds.3dsx"

if [[ -z "${TARGET_IP}" ]]; then
    echo "Usage: $0 <3ds-ip>" >&2
    echo "Or set RIDGEDASH_3DS_IP in your shell." >&2
    exit 2
fi

"${ROOT_DIR}/packaging/3ds/package_3ds_container.sh"

exec "${CONTAINER_BIN}" run --rm \
    --arch amd64 \
    --volume "${ROOT_DIR}:/workspace" \
    --workdir /workspace \
    "${IMAGE}" \
    3dslink --address "${TARGET_IP}" --server "${ARTIFACT}"
