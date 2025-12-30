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


#ifndef __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_CAMODEL_VEC_NAME_MAP_H__
#define __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_CAMODEL_VEC_NAME_MAP_H__

#include <unordered_map>

#include "common/defs.h"
#include "vec_name_template.h"

namespace Profiling {
namespace Parse {

using InstrLogStr2Template = std::unordered_map<std::string, VecInstrTemplate>;

bool GetInstrLogStr2TemplateMap(Common::ChipProductType seriesChipType, InstrLogStr2Template &resMap);
}
}
#endif // __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_CAMODEL_VEC_NAME_MAP_H__
