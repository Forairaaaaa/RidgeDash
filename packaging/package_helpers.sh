#!/usr/bin/env bash

# Read the canonical release version from the top-level CMake project.
ridgedash_project_version() {
    local cmake_file="$1"
    local version

    version="$(sed -nE 's/^project\(RidgeDash VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' "${cmake_file}" | head -n 1)"
    if [[ -z "${version}" ]]; then
        echo "Could not read RidgeDash version from ${cmake_file}" >&2
        return 1
    fi

    printf '%s\n' "${version}"
}
