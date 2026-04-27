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

#ifndef AICORE_TIMELINE_H
#define AICORE_TIMELINE_H

#include <utility>
#include <vector>
#include <cstdint>
#include "json.hpp"
#include "profiling/device/data_visualize/basic_op_and_pmu.h"
#include "profapi/profapi.h"
#include "timeline_parser.h"

namespace Visualize {

// 用于临时存储同一操作类型的start和end信息
struct OperationInfo {
    uint64_t startSyscyc;
    uint32_t blockId;
    uint64_t startCurPc;
    uint64_t endSyscyc;
    bool startFound;
    bool endFound;
    std::string type;
};

class AicoreTimelineParser : public TimelineParser {
public:
    AicoreTimelineParser(uint64_t minTimeCyc, std::shared_ptr<OpBasicInfo> &opBasicInfoObj, std::shared_ptr<BasicPmu> &basicPmuObj)
        : TimelineParser(minTimeCyc, opBasicInfoObj, basicPmuObj)
        {
            for (const auto &pair: basicPmuObj->GetTotalPmuData()) {
                if (pair.second.blockType == Common::OpType::VECTOR) {
                    aivBlockNum_ ++;
                }
                if (pair.second.blockType == Common::OpType::CUBE) {
                    aicBlocKNum_ ++;
                }
            }
        }
    bool TimelineToJson(const std::string &outputPath) override;
    bool GetAicoreTimeStamps(std::vector<MsprofAicTimeStampInfoUpdate> &aicoreTimeStamps);
    void GenPc2Code(std::vector<MsprofAicTimeStampInfoUpdate> &aicoreTimeStamps);
    void ProcessAicoreData(const std::vector<MsprofAicTimeStampInfoUpdate> &aicoreTimeStamps);
private:
    ChipProductType chipSeries_;
    void GetTimeStampType(std::vector<MsprofAicTimeStampInfo> &infos, std::vector<MsprofAicTimeStampInfoUpdate> &aicoreTimeStamps);
    uint32_t aicBlocKNum_ = 0;
    uint32_t aivBlockNum_ = 0;
    std::map<std::tuple<uint32_t, uint32_t, std::string>, std::vector<OperationInfo>> timeStampInfo_;
};

inline std::string GetAicoreBlockName(uint16_t dotBlockId)
{
    if (dotBlockId >= CUBE_BLOCK_START_INDEX) {
        return "AIC_BLOCK" + std::to_string(dotBlockId - CUBE_BLOCK_START_INDEX);
    }
    return "AIV_BLOCK" + std::to_string(dotBlockId);
}

inline std::string GetAicoreTimeLinePid(uint16_t dotBlockId)
{
    return (dotBlockId >= CUBE_BLOCK_START_INDEX) ? "AIC BLOCK" : "AIV BLOCK";
}
}

#endif // AICORE_TIMELINE_H