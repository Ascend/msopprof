#!/usr/bin/env bash
# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------
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
