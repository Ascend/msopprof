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


#include "cacheline_heat_map.h"
#include "pc_process.h"
#include "filesystem.h"
#include "profiling/op_prof_data_parse.h"
namespace Visualize {
constexpr uint64_t HEX_BASE = 16;
bool CachelineHeatMap::ToJson(const std::string &outputPath)
{
    if (clidBasedCacheData_.empty()) {
        Utility::LogDebug("No cacheline data, cannot create cacheline heat map.");
        return false;
    }
    std::string dumpPath = Utility::JoinPath({outputPath, "dump"});
    if (!Utility::StoullConverter(Profiling::GetStartPcFromDump(dumpPath), startPc_, Utility::RADIX_16)) {
        Utility::LogWarn("Failed to convert start pc in cache line heat map");
        return false;
    }
    std::vector<nlohmann::json> cachelineJson;
    for (uint32_t i = 0U; i < clidBasedCacheData_.size(); i++) {
        nlohmann::json setsJson;
        setsJson["Cacheline Id"] = i;
        uint64_t totalTimes = clidBasedCacheData_[i].stat.hit + clidBasedCacheData_[i].stat.miss;
        float hitRate = (totalTimes != 0) ?
            (static_cast<float>(clidBasedCacheData_[i].stat.hit) / static_cast<float>(totalTimes)) : 0;
        nlohmann::json hitCacheData;
        hitCacheData["Value"] = std::pair<uint32_t, float>(clidBasedCacheData_[i].stat.hit, hitRate);
        hitCacheData["Address Range"] = Utility::MergeAddrRange(clidBasedCacheData_[i].hitPc, startPc_);
        setsJson["Hit"] = hitCacheData;

        float missRate = (totalTimes != 0) ?
            (static_cast<float>(clidBasedCacheData_[i].stat.miss) / static_cast<float>(totalTimes)) : 0;
        nlohmann::json missCacheData;
        missCacheData["Value"] = std::pair<uint32_t, float>(clidBasedCacheData_[i].stat.miss, missRate);
        missCacheData["Address Range"] = Utility::MergeAddrRange(clidBasedCacheData_[i].missPc, startPc_);
        setsJson["Miss"] = missCacheData;
        cachelineJson.emplace_back(setsJson);
    }
    cacheLineHeatMapJson_["Cacheline Records"] = cachelineJson;
    return true;
}
}

