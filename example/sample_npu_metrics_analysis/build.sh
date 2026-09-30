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
set -euo pipefail

ARCH="dav-3510"
RUN_MODE="npu"
PROFILE="after"

for arg in "$@"; do
    case "${arg}" in
        before|after)
            PROFILE="${arg}"
            ;;
        dav-2201|dav-3510)
            ARCH="${arg}"
            ;;
        npu|sim)
            RUN_MODE="${arg}"
            ;;
        *)
            echo "Usage: $0 [before|after] [dav-2201|dav-3510] [npu|sim]" >&2
            exit 1
            ;;
    esac
done

BUILD_DIR="build/${PROFILE}_${RUN_MODE}_${ARCH}"

echo "[INFO] Configure grouped_matmul_${PROFILE}: profile=${PROFILE}, run_mode=${RUN_MODE}, arch=${ARCH}"
cmake -S . -B "${BUILD_DIR}" \
    -DCMAKE_ASC_RUN_MODE="${RUN_MODE}" \
    -DCMAKE_ASC_ARCHITECTURES="${ARCH}" \
    -DGROUPED_MATMUL_PROFILE="${PROFILE}"
cmake --build "${BUILD_DIR}" -j

echo "[INFO] Built ${BUILD_DIR}/grouped_matmul_${PROFILE}"
