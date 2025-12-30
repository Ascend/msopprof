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

#ifndef __PARSER_UTILS_H__
#define __PARSER_UTILS_H__

#include <vector>
#include <string>
namespace Parser {
inline std::vector<std::string> StringToArgv(std::string const &cmd)
{
    std::vector<std::string> argv;
    std::string::size_type fast = 0;
    std::string::size_type slow = cmd.find_first_not_of(" ");
    while (fast < cmd.length()) {
        fast = cmd.find_first_of(" ", slow);
        if (fast != slow) {
            argv.emplace_back(cmd.substr(slow, fast - slow));
        }
        slow = cmd.find_first_not_of(" ", fast);
    }
    return argv;
}
}
#endif  // __PARSER_UTILS_H__
