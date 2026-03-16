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


#ifndef __MSOPPROF_PROFILING_DEVICE_DATA_PARSE_H__
#define __MSOPPROF_PROFILING_DEVICE_DATA_PARSE_H__

#include <map>
#include <memory>
#include <vector>

#include "common/defs.h"
#include "common/prof_args.h"
#include "common/dbi_defs.h"
#include "metric_data_handler.h"
#include "profiling/op_prof_data_parse.h"
#include "profiling/device/data_visualize/data_visualize.h"
#include "profiling/op_prof_task.h"

namespace Profiling {

class DeviceDataParse : public DataParse {
public:
    DeviceDataParse(Common::ChipType chipType, Common::PmuEventsId pmuEventsId,
                    Common::ProfMetricsAbilityConfig metrics);
    bool Execute(std::string dataPath) override;

private:
    struct ChipInfo {
        std::map<std::string, std::vector<std::string>> metricHeader;
        Common::MetricEventsMapType aicMetricEventsMap;
        Common::MetricEventsMapType aivMetricEventsMap;
    };
    void GetCalMetricItems(const Common::ProfMetricsAbilityConfig &metrics);
    std::map<std::string, ProfBinInfo, FileNameCompare> GetProfBinInfo(const Common::ProfMetricsAbilityConfig &metrics,
                                                      const Common::PmuEventsId &pmuEventsId) const;
    void GetRelatedMap(const Common::ProfMetricsAbilityConfig &metrics,
                              const Common::MetricEventsMapType &metricEvents,
                              std::map<uint16_t, std::vector<std::string>> &relatedMap) const;
    void ParseKernelFile(const std::string &kernelDir, const std::string &kernelName,
                         const std::string &deviceId) override;
    bool CheckKernelFiles(const std::string &path, std::vector<std::string> &fileNames,
                          std::string &errorMsg) const;
    bool ParseExactKernelData(const std::string &path, const std::string &kernelName = "");
    bool ParserInit();
    void ParseTmpDump(const std::string &tmpPath);
    void GetRangeFreq(const std::string &path, std::vector<std::string> &freqs) const;
    void GetRangeKernelDurBin(const std::string &path);
    void GenerateRangeKernelBin(const std::vector<std::string> &outputVec, const std::vector<char> &profBinData,
                                const std::pair<uint16_t, uint16_t> &streamAndTaskId, const std::string &path, int round, size_t kernelIndex);
    void ParseRangeKernelProfBin(const std::string &path, const std::vector <std::string> &outputVec, int round);
    void ParseSingleRangeData(const std::string &path);
    /// mapping: ChipType -> specific data handler
    std::map<Common::ChipType, ChipInfo> chipInfoMap_;

    ChipInfo chipInfo_;
    std::set<std::string> aicCalMetricItems_;
    std::set<std::string> aivCalMetricItems_;
    // key is file name
    std::map<std::string, ProfBinInfo, FileNameCompare> eventMap_;
    Common::ChipType chipType_;
    ChipProductType chipProductType_;
    Common::PmuEventsId pmuEventsId_;
    // range replay duration map: map<pair<streamId, taskId>, vector<char>>
    std::map<std::pair<uint16_t, uint16_t>, std::vector<char>> rangeReplayDurBinMap_;
};
}

#endif // __MSOPPROF_PROFILING_DEVICE_DATA_PARSE_H__
