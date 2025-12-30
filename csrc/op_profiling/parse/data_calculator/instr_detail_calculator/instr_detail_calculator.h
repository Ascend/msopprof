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


#ifndef __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_CALCULATOR_H__
#define __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_CALCULATOR_H__

#include <regex>

#include "common/defs.h"
#include "profiling/simulator/data_parse/sim_defs.h"
#include "parse/plugin/base_context.h"
#include "parse/plugin/plugin_interface.h"
#include "instr_detail_defs.h"
#include "instr_in_camodel/camodel_instr_detail_map.h"

namespace Profiling {
namespace Parse {

using InstrExtractFunc = std::function<InstrRegDetail (const RegDetailRegexMap &regMap,
                                                       const std::string &instrDetail)>;
using InstrExtractAndParsePair = std::pair<InstrExtractFunc, InstrTypeTemplate>;
using InstrProcessMap = std::unordered_map<std::string, InstrExtractAndParsePair>;

struct SpRegexStruct {
    std::regex pattern;
    uint8_t expectResNum;
    uint8_t posInSpReg;
};

class InstrDetailConfig {
public:
    explicit InstrDetailConfig(Common::ChipProductType chipType = Common::ChipProductType::ASCEND910B_SERIES)
        : chipType_(chipType) {};

    Common::ChipProductType GetChipType() const
    {
        return chipType_;
    }

    Common::ChipProductType GetProductSeriesType() const
    {
        return Common::GetProductSeriesType(chipType_);
    }

private:
    const Common::ChipProductType chipType_;
};


class InstrDetailCalculator : public PluginInterface {
public:
    using SpStruct2RegexMap = std::map<std::string, std::vector<SpRegexStruct>>;

    explicit InstrDetailCalculator(DataCenter& dataCenter, InstrDetailConfig& instrDetailConfig)
        : PluginInterface(dataCenter, instrDetailConfig.GetChipType()), instrDetailConfig_(instrDetailConfig) {};

protected:
    void AttributeMapInit();
    bool GetMemOpInfo(const MergeInfo &instrInfo, MemOpInfo &memOpInfo);
    void UpdateSpReg(const std::string &name, const std::string &detail, SpReg &spReg);

    bool spEnable_ = false;
    RegDetailRegexMap regDetailRegexMap_;
    InstrProcessMap instrProcessMap_;
    SpStruct2RegexMap spStruct2RegexMap_;

    InstrDetailConfig instrDetailConfig_;
};

}
}


#endif // __MSOPPROF_PARSE_DATA_CALCULATOR_INSTR_DETAIL_CALCULATOR_H__
