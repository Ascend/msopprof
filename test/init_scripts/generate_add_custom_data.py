#!/usr/bin/env python3
# -*- coding: utf-8 -*-
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
import stat
import sys
import numpy as np

OPEN_FILE_MODES_640 = stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP
WRITE_FILE_FLAGS = os.O_WRONLY | os.O_CREAT | os.O_TRUNC


def write_file(path):
    with os.fdopen(os.open(path, os.O_WRONLY | os.O_CREAT, 0o640),
                   'w', encoding='utf8', newline='') as file_handler:
        file_handler.write('')


def gen_golden_data_simple(output_path):
    if not os.path.exists(output_path):
        os.makedirs(output_path)
    one_repeat_calcount = 128
    block_dim_imm = 8
    tile_num_imm = 8
    double_buffer_imm = 2
    total_length_imm = block_dim_imm * \
                       one_repeat_calcount * tile_num_imm * double_buffer_imm

    total_length = np.array(total_length_imm, dtype=np.uint32)
    tile_num = np.array(tile_num_imm, dtype=np.uint32)
    tiling = (total_length, tile_num)
    tiling_data = b''.join(x.tobytes() for x in tiling)
    tiling_path = os.path.join(output_path, 'tiling.bin')
    with os.fdopen(os.open(tiling_path, WRITE_FILE_FLAGS, OPEN_FILE_MODES_640), 'wb') as f:
        f.write(tiling_data)

    input_x = np.random.uniform(-100, 100, [total_length_imm]).astype(np.float16)
    input_y = np.random.uniform(-100, 100, [total_length_imm]).astype(np.float16)
    golden = (input_x + input_y).astype(np.float16)

    input_x.tofile(os.path.join(output_path, 'input_x.bin'))
    input_y.tofile(os.path.join(output_path, 'input_y.bin'))
    golden.tofile(os.path.join(output_path, 'golden.bin'))
    write_file(os.path.join(output_path, 'add_custom.o'))


if __name__ == "__main__":
    input = sys.argv[1]
    currentDir = os.path.abspath(os.path.dirname(os.path.realpath(__file__)))
    os.chdir(currentDir)
    # 设置输出路径的默认值
    if input == "":
        input = currentDir + "/../../build_ut/test/ut/resources"
    output_path = input + "/op_test/add_custom"
    output = os.path.join(os.path.dirname(__file__), output_path)
    if not os.path.exists(output):
        os.makedirs(output)
    gen_golden_data_simple(output)
