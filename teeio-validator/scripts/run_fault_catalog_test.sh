#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 2 ]]; then
    printf 'Usage: %s <teeio_validator> <scenario_directory>\n' "$0" >&2
    exit 2
fi

binary=$(realpath "$1")
scenario_dir=$(realpath "$2")
work_dir=$(mktemp -d)
trap 'rm -rf "${work_dir}"' EXIT

mapfile -t scenario_files < <(find "$scenario_dir" -maxdepth 1 -type f -name '*.ini' | sort)
if [[ ${#scenario_files[@]} -eq 0 ]]; then
    printf 'No scenario files found in %s\n' "$scenario_dir" >&2
    exit 1
fi

for scenario_file in "${scenario_files[@]}"; do
    driver=$(awk -F= '/^[[:space:]]*driver[[:space:]]*=/{gsub(/[[:space:]]/, "", $2); print $2; exit}' "$scenario_file")
    driver=${driver:-Version.1}
    set +e
    output=$(cd "$work_dir"; "$binary" -f "$scenario_file" -t 1 -c 1 -s "$driver" -l verbose 2>&1)
    status=$?
    set -e
    if [[ $status -ne 0 && $status -ne 255 ]]; then
        printf 'Validator failed unexpectedly for %s (exit %s)\n%s\n' \
            "$scenario_file" "$status" "$output" >&2
        exit 1
    fi
done

printf 'Ran all %s scenario files\n' "${#scenario_files[@]}"
