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


#ifndef MSOPT_GPRLIVEREGISTERCALCULATOR_H
#define MSOPT_GPRLIVEREGISTERCALCULATOR_H

#include "instr_detail_calculator.h"
#include "instr_detail_defs.h"
#include "parse/data_table/instr_detail_table.h"

namespace Profiling {
namespace Parse {

const std::map<Common::ChipProductType, std::regex> patternMap = {
    {Common::ChipProductType::ASCEND310P_SERIES,
     std::regex(R"(X(\d{1,2})|x[a-z]:0x(\d{1,2})|x[a-z]:(\d{1,2})|x\[(\d{1,2})\])")},
    {Common::ChipProductType::ASCEND910_95_SERIES, std::regex(R"(R([a-z0-9]{1,2}):([0-9a-f]{1,2})\|R)")},
    {Common::ChipProductType::ASCEND910B_SERIES, std::regex(R"(X(\d{1,2}))")},
    {Common::ChipProductType::ASCEND910_93_SERIES, std::regex(R"(X(\d{1,2}))")},
};

void GetNormalDstAndSrcRegister(std::vector<std::string> &dstRegisters, std::vector<std::string> &srcRegisters,
                                const std::vector<uint16_t> &dstRegisterLoc, std::vector<std::string> &allRegisters);
void GetSpecialDstAndSrcRegister(std::vector<std::string> &dstRegisters, std::vector<std::string> &srcRegisters,
                                 const std::vector<uint16_t> &dstRegisterLoc, std::vector<std::string> &allRegisters);

class GPRLiveRegisterCalculator : public InstrDetailCalculator {
public:
    explicit GPRLiveRegisterCalculator(DataCenter& dataCenter, InstrDetailConfig& instrDetailConfig)
        :InstrDetailCalculator(dataCenter, instrDetailConfig)
    {
        Common::ChipProductType chiptypeSeries = Common::GetProductSeriesType(chipType_);
        if (patternMap.find(chiptypeSeries) != patternMap.end()) {
            pattern_ = patternMap.at(chiptypeSeries);
        }
    };

    PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("GPRLiveRegisterCalculator");
        RegisterMandatoryDb({typeid(InstrDetailTable)});
        RegisterChip({Common::ChipProductType::ASCEND310P_SERIES,
                      Common::ChipProductType::ASCEND910_95_SERIES,
                      Common::ChipProductType::ASCEND910B_SERIES,
                      Common::ChipProductType::ASCEND910_93_SERIES});
    }
private:
    void UpdateDstRegister(InstrDetailTable &instrDetailTable,
                           std::map<std::string, uint32_t> &registerWithIndex,
                           std::vector<std::string> &dstRegisters, uint32_t index) const;
    void UpdateSrcRegister(std::map<std::string, uint32_t> &registerWithIndex,
                           std::vector<std::string> &srcRegisters, uint32_t index) const;
    void GetDstAndSrcRegister(std::vector<std::string> &dstRegisters, std::vector<std::string> &srcRegisters,
                              const MergeInfo &mergeInfo) const;
    void ExtractRegister(const std::string &detail, const std::regex &pattern, std::vector<std::string> &gprVector) const;
    void GetDstAndSrcRegisterA5(std::vector<std::string> &dstRegisters, std::vector<std::string> &srcRegisters,
        const MergeInfo &mergeInfo);
    std::regex patternOfA2_ = std::regex(R"(X(\d{1,2}))");
    std::regex patternOfA300_ =  std::regex(R"(X(\d{1,2})|x[a-z]:0x(\d{1,2})|x[a-z]:(\d{1,2})|x\[(\d{1,2})\])");
    std::regex patternOfA5_ = std::regex(R"(R([a-z0-9]{1,2}):([0-9a-f]{1,2})\|R)");
    std::regex pattern_;
};
}
}


#endif // MSOPT_GPRLIVEREGISTERCALCULATOR_H
