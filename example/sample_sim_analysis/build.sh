#!/usr/bin/env bash
set -euo pipefail

ARCH="dav-3510"
RUN_MODE="sim"
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
