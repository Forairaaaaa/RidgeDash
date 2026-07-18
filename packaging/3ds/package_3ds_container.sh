#!/usr/bin/env bash
# Build the 3DS release artifact without installing devkitPro on the host.
# Requires Apple Container on Apple silicon with macOS 26 or later.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
CONTAINER_BIN="${CONTAINER:-container}"
IMAGE="${RIDGEDASH_3DS_IMAGE:-docker.io/devkitpro/devkitarm:20260610}"
CPUS="${RIDGEDASH_3DS_CPUS:-6}"
MEMORY="${RIDGEDASH_3DS_MEMORY:-6g}"

command -v "${CONTAINER_BIN}" >/dev/null 2>&1 || {
    echo "Apple Container is not installed: https://github.com/apple/container" >&2
    exit 1
}

if ! "${CONTAINER_BIN}" system status >/dev/null 2>&1; then
    echo "Apple Container service is not running. Start it with: container system start" >&2
    exit 1
fi

"${CONTAINER_BIN}" run \
    --arch amd64 \
    --rm \
    --cpus "${CPUS}" \
    --memory "${MEMORY}" \
    --volume "${ROOT_DIR}:/workspace" \
    --workdir /workspace \
    "${IMAGE}" \
    bash -lc packaging/3ds/package_3ds.sh
