/* -------------------------------------------------------------------------
 *  This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * ------------------------------------------------------------------------- */

#ifndef MSOPT_OP_RUNNER_H
#define MSOPT_OP_RUNNER_H

#include "data_format.h"

namespace OpRunner {
enum class RunType {
    EXEC_BINARY = 0,
    CONFIG_FILE
};

struct Param {
    std::string type;
    std::string name;
    std::string dType;
    size_t dataSize;
    std::string dataPath;
    bool isRequired;
};

struct KernelConfig {
    bool hasTilingKey {false};
    uint64_t tilingKey {0};
    int blockDim {0};
    int deviceID {0};
    std::string kernelName;
    std::string kernelBinaryPath;
    std::string magic;
    std::string outputDataPath;
    std::string runMode;
    std::vector<Param> params;
};

extern "C" bool RunOpBinary(const std::vector<std::string>& executeCmd,
                            const std::map<std::string, std::string> &envs, int32_t timeout = -1);
}
#endif // MSOPT_OP_RUNNER_H
