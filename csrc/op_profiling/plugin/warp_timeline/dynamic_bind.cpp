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

#include <tuple>
#include "kernel_injection/include/MSBit.h"

extern "C" {
std::vector<std::tuple<InstrType, std::string, std::vector<uint16_t>>> BIND_FUNCTION{
    {InstrType::SIMT_START, "__msopprof_report_simt_start", {}},
    {InstrType::SIMT_END, "__msopprof_report_simt_end", {}},
};

void MSBitAtInit() {
    for (auto it : BIND_FUNCTION) {
        Bind(std::get<0>(it), std::get<1>(it), std::get<2>(it));
    }
}
}
