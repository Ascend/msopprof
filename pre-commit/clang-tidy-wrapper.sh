#!/usr/bin/env bash
# =============================================================================
# clang-tidy wrapper — checks compile_commands.json before running clang-tidy
# =============================================================================
set -euo pipefail

BUILD_DIRS=("build")

for dir in "${BUILD_DIRS[@]}"; do
    if [[ -f "${dir}/compile_commands.json" ]]; then
        break
    fi
done

if [[ ! -f "${dir}/compile_commands.json" ]]; then
    cat >&2 <<'EOF'
=======================================================================
  ERROR: compile_commands.json NOT FOUND
-----------------------------------------------------------------------
  clang-tidy requires a compilation database to work.
  Run this command to generate compile_commands.json:

    python3 build.py

  This will generate compile_commands.json under the build/ directory.
=======================================================================
EOF
    exit 1
fi

exec clang-tidy "$@"
