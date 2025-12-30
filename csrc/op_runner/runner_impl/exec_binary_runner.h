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

#ifndef MSOPT_EXEC_BINARY_RUNNER_H
#define MSOPT_EXEC_BINARY_RUNNER_H

#include <map>
#include <vector>

#include "data_format.h"

namespace OpRunner {

class ExecBinaryRunner {
public:
    // timeout: binary runs timeout, default value -1 for not set timeout
    bool Run(const std::vector<std::string>& executeCmd, const std::map<std::string, std::string>& envs,
             int32_t timeout = -1);

private:
    void KillBinaryProcess() const;
    pid_t pid_;
};

}

#endif // MSOPT_EXEC_BINARY_RUNNER_H