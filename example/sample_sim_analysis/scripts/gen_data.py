#!/usr/bin/python3
# coding=utf-8

# ----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# ----------------------------------------------------------------------------------------------------------


import os
import numpy as np

def FastGelu(x):
    abs_x = np.abs(x)
    x2 = np.exp(0.851 * (x - abs_x))
    x3 = 1 + np.exp(-1.702 * abs_x)
    return x * x2 / x3

def gen_golden_data():
    np.random.seed(0)
    groupNum = 8
    m = 1024
    k = 1024
    n = 8192
    groupList = np.array([128] * groupNum, dtype=np.int64)

    x = np.random.randint(-10, 10, [m, k]).astype(np.int8)
    weight = np.random.randint(-10, 10, [groupNum, k, n]).astype(np.int8)
    scale = np.random.normal(0, 0.01, (groupNum, n)).astype(np.float32)
    perTokenScale = np.random.normal(0, 0.01, (m, 1)).astype(np.float32)

    # 关键优化：一次性转换为 float32，让 BLAS 加速矩阵乘法
    x_f32 = x.astype(np.float32)
    weight_f32 = weight.astype(np.float32)

    # 预计算分割索引
    index = np.cumsum(groupList)
    indices = [0] + index.tolist()

    mmOut = np.empty((m, n), dtype=np.float32)

    for i in range(groupNum):
        start_idx = indices[i]
        end_idx = indices[i + 1]
        # 直接使用 float32 矩阵乘法，BLAS 会多核并行加速
        mmOut[start_idx:end_idx] = np.dot(x_f32[start_idx:end_idx], weight_f32[i])
        # 应用 scale 和 perTokenScale
        mmOut[start_idx:end_idx] *= scale[i] * perTokenScale[start_idx:end_idx]

    golden = FastGelu(mmOut)
    golden = golden.astype(np.float16)
    os.makedirs("input", exist_ok=True)
    os.makedirs("output", exist_ok=True)

    x.tofile("./input/x.bin")
    weight_nz = weight.reshape([groupNum, k // 16, 16, n // 32, 32]).transpose([0, 3, 1, 2, 4])
    weight_nz.tofile("./input/weight.bin")
    groupList.tofile("./input/groupList.bin")
    scale.tofile("./input/scale.bin")
    perTokenScale.tofile("./input/perTokenScale.bin")
    golden.tofile("./output/golden.bin")

if __name__ == "__main__":
    gen_golden_data()
