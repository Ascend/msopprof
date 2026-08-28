#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ARCH="dav-3510"
RUN_MODE="npu"
CASE="3"

for arg in "$@"; do
    case "${arg}" in
        0|1|2|3)
            CASE="${arg}"
            ;;
        case0|case1|case2|case3)
            CASE="${arg#case}"
            ;;
        all)
            CASE="all"
            ;;
        dav-3510)
            ARCH="${arg}"
            ;;
        npu|sim)
            RUN_MODE="${arg}"
            ;;
        *)
            echo "Usage: $0 [0|1|2|3|all] [dav-3510] [npu|sim]" >&2
            exit 1
            ;;
    esac
done

build_case() {
    local case_id="$1"
    local build_dir="${SCRIPT_DIR}/build/case${case_id}_${RUN_MODE}_${ARCH}"

    echo "[INFO] Configure fused_bias_silu_case${case_id}: run_mode=${RUN_MODE}, arch=${ARCH}"
    cmake -S "${SCRIPT_DIR}" -B "${build_dir}" \
        -DCMAKE_ASC_RUN_MODE="${RUN_MODE}" \
        -DCMAKE_ASC_ARCHITECTURES="${ARCH}" \
        -DSCENARIO_NUM="${case_id}"
    cmake --build "${build_dir}" -j
    echo "[INFO] Built ${build_dir}/fused_bias_silu_case${case_id}"
}

if [[ "${CASE}" == "all" ]]; then
    for case_id in 0 1 2 3; do
        build_case "${case_id}"
    done
else
    build_case "${CASE}"
fi
