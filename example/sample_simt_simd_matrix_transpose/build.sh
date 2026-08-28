#!/usr/bin/env bash
set -euo pipefail

ARCH="dav-3510"
RUN_MODE="npu"
PROFILE="2"

for arg in "$@"; do
    case "${arg}" in
        0|1|2)
            PROFILE="${arg}"
            ;;
        case0|case1|case2)
            PROFILE="${arg#case}"
            ;;
        dav-3510)
            ARCH="${arg}"
            ;;
        npu|sim)
            RUN_MODE="${arg}"
            ;;
        *)
            echo "Usage: $0 [0|1|2] [dav-3510] [npu|sim]" >&2
            exit 1
            ;;
    esac
done

BUILD_DIR="build/case${PROFILE}_${RUN_MODE}_${ARCH}"

echo "[INFO] Configure matrix_transpose_case${PROFILE}: case=${PROFILE}, run_mode=${RUN_MODE}, arch=${ARCH}"
cmake -S . -B "${BUILD_DIR}" \
    -DCMAKE_ASC_RUN_MODE="${RUN_MODE}" \
    -DCMAKE_ASC_ARCHITECTURES="${ARCH}" \
    -DSCENARIO_NUM="${PROFILE}"
cmake --build "${BUILD_DIR}" -j

echo "[INFO] Built ${BUILD_DIR}/matrix_transpose_case${PROFILE}"
