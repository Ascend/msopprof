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


#ifndef MSOPT_CACHELINE_HEAT_MAP_H
#define MSOPT_CACHELINE_HEAT_MAP_H

#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include "profiling/device/data_parse/l2cache/l2cache.h"
#include "json.hpp"
namespace Visualize {
class CachelineHeatMap {
public:
    explicit CachelineHeatMap(const std::shared_ptr<Profiling::L2Cache> &l2Cache_) : startPc_(0ULL)
    {
        if (l2Cache_ != nullptr) {
            clidBasedCacheData_ = l2Cache_->GetCIDBasedCacheData();
        }
    }
    bool ToJson(const std::string &outputPath);

    nlohmann::json GetCacheLineHeatMapJson()
    {
        return cacheLineHeatMapJson_;
    }
private:
    std::vector<Profiling::CacheMetrics> clidBasedCacheData_;
    nlohmann::json cacheLineHeatMapJson_;
    uint64_t startPc_;
};
}

#endif // MSOPT_CACHELINE_HEAT_MAP_H
