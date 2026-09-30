#!/usr/bin/env bash
# Runs every scenario INI and requires exit status 0; per-scenario output and
# artifacts are kept. Outcome classification and recovery belong to the caller.

set -euo pipefail

if [[ $# -lt 2 || $# -gt 3 ]]; then
    printf 'Usage: %s <teeio_validator> <scenario_directory> [log_directory]\n' "$0" >&2
    exit 2
fi

binary=$(realpath "$1")
scenario_dir=$(realpath "$2")
log_dir=${3:-$(mktemp -d)}
mkdir -p "$log_dir"
log_dir=$(realpath "$log_dir")

mapfile -t scenario_files < <(find "$scenario_dir" -maxdepth 1 -type f -name '*.ini' | sort)
if [[ ${#scenario_files[@]} -eq 0 ]]; then
    printf 'No scenario files found in %s\n' "$scenario_dir" >&2
    exit 1
fi

failed=0
blocked=0
for scenario_file in "${scenario_files[@]}"; do
    name=$(basename "$scenario_file" .ini)
    if grep -Eq '^[[:space:]]*runnable[[:space:]]*=[[:space:]]*0[[:space:]]*([;#].*)?$' "$scenario_file"; then
        printf 'Skipped blocked scenario %s\n' "$name"
        blocked=$((blocked + 1))
        continue
    fi
    driver=$(awk -F= '/^[[:space:]]*driver[[:space:]]*=/{gsub(/[[:space:]]/, "", $2); print $2; exit}' "$scenario_file")
    driver=${driver:-Version.1}
    run_dir="${log_dir}/${name}"
    mkdir -p "$run_dir"
    set +e
    (cd "$run_dir" && "$binary" -f "$scenario_file" -t 1 -c 1 -s "$driver" -l verbose) \
        > "${run_dir}/output.log" 2>&1
    status=$?
    set -e
    if [[ $status -ne 0 ]]; then
        printf 'Scenario %s failed (exit %s); log: %s\n' \
            "$name" "$status" "${run_dir}/output.log" >&2
        failed=$((failed + 1))
    fi
done

printf 'Ran %s scenario files (%s blocked, %s failed); logs: %s\n' \
    "$((${#scenario_files[@]} - blocked))" "$blocked" "$failed" "$log_dir"
[[ $failed -eq 0 ]]
