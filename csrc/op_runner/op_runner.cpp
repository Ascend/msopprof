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


#include "op_runner.h"

#include "runner_impl/exec_binary_runner.h"

using namespace OpRunner;

extern "C" {
bool __attribute__((visibility("default"))) RunOpBinary(const std::vector<std::string>& executeCmd,
                                                        const std::map<std::string, std::string>& envs, int32_t timeout)
{
    ExecBinaryRunner runner;
    return runner.Run(executeCmd, envs, timeout);
}
}

