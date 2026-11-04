#!/usr/bin/python3

# ----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in this case directory for the full text of the License.
# ----------------------------------------------------------------------------------------------------------


import argparse
import os

import numpy as np

M = 16
N = 8192
ROWS_PER_CHUNK = 16
RANDOM_SEED = 20260806


def fused_compute(input_x, input_y, fused_iterations):
    result = (input_x + input_y).astype(np.float16)
    for _ in range(fused_iterations):
        result = (result * input_x).astype(np.float16)
        result = (result + input_y).astype(np.float16)
    return result


def gen_golden_data(fused_iterations):
    os.makedirs("input", exist_ok=True)
    os.makedirs("output", exist_ok=True)
    rng = np.random.default_rng(RANDOM_SEED)

    with open("./input/input_x.bin", "wb") as input_x_file, open("./input/input_y.bin", "wb") as input_y_file, open(
        "./output/golden.bin", "wb"
    ) as golden_file:
        for row_start in range(0, M, ROWS_PER_CHUNK):
            row_count = min(ROWS_PER_CHUNK, M - row_start)
            chunk_shape = (row_count, N)
            # 将输入限制在较小范围内，避免多轮half乘加递推发生溢出。
            input_x = rng.uniform(-0.1, 0.1, chunk_shape).astype(np.float16)
            input_y = rng.uniform(-0.1, 0.1, chunk_shape).astype(np.float16)
            golden = fused_compute(input_x, input_y, fused_iterations)
            input_x.tofile(input_x_file)
            input_y.tofile(input_y_file)
            golden.tofile(golden_file)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--fused-iterations", type=int, default=14, help="positive number of ti+1 = ti * x + y recurrence iterations"
    )
    args = parser.parse_args()
    if args.fused_iterations < 1:
        parser.error("--fused-iterations must be a positive integer")
    gen_golden_data(args.fused_iterations)
