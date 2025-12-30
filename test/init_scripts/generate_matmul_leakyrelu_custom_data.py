# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2025 Huawei Technologies Co.,Ltd.
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
import os
from enum import Enum
import sys
import numpy as np

np.random.seed(19)


class DataFormat(Enum):
    ND = 1
    NZ = 2
    ND_ALIGN = 3


def write_file(path):
    with os.fdopen(os.open(path, os.O_WRONLY | os.O_CREAT, 0o640),
                   'w', encoding='utf8', newline='') as file_handler:
        file_handler.write('')


def gen_golden_data(output_path):
    a1_length = 128 * 256 * np.dtype(np.float16).itemsize
    b1_length = 256 * 128 * np.dtype(np.float16).itemsize
    c01_length = 128 * 128 * np.dtype(np.float32).itemsize
    is_bias = 1
    trans_length = np.max([a1_length, b1_length, c01_length])
    x1_gm = np.random.randint(1, 10, [1024, 256]).astype(np.float16)
    x2_gm = np.random.randint(1, 10, [256, 640]).astype(np.float16)
    tiling = np.zeros((32), np.int32)
    l1_size = 0
    l0c_size = 0
    ub_size = 0
    l1_size += 1 * a1_length
    l1_size += 1 * b1_length
    iterate_size = 1
    l0c_size += iterate_size * c01_length
    # qidBias
    l1_size += iterate_size * 128 * np.dtype(np.float32).itemsize
    # qidVecIn_
    ub_size += iterate_size * trans_length
    ub_size += trans_length
    # qidCO2_
    ub_size += 128 * 128 * np.dtype(np.float32).itemsize
    tiling[0:24] = [
        2, 1024, 640, 256, 512, 640, 256, 128, 128, 256, \
        1, 1, 1, 1, a1_length, b1_length, c01_length, \
        is_bias, trans_length, 0, 0, l1_size, l0c_size, ub_size
    ]
    bias = np.random.randint(1, 10, [640]).astype(np.float32)

    golden = (np.matmul(x1_gm.astype(np.float32), x2_gm.astype(np.float32)) + bias.astype(np.float32)).astype(
        np.float32)
    golden = np.where(golden >= 0, golden, golden * 0.001)

    if DataFormat(1) == DataFormat['NZ']:
        c0_sze = 16
        if np.float16 == np.float32:
            c0_sze = 8
        x1_gm = x1_gm.reshape((int(1024 / 16), 16, int(256 / c0_sze), c0_sze)).transpose(2, 0, 1, 3).astype(np.float16)

    if DataFormat(1) == DataFormat['NZ']:
        c0_sze = 16
        if np.float16 == np.float32:
            c0_sze = 8
        x2_gm = x2_gm.reshape((int(256 / 16), 16, int(640 / c0_sze), c0_sze)).transpose(2, 0, 1, 3).astype(np.float16)

    if DataFormat(1) == DataFormat['NZ']:
        golden = golden.reshape((int(1024 / 16), 16, int(640 / 16), 16)).transpose(2, 0, 1, 3).astype(np.float32)

    x1_gm.tofile(os.path.join(output_path, "x1.bin"))
    x2_gm.tofile(os.path.join(output_path, "x2.bin"))
    tiling.tofile(os.path.join(output_path, "tiling.bin"))
    bias.tofile(os.path.join(output_path, "bias.bin"))
    golden.tofile(os.path.join(output_path, "golden.bin"))
    write_file(os.path.join(output_path, 'matmul_leakyrelu_custom.o'))


if __name__ == "__main__":
    input = sys.argv[1]
    currentDir = os.path.abspath(os.path.dirname(os.path.realpath(__file__)))
    os.chdir(currentDir)
    # 设置输出路径的默认值
    if input == "":
        input = currentDir + "/../../build_ut/test/ut/resources"
    output_path = input + "/op_test/matmul_leakyrelu_custom"
    output = os.path.join(os.path.dirname(__file__), output_path)
    if not os.path.exists(output):
        os.makedirs(output)
    gen_golden_data(output)
