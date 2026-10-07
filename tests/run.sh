#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
test_build="$(mktemp -d)"
trap 'rm -rf "$test_build"' EXIT
g++ -std=c++17 -Wall -Wextra -Wno-misleading-indentation \
  -I "$repo_root/tests/stubs" -I "$repo_root/firmware/Nerd_Clock" \
  "$repo_root/tests/test_clock.cpp" "$repo_root"/firmware/Nerd_Clock/*.cpp -o "$test_build/test_clock"
"$test_build/test_clock" > "$test_build/payloads.jsonl"
python3 "$repo_root/tests/validate.py" "$test_build/payloads.jsonl" "$repo_root/firmware/Nerd_Clock/WebUi.h"

python3 "$repo_root/tests/test_ota_package.py"
