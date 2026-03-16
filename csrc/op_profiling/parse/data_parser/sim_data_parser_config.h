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


#ifndef MSOPT_SIM_DATA_PARSER_CONFIG_H
#define MSOPT_SIM_DATA_PARSER_CONFIG_H

#include <utility>
#include <vector>
#include <string>
#include "filesystem.h"
#include "profiling/simulator/data_parse/sim_common_statistic.h"

namespace Profiling {
namespace Parse {

class SimDataParserConfig {
public:
    explicit SimDataParserConfig(std::string dumpDir, CoreNameAndPreFixPair  coreInfo,
        const std::set<int> &parseCoreId, bool enableResourceConflictRatio,
        ChipProductType chipType = ChipProductType::ASCEND910B_SERIES)
        : parseCoreId_(parseCoreId), enableResourceConflictRatio_(enableResourceConflictRatio), chipType_(chipType),
        coreName_(coreInfo.first), dumpDir_(std::move(dumpDir)), coreInfo_(std::move(coreInfo)) {}

    explicit SimDataParserConfig(std::string coreName, const std::set<int> &parseCoreId,
        bool enableResourceConflictRatio, ChipProductType chipType = ChipProductType::ASCEND910B_SERIES)
        : parseCoreId_(parseCoreId), enableResourceConflictRatio_(enableResourceConflictRatio), chipType_(chipType),
        coreName_(std::move(coreName)) {}

    ChipProductType GetChipType() const
    {
        return chipType_;
    }

    ChipProductType GetProductSeriesType() const
    {
        return ::GetProductSeriesType(chipType_);
    }

    void SetEnableResourceConflictRatio(bool enableResourceConflictRatio)
    {
        enableResourceConflictRatio_ = enableResourceConflictRatio;
    }

    bool DisableSetAndWaitInstr(const std::string &instrName) const
    {
        if (GetEnableResourceConflictRatio()) {
            return false;
        }
        // when ResourceConflictRatio is disabled, WAIT_FLAG/wait_event and SET_FLAG/set_event instr should be disabled.
        if (instrName == WAIT_EVENT || instrName == SET_EVENT || instrName == WAIT_FLAG || instrName == SET_FLAG) {
            return true;
        }
        return false;
    }

    bool GetEnableResourceConflictRatio() const
    {
        return enableResourceConflictRatio_;
    }

    const CoreNameAndPreFixPair& GetCoreInfo() const
    {
        return coreInfo_;
    }

    void SetCoreName(const std::string &coreName)
    {
        coreName_ = coreName;
    }

    const std::string& GetCoreName() const
    {
        return coreName_;
    }

    std::string GetDumpDir() const
    {
        return dumpDir_;
    }

    const std::set<int> &GetParseCoreId()
    {
        return parseCoreId_;
    }

    std::vector<std::string> GetSplitFilesVec(const std::string &fileName, const std::string &suffix)
    {
        std::vector<std::string> dumpFilePathVec;
        std::string fileNamePath = Utility::JoinPath({dumpDir_, fileName + suffix});
        int i = 1;
        std::string dumpFilePath;
        if (Utility::IsExist(fileNamePath)) {
            dumpFilePathVec.emplace_back(fileNamePath);
        }
        size_t fileCount = 0;
        while (true) {
            if (fileCount >= Utility::MAX_NUM_TRAVERSED_FILES) {
                return dumpFilePathVec;
            }
            dumpFilePath = Utility::JoinPath({dumpDir_, fileName + "." + std::to_string(i++) + suffix});
            if (Utility::IsExist(dumpFilePath)) {
                dumpFilePathVec.emplace_back(dumpFilePath);
            } else {
                return dumpFilePathVec;
            }
            fileCount++;
        }
    }
private:
    std::set<int> parseCoreId_;
    bool enableResourceConflictRatio_ = true;
    ChipProductType chipType_;
    std::string coreName_;
    // dump parser only
    std::string dumpDir_;
    CoreNameAndPreFixPair coreInfo_;
};
}
}
#endif // MSOPT_SIM_DATA_PARSER_CONFIG_H
