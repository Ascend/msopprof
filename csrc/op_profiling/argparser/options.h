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

#ifndef __PARSER_OPTIONS_H__
#define __PARSER_OPTIONS_H__

#include <ios>
#include <string>

namespace Parser {

struct OnOff {
    bool isOn;
};

inline bool ParseValue(std::string const &str, std::string &value)
{
    if (!str.empty()) {
        value = str;
    }
    return true;
}

inline bool ParseValue(std::string const &str, OnOff &value)
{
    if (str == "on") {
        value.isOn = true;
        return true;
    } else if (str == "off") {
        value.isOn = false;
        return true;
    } else {
        return false;
    }
}
};

#endif  // __PARSER_OPTIONS_H__
