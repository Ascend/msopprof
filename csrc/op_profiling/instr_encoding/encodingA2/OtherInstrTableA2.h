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


#ifndef MSOPT_OTHER_INSTR_TABLE_A2_H
#define MSOPT_OTHER_INSTR_TABLE_A2_H

#include <unordered_map>
#include <string>

namespace Encode {
const std::unordered_map<uint32_t, std::string> INSTR18_TABLE1_A2 = {
    {0x41f00000, "SET_CROSS_CORE"}
};
}

#endif // MSOPT_OTHER_INSTR_TABLE_A2_H
