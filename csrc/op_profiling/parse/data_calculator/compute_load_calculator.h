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

#ifndef MSOPT_PARSE_COMPUTE_LOAD_CALCULATOR_H
#define MSOPT_PARSE_COMPUTE_LOAD_CALCULATOR_H

#include "parse/data_center/data_center.h"
#include "parse/plugin/plugin_interface.h"
#include "profiling/device/data_visualize/data_visualize_const.h"
#include "profiling/device/data_visualize/basic_op_and_pmu.h"

namespace Profiling {
namespace Parse {

struct ComputeLoadInfo {
    nlohmann::json figure;
    nlohmann::json table;
};

class ComputeLoadCalculator : public PluginInterface {
public:
    explicit ComputeLoadCalculator(DataCenter& dataCenter, const Common::ChipProductType &chipType)
        : PluginInterface(dataCenter, chipType) {}
    PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("ComputeLoadCalculator");
        RegisterMandatoryDb({});
        RegisterChip({Common::ChipProductType::ALL_PRODUCT_TYPE});
    }
private:
    void ComputeLoadInfoFor910B(std::map<uint8_t, std::vector<nlohmann::json>> &jsonMap);
    void ComputeLoadInfoFor310P(std::map<uint8_t, std::vector<nlohmann::json>> &jsonMap);
    void ComputeLoadInfoFor91095(std::map<uint8_t, std::vector<nlohmann::json>> &jsonMap);
    void GenBlockDetail(const Visualize::ComputeLoadBlockDetail &computeLoadBlockDetail, const uint64_t &value,
        const Visualize::DetailInfo &detailInfo, std::map<uint8_t, std::vector<nlohmann::json>> &jsonMap) const;
    
    ComputeLoadInfo computeLoadInfo;
    // 计算负载类的常规数据,被计算负载类的方法调用
    const std::vector<Visualize::DetailInfo> detailInfoAicA5_ = {
        {"Cube All Active", Visualize::UnitType::PER, 0 },
        {"Cube FP", Visualize::UnitType::PER, 0},
        {"Cube INT", Visualize::UnitType::PER, 0},
        {"Cube FP", Visualize::UnitType::INSTR, 1},
        {"Cube INT", Visualize::UnitType::INSTR, 1},
        {"Cube All Active", Visualize::UnitType::INSTR, 1},
        {"Cube Wait", Visualize::UnitType::US, 1},
    };
    const std::vector<Visualize::DetailInfo> detailInfoAivA5_ = {
        {"Vector All Active", Visualize::UnitType::PER, 0},
        {"Vector All Active", Visualize::UnitType::INSTR, 1},
        {"Vector Wait", Visualize::UnitType::US, 1},
    };
};

}
}
#endif
