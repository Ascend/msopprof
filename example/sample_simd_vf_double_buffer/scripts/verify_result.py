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


import os
import sys

import numpy as np

RELATIVE_TOL = 1e-4
ABSOLUTE_TOL = 1e-5
EXPECTED_ELEMENTS = 16 * 8192
CHUNK_ELEMENTS = 1024 * 1024
MAX_REPORTED_MISMATCHES = 10


def check_file_size(file_path):
    expected_bytes = EXPECTED_ELEMENTS * np.dtype(np.float16).itemsize
    actual_bytes = os.path.getsize(file_path)
    if actual_bytes != expected_bytes:
        raise ValueError(f"size mismatch for {file_path}: expected={expected_bytes}, actual={actual_bytes}")


def verify_result(output_path, golden_path):
    check_file_size(output_path)
    check_file_size(golden_path)
    output = np.memmap(output_path, dtype=np.float16, mode="r", shape=(EXPECTED_ELEMENTS,))
    golden = np.memmap(golden_path, dtype=np.float16, mode="r", shape=(EXPECTED_ELEMENTS,))
    mismatch_count = 0
    reported_count = 0

    for start in range(0, EXPECTED_ELEMENTS, CHUNK_ELEMENTS):
        end = min(start + CHUNK_ELEMENTS, EXPECTED_ELEMENTS)
        matched = np.isclose(output[start:end], golden[start:end], rtol=RELATIVE_TOL, atol=ABSOLUTE_TOL, equal_nan=False)
        mismatch_offsets = np.flatnonzero(~matched)
        mismatch_count += mismatch_offsets.size
        for offset in mismatch_offsets[: MAX_REPORTED_MISMATCHES - reported_count]:
            real_index = start + int(offset)
            golden_data = golden[real_index]
            output_data = output[real_index]
            absolute_diff = abs(float(output_data) - float(golden_data))
            relative_diff = absolute_diff / max(abs(float(golden_data)), ABSOLUTE_TOL)
            print(
                f"data index: {real_index:06d}, expected: {golden_data:-.9f}, "
                f"actual: {output_data:-.9f}, rdiff: {relative_diff:.6f}"
            )
            reported_count += 1

    print(f"mismatched elements: {mismatch_count}, rtol: {RELATIVE_TOL}, atol: {ABSOLUTE_TOL}")
    return mismatch_count == 0


if __name__ == "__main__":
    try:
        if len(sys.argv) != 3:
            raise ValueError(f"Usage: {sys.argv[0]} <output.bin> <golden.bin>")
        res = verify_result(sys.argv[1], sys.argv[2])
        if not res:
            raise ValueError("[ERROR] result error")
        else:
            print("test pass!")
    except (OSError, ValueError) as e:
        print(e)
        sys.exit(1)
