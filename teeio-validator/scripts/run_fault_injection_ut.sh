#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
validator_dir=$(cd -- "${script_dir}/.." && pwd)
repo_dir=$(cd -- "${validator_dir}/.." && pwd)
gtest_dir="${repo_dir}/third_party/googletest/googletest"
build_dir=$(mktemp -d)
trap 'rm -rf "${build_dir}"' EXIT

if [[ ! -f "${gtest_dir}/src/gtest-all.cc" ]]; then
    printf 'GoogleTest sources not found at %s\n' "${gtest_dir}" >&2
    printf 'Run git submodule update --init third_party/googletest.\n' >&2
    exit 1
fi

"${CC:-cc}" \
    -std=c99 -Wall -Wextra -Werror \
    -I"${validator_dir}/include" \
    -c "${validator_dir}/library/helperlib/fault_injection.c" \
    -o "${build_dir}/fault_injection.o"

"${CXX:-c++}" \
    -std=c++17 -Wall -Wextra -Werror -pthread \
    -I"${validator_dir}/include" \
    -I"${gtest_dir}" \
    -I"${gtest_dir}/include" \
    "${validator_dir}/test/host_fault_injection/fault_injection_test.cpp" \
    "${gtest_dir}/src/gtest-all.cc" \
    "${gtest_dir}/src/gtest_main.cc" \
    "${build_dir}/fault_injection.o" \
    -o "${build_dir}/fault_injection_test"

"${build_dir}/fault_injection_test"