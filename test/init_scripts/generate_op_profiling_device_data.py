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
import struct
import sys


def write_file(path, data):
    with os.fdopen(os.open(path, os.O_WRONLY | os.O_CREAT, 0o640), 'wb') as file_handler:
        file_handler.write(data)


def gen_bbb_data(path):
    bbbmap = b'''InstrumentBlockCount
function: matmul_custom_0_mix_aic
start addr: 0x000000
BBB id:0 Offset: 0x000000 end: 0x000048
BBB id:1 Offset: 0x00004c end: 0x00004c
BBB id:2 Offset: 0x000050 end: 0x000054
BBB id:3 Offset: 0x000058 end: 0x000064
BBB id:4 Offset: 0x000068 end: 0x000698
BBB id:5 Offset: 0x00069c end: 0x0006c8
BBB id:6 Offset: 0x0006cc end: 0x000738
BBB id:7 Offset: 0x00073c end: 0x0007c0
BBB id:8 Offset: 0x0007c4 end: 0x0007c8
BBB id:9 Offset: 0x0007cc end: 0x000880
'''
    write_file(os.path.join(path, 'kernel0Stub.o.bbbmap.0'), bbbmap)


def gen_data(output_path):
    # pack the data into binary format
    pmu_values = list(range(1001, 1009))
    prof_arr_910b = [0] * 48
    # coretype
    prof_arr_910b[7] = 1
    # totalCycles
    prof_arr_910b[12] = 10000
    prof_arr_910b[36] = 20000
    # pmuValues
    prof_arr_910b[14:22] = pmu_values
    prof_arr_910b[38:46] = pmu_values

    time_arr_910b = [0] * 36
    time_arr_910b[0] = 0
    time_arr_910b[4] = 30000
    time_arr_910b[18] = 1
    time_arr_910b[22] = 40000

    output_910b = output_path + "/device910B/dump"
    if not os.path.exists(output_910b):
        os.makedirs(output_910b)
    for i in range(1, 7):
        write_file(os.path.join(output_910b, 'DeviceProf' + str(i) + '.bin'), struct.pack("4HQ4HIHH12Q4HQ4HIHH12Q",
                                                                                          *prof_arr_910b))
    write_file(os.path.join(output_910b, 'duration.bin'), struct.pack("4HQHH11I4HQHH11I", *time_arr_910b))
    gen_bbb_data(output_910b)
    # every MsprofAicpuMC2HcclInfo 45 nums, AicpuKfcProfCommTurn 44 nums
    aicpu_arr_910b = [0] * (45 + 44)
    aicpu_pack = "2H3I4Q6IQ2Id3Q6I4H13Q" + "2H3I9QI2H8B19Q"
    aicpu_arr_910b[1] = 6000                   # MSPROF_REPORT_AICPU_LEVEL
    aicpu_arr_910b[2] = 6                      # MSPROF_REPORT_AICPU_MC2_HCCL_INFO
    aicpu_arr_910b[6] = 308727085000894009     # HCCL_TASK_TYPE_MAP: hccl_info
    aicpu_arr_910b[13] = 0                     # MsprofAicpuMC2HcclInfo: planeID
    aicpu_arr_910b[23] = 0                     # MsprofAicpuMC2HcclInfo: dataType
    aicpu_arr_910b[24] = 0                     # MsprofAicpuMC2HcclInfo: linkType
    aicpu_arr_910b[25] = 0                     # MsprofAicpuMC2HcclInfo: transportType
    aicpu_arr_910b[27] = 3                     # MsprofAicpuMC2HcclInfo: taskId
    aicpu_arr_910b[28] = 53                    # MsprofAicpuMC2HcclInfo: streamId
    aicpu_arr_910b[45 + 1] = 6000          # MSPROF_REPORT_AICPU_LEVEL
    aicpu_arr_910b[45 + 2] = 4             # MSPROF_REPORT_AICPU_MC2_EXECUTE_COMM_TIME
    write_file(os.path.join(output_910b, 'aicpu.bin'), struct.pack(aicpu_pack, *aicpu_arr_910b))

    prof_arr_310p = [0] * 25
    # totalCycles
    prof_arr_310p[7] = 50000
    # pmuValues
    prof_arr_310p[9:17] = pmu_values

    time_arr_310p = [0] * 36
    time_arr_310p[0] = 0
    time_arr_310p[5] = 60000
    time_arr_310p[18] = 1
    time_arr_310p[23] = 70000

    l2cache_arr_310p = [0] * 12
    # pmuValues
    l2cache_arr_310p[4:12] = pmu_values
    output_310p = output_path +  "/device310P/dump"
    if not os.path.exists(output_310p):
        os.makedirs(output_310p)
    for i in range(1, 7):
        write_file(os.path.join(output_310p, 'DeviceProf' + str(i) + '.bin'), struct.pack("BBHHHII10Q8I",
                                                                                          *prof_arr_310p))
    write_file(os.path.join(output_310p, 'duration.bin'), struct.pack("BBHHHQ12IBBHHHQ12I", *time_arr_310p))
    write_file(os.path.join(output_310p, 'L2Cache.bin'), struct.pack("HHHH8Q", *l2cache_arr_310p))

    prof_arr_A5 = [0] * 24
    prof_arr_A5[0] = 41
    # totalCycles
    prof_arr_A5[4] = 50000
    # pmuValues
    prof_arr_A5[12:22] = list(range(1001, 1011))
    time_arr_A5 = [0] * 20
    time_arr_A5[0] = 0
    time_arr_A5[4] = 60000
    time_arr_A5[10] = 1
    time_arr_A5[14] = 70000
    output_A5 = output_path +  "/deviceA5/dump"
    for i in range(1, 7):
        write_file(os.path.join(output_A5, 'DeviceProf' + str(i) + '.bin'), struct.pack("4HQ4H2HI12Q",
                                                                                        *prof_arr_A5))
    write_file(os.path.join(output_A5, 'duration.bin'), struct.pack("4HQ2H3I4HQ2H3I", *time_arr_A5))

if __name__ == "__main__":
    input = sys.argv[1]
    currentDir = os.path.abspath(os.path.dirname(os.path.realpath(__file__)))
    os.chdir(currentDir)
    # 设置输出路径的默认值
    if input == "":
        input = currentDir + "/../../build_ut/test/ut/resources"
    output_path = input + "/op_profiling"
    gen_data(output_path)
