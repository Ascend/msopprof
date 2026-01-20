
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

#include "log.h"
#include "parse/data_table/mte_throughput_table.h"
#include "smart_pointer.h"
#include "mte_log_visualizer.h"

using namespace Utility;

namespace Profiling {
namespace Parse {

PluginErrorCode MteLogVisualizer::Entry()
{
    std::shared_ptr<std::vector<nlohmann::json>> mteThroughputJsonPtr = MakeShared<std::vector<nlohmann::json>>();
    if (mteThroughputJsonPtr == nullptr) {
        LogError("Failed to create mte Throughput Json in visualization.");
        return PluginErrorCode::FATAL_ERROR;
    }
    dataCenter_.DataTableRegister(mteThroughputJsonPtr);
    std::shared_ptr<MteThroughputChart> mteThroughputChartPtr = dataCenter_.GetDbPtr<MteThroughputChart>();
    if (mteThroughputChartPtr == nullptr) {
        LogError("Failed to get mte throughput data in visualization.");
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    LogDebug("Start to visualize mte throughput, max time:%zu.", mteThroughputChartPtr->size());
    validTsNum_ = 0;
    for (size_t i = 0; i < mteThroughputChartPtr->size(); ++i) {
        FillData(*mteThroughputJsonPtr, (*mteThroughputChartPtr)[i], i);
    }
    LogDebug("Finish to visualize mte throughput, valid time point num:%d.", validTsNum_);

    // 当采集MTE无数据时，不进行落盘，避免影响其他泳道的显示
    if (validTsNum_ == 0) {
        LogDebug("No valid data to visualize mte throughput.");
        return PluginErrorCode::SUCCESS;
    }
    // 需要在尾部补充值为0的数据，否则最后一个数据只会显示一条线
    FillData(*mteThroughputJsonPtr, {0, 0, 0, 0, 0, 0}, endTs_ + 1);
    return PluginErrorCode::SUCCESS;
}

void MteLogVisualizer::FillData(std::vector<nlohmann::json> &mteThroughputJson,
                                const std::vector<double> &valueList, size_t ts)
{
    if (valueList.size() < static_cast<size_t>(MteLogInstrType::END)) {
        return;
    }
    for (size_t type = 0; type < static_cast<size_t>(MteLogInstrType::END); ++type) {
        nlohmann::json tsPointData;
        tsPointData["pid"] = "MTE Throughput";
        tsPointData["tid"] = 0;
        tsPointData["name"] = MteLogInstrTypeStr.at(static_cast<MteLogInstrType>(type));
        tsPointData["ph"] = "C";
        tsPointData["ts"] = ts;
        nlohmann::json args;
        args["throughput(MB/s)"] = valueList[type];
        tsPointData["args"] = args;
        mteThroughputJson.push_back(tsPointData);
    }
    endTs_ = ts;
    validTsNum_++;
}
}
}