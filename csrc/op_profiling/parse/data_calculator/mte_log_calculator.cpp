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

#include "mte_log_calculator.h"

#include <vector>
#include <algorithm>
#include "log.h"
#include "common/visualize.h"
#include "filesystem.h"
#include "profiling/device/data_parse/pmu_calculate.h"
#include "number_operation.h"

using namespace Utility;

namespace Profiling {
namespace Parse {
constexpr uint64_t MAX_MTE_SIZE = 60000000; // 1分钟的算子运行时间申请MteThroughputChart的大小约为4.3GB

PluginErrorCode MteLogCalculator::Entry()
{
    std::shared_ptr<std::vector<Parse::MteLogInstrMap>> mteLogInstrMapVecPtr =
        dataCenter_.GetDbPtr<std::vector<Parse::MteLogInstrMap>>();
    if (mteLogInstrMapVecPtr == nullptr) {
        Utility::LogError("Failed to get mte log data.");
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    // 注册数据到数据中心
    LogDebug("start to calculate mte throughput, core num:%zu.", mteLogInstrMapVecPtr->size());
    std::shared_ptr<MteThroughputChart> mteThroughputChartPtr = std::make_shared<MteThroughputChart>();
    dataCenter_.DataTableRegister(mteThroughputChartPtr);
    // 计算六种类型的每个时间点上的吞吐率
    for (Parse::MteLogInstrMap &mteLogInstrMap : *mteLogInstrMapVecPtr) {
        for (const auto &mteLogPair : mteLogInstrMap) {
            // maxReqTs = -1 表示没有和GM交互
            if (SafeEqual(mteLogPair.second.maxReqTs, -1.0f, 0.0f)) {
                continue;
            }
            uint64_t maxTsPoint = static_cast<uint64_t>(std::floor(mteLogPair.second.maxReqTs));
            // 在instr层进行resize，减少resize次数，提高性能
            if (mteThroughputChartPtr->size() <= maxTsPoint) {
                std::vector<double> tmp(static_cast<size_t>(MteLogInstrType::END), 0);
                mteThroughputChartPtr->resize(maxTsPoint + 1, tmp);
            }
            CalOneInstrThroughput(mteLogPair.second.instrType, mteLogPair.second.reqTbl, *mteThroughputChartPtr);
        }
    }
    LogDebug("Finish to calculate mte throughput, max time:%zu.", mteThroughputChartPtr->size());
    return PluginErrorCode::SUCCESS;
}

void MteLogCalculator::CalOneInstrThroughput(MteLogInstrType instrType,
                                             const std::unordered_map<uint64_t, MteLogReqInfo> &reqInfo,
                                             MteThroughputChart &mteThroughputInfo) const
{
    size_t instrTypeIndex = static_cast<uint64_t>(instrType);
    bool exceedLimit = false;
    for (const auto &reqPair : reqInfo) {
        uint64_t tsPoint = static_cast<uint64_t>(std::floor(reqPair.second.ts));
        if (tsPoint > MAX_MTE_SIZE) {
            exceedLimit = true;
            continue;
        }
        if (tsPoint >= mteThroughputInfo.size()) {
            std::vector<double> tmp(static_cast<size_t>(MteLogInstrType::END), 0);
            mteThroughputInfo.resize(tsPoint + 1, tmp);
        }
        mteThroughputInfo[tsPoint][instrTypeIndex] += (reqPair.second.dataSize * THROUGHPUT_CONVERSION); // 转化为MB/s
        if (instrType == MteLogInstrType::GM_TO_UB || instrType == MteLogInstrType::GM_TO_L1) {
            mteThroughputInfo[tsPoint][static_cast<uint64_t>(MteLogInstrType::GM_TO_TOTAL)]
                += (reqPair.second.dataSize * THROUGHPUT_CONVERSION);
        } else if (instrType == MteLogInstrType::UB_TO_GM || instrType == MteLogInstrType::L1_TO_GM) {
            mteThroughputInfo[tsPoint][static_cast<uint64_t>(MteLogInstrType::TOTAL_TO_GM)]
                += (reqPair.second.dataSize * THROUGHPUT_CONVERSION);
        }
    }
    if (exceedLimit) {
        LogWarn("MTE log receive ticks exceed one minutes, only show part result");
    }
}
}
}
