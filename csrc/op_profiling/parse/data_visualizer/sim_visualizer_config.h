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


#ifndef MSOPT_SIM_VISUALIZER_CONFIG_H
#define MSOPT_SIM_VISUALIZER_CONFIG_H

#include "profiling/simulator/data_parse/sim_defs.h"
#include "parse/data_table/instr_detail_table.h"
#include "parse/data_table/cache_detail_table.h"

namespace Profiling {
namespace Parse {

struct SimData {
    std::shared_ptr<Parse::InstrDetailTable> instrs;
    std::shared_ptr<Parse::CacheDetailTable> caches;
    std::shared_ptr<UserMarkStruct> userMarks;
};

constexpr int DEFAULT_MHZ = 1850;
constexpr int DIR_DEFAULT_MODE = 0740;

class SimVisualizerConfig {
public:
    explicit SimVisualizerConfig(const std::string &outputPath, Profiling::Pc2CodeMap& pc2code,
        uint32_t threads, Common::ChipProductType chipType = Common::ChipProductType::ASCEND910B_SERIES)
        : outputPath_(outputPath), pc2code_(pc2code), threads_(threads), chipType_(chipType) {}

    Common::ChipProductType GetChipType() const
    {
        return chipType_;
    }

    Common::ChipProductType GetChipTypeProduct() const
    {
        return Common::GetProductSeriesType(chipType_);
    }

    std::string &GetOutputPath()
    {
        return outputPath_;
    }

    Profiling::Pc2CodeMap& GetPc2Code() const
    {
        return pc2code_;
    }

    uint32_t GetThreads() const
    {
        return threads_;
    }
private:
    std::string outputPath_;
    Profiling::Pc2CodeMap& pc2code_;
    uint32_t threads_;
    Common::ChipProductType chipType_;
};

}
}
#endif // MSOPT_SIM_VISUALIZER_CONFIG_H
