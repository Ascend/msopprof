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
import sys
from pathlib import Path

def writePcSampling(output_path) :
    file_path = os.path.join(os.path.dirname(__file__), "../../test/ut/resources/op_profiling/instr_prof/pcSampling.bin.0")
    data_8_bytes = b'\x00\x00\x02\x00\x58\x00\x00\x00'
    data1 = b'\x05\x00\x00' + b'\xff' * 9 + b'\x00' * 4
    data2 = b'\x05\x00\x00' + b'\x02' * 9 + b'\x00' * 4
    data3 = b'\x08\x00\x00' + b'\x04' * 9 + b'\x00' * 4
    data4 = b'\x01\x00\x00' + b'\x09' * 9 + b'\x00' * 4
    data_reserv_bytes = b'\x00' * 2097088 # 2 * 1024 * 1024 -64

    # 获取目录路径
    directory = os.path.dirname(file_path)

    # 如果目录不存在，创建目录
    if not os.path.exists(directory):
        os.makedirs(directory)

    with os.fdopen(os.open(file_path, os.O_WRONLY | os.O_CREAT, 0o640), 'wb') as file_handler:
        file_handler.write(data_8_bytes + data1 + data2 + data3 + data4 + data_reserv_bytes)

def writeTimeLine(output_path):
    file_path = output_path +  "/dump/timeline.bin.0"
    file = Path(file_path)
    file.touch(exist_ok=True)

if __name__ == "__main__":
    input = sys.argv[1]
    currentDir = os.path.abspath(os.path.dirname(os.path.realpath(__file__)))
    os.chdir(currentDir)
    # 设置输出路径的默认值
    if input == "":
        input = currentDir + "/../../build_ut/test/ut/resources"
    writePcSampling(input)
    writeTimeLine(input)