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

#include "occupancy.h"

#include <limits>
#include <algorithm>
#include "ustring.h"
#include "json.hpp"
#include "log.h"
#include "data_visualize_const.h"
#include "common/defs.h"
#include "number_operation.h"

using namespace Common;
using namespace Profiling;
using namespace Utility;

namespace Visualize {
struct OccupancyEvent {
    inline void ToJson(nlohmann::json &jsonData) const
    {
        jsonData["subcore_type"] = this->subcoreType;
        jsonData["subcore_id"] = this->subcoreId;
        jsonData["throughput"] = std::to_string(this->metric.throughput);
        jsonData["cycles"] = std::to_string(this->metric.cycles);
        jsonData["L2cache_hit_rate"] = std::to_string(this->metric.cacheHitRate);
    }
    std::string subcoreType;
    std::string subcoreId;
    OccupancyMetrics metric;
};

template<typename T>
bool UpdataMaxAndMin(T &max, T &min, const std::vector<std::pair<std::string, T>> &Ocpy)
{
    for (const auto &metricsValue : Ocpy) {
        max = std::max(max, metricsValue.second);
        min = std::min(min, metricsValue.second);
    }
    return !IsZero<T>(max);
}

void Occupancy::ClearOccupancyJson()
{
    metricsMap_.clear();
    fusionPmuMapDetails_.clear();
    blockIdCoreIdPairVec_.clear();
}

template<typename T>
std::vector<std::string> Occupancy::AnalysisOccupy(std::vector<std::pair<std::string, T>> &Ocpy,
                                                   OccupancyDataType OcpyType)
{
    std::vector<std::string> adviceList;
    if (Ocpy.empty()) {
        return adviceList;
    }
    T maxOcpy = std::numeric_limits<T>::min();
    T minOcpy = std::numeric_limits<T>::max();
    if (!UpdataMaxAndMin<T>(maxOcpy, minOcpy, Ocpy)) {
        return adviceList;
    }
    // 最小值大于最大值*0.9，认为负载均衡
    if (static_cast<float>(minOcpy) / maxOcpy >= OCCUPANCY_BALANCE_THRESHOLD) {
        return adviceList;
    }

    // 收集分析结果
    std::smatch matchRes;
    uint16_t blockId = 0;
    uint16_t subBlockId = 0;
    for (auto &metricsValue : Ocpy) {
        // metricsValue pair 第一个字段表示核名字，第二个字段表示指标值
        if (std::regex_search(metricsValue.first, matchRes, pattern_)) {
            std::string blockIdStr = matchRes[1];    // 1表示blockId
            std::string blockTypeStr = matchRes[2];  // 2 表示block类型 cube vector
            std::string subBlockIdStr = matchRes[3]; // 3 表示subblockId

            if (!Utility::StringToNum(blockIdStr, blockId)) {
                continue;
            }
            if (!Utility::StringToNum(subBlockIdStr, subBlockId)) {
                continue;
            }
            
            uint16_t coreId = (opType_ == Common::OpType::VECTOR) ? blockId / 2 : blockId;
            uint16_t subCoreId = (opType_ == Common::OpType::VECTOR) ? blockId % 2 : subBlockId;
            if (OcpyType == OccupancyDataType::OCPY_CYCLES && SafeEqual(metricsValue.second, maxOcpy, T(0))) {
                adviceList.emplace_back("core" + std::to_string(coreId) + " " + blockTypeStr +
                    std::to_string(subCoreId) + " took more time than other " + blockTypeStr + " cores");
                continue;
            }
            if (OcpyType == OccupancyDataType::OCPY_THROUGHPUT && SafeEqual(metricsValue.second, maxOcpy, T(0))) {
                adviceList.emplace_back("core" + std::to_string(coreId) + " " + blockTypeStr +
                    std::to_string(subCoreId) + " write/read more data than other "+ blockTypeStr + " cores");
                continue;
            }
            if (OcpyType == OccupancyDataType::OCPY_CACHE_HIT_RATE && SafeEqual(metricsValue.second, minOcpy, T(0))) {
                adviceList.emplace_back("core" + std::to_string(coreId) + " " + blockTypeStr +
                    std::to_string(subCoreId) + " cache hit rate lower than other "+ blockTypeStr + " cores");
                continue;
            }
        }
    }
    return adviceList;
}

std::string Occupancy::GetAdvice()
{
    std::vector<std::pair<std::string, uint64_t>> cyclesOccupancyVec;
    std::vector<std::pair<std::string, float>> throughputOccupancyVec;
    std::vector<std::pair<std::string, float>> cacheHitRateOccupancyVec;
    std::vector<std::pair<std::string, uint64_t>> cyclesOccupancyCube;
    std::vector<std::pair<std::string, float>> throughputOccupancyCube;
    std::vector<std::pair<std::string, float>> cacheHitRateOccupancyCube;

    // 建立对比数据结构
    std::regex vecPattern("[0-9]{1,2}vector[0,1]");
    std::regex cubePattern("[0-9]{1,2}cube0");
    for (const auto &metrics : metricsMap_) {
        std::string coreName = metrics.first;
        if (std::regex_match(coreName, vecPattern)) {
            cyclesOccupancyVec.push_back({coreName, metrics.second.cycles});
            throughputOccupancyVec.push_back({coreName, metrics.second.throughput});
            cacheHitRateOccupancyVec.push_back({coreName, metrics.second.cacheHitRate});
        } else if (std::regex_match(coreName, cubePattern)) {
            cyclesOccupancyCube.push_back({coreName, metrics.second.cycles});
            throughputOccupancyCube.push_back({coreName, metrics.second.throughput});
            cacheHitRateOccupancyCube.push_back({coreName, metrics.second.cacheHitRate});
        }
    }
    // 判断cycles是否负载均衡，并给出结论
    std::vector<std::string> ocpySummary;
    std::vector<std::string> adviceList = AnalysisOccupy(cyclesOccupancyVec, OccupancyDataType::OCPY_CYCLES);
    ocpySummary.insert(ocpySummary.end(), adviceList.begin(), adviceList.end());

    adviceList = AnalysisOccupy(cyclesOccupancyCube, OccupancyDataType::OCPY_CYCLES);
    ocpySummary.insert(ocpySummary.end(), adviceList.begin(), adviceList.end());

    adviceList = AnalysisOccupy(throughputOccupancyVec, OccupancyDataType::OCPY_THROUGHPUT);
    ocpySummary.insert(ocpySummary.end(), adviceList.begin(), adviceList.end());

    adviceList = AnalysisOccupy(throughputOccupancyCube, OccupancyDataType::OCPY_THROUGHPUT);
    ocpySummary.insert(ocpySummary.end(), adviceList.begin(), adviceList.end());

    adviceList = AnalysisOccupy(cacheHitRateOccupancyVec, OccupancyDataType::OCPY_CACHE_HIT_RATE);
    ocpySummary.insert(ocpySummary.end(), adviceList.begin(), adviceList.end());

    adviceList = AnalysisOccupy(cacheHitRateOccupancyCube, OccupancyDataType::OCPY_CACHE_HIT_RATE);
    ocpySummary.insert(ocpySummary.end(), adviceList.begin(), adviceList.end());
    
    std::string summary;
    int summaryIndex = 1;
    if (ocpySummary.empty()) {
        return summary;
    }
    for (auto &advice : ocpySummary) {
        summary += "\t";
        summary += std::to_string(summaryIndex++);
        summary += ") ";
        summary += advice.c_str();
        summary += ".\n";
    }
    Utility::LogSummary("Occupancy Summary Report:\n\n" + summary);
    return summary;
}

bool Occupancy910B::CheckCacheHitEventMap(const std::map<uint64_t, uint64_t> &pmuEvents) const
{
    if (pmuEvents.count(EVENT_WRITE_CACHE_HIT) > 0 && pmuEvents.count(EVENT_WRITE_CACHE_MISS) > 0 &&
        pmuEvents.count(EVENT_READ_R0_CACHE_HIT) > 0 && pmuEvents.count(EVENT_READ_R0_CACHE_MISS) > 0 &&
        pmuEvents.count(EVENT_READ_R1_CACHE_HIT) > 0 && pmuEvents.count(EVENT_READ_R1_CACHE_MISS) > 0 &&
        pmuEvents.count(EVENT_READ_R1_CACHE_MISS_NO_ALLOCATE) > 0 &&
        pmuEvents.count(EVENT_READ_R0_CACHE_MISS_NO_ALLOCATE) > 0 &&
        pmuEvents.count(EVENT_WRITE_CACHE_MISS_NO_ALLOCATE) > 0) {
        return true;
    }
    return false;
}

OccupancyMetrics Occupancy910B::GetSubBlockData(const std::map<uint64_t, uint64_t> &eventMap, uint64_t totalCycles)
{
    // cube与vector核公式一致，eventMap不存在的pmu默认置0
    auto pmu = eventMap;
    std::string location = "occupancy";
    uint64_t gmPmu = SafeAddAll<uint64_t>({pmu[EVENT_GM_MAIN_WRITE], pmu[EVENT_GM_MAIN_READ_R0], pmu[EVENT_GM_MAIN_READ_R1]}, location);
    uint64_t throughput = SafeMul(gmPmu, THROUGHPUT_DATA_GRANULARITY, location); // 读写数据量

    float cacheHitRate = 0;
    if (CheckCacheHitEventMap(eventMap)) {
        uint64_t hit = SafeAddAll<uint64_t>({eventMap.at(EVENT_WRITE_CACHE_HIT),
            eventMap.at(EVENT_READ_R0_CACHE_HIT), eventMap.at(EVENT_READ_R1_CACHE_HIT)}, location);
        uint64_t total = SafeAddAll<uint64_t>({hit,
            eventMap.at(EVENT_WRITE_CACHE_MISS), eventMap.at(EVENT_WRITE_CACHE_MISS_NO_ALLOCATE),
            eventMap.at(EVENT_READ_R0_CACHE_MISS), eventMap.at(EVENT_READ_R0_CACHE_MISS_NO_ALLOCATE),
            eventMap.at(EVENT_READ_R1_CACHE_MISS), eventMap.at(EVENT_READ_R1_CACHE_MISS_NO_ALLOCATE)}, location);
        if (total > 0) {
            cacheHitRate = static_cast<float>(PERCENTAGE_CONVERSION * hit) / total;
        }
    }
    return OccupancyMetrics{totalCycles, throughput, cacheHitRate};
}

OccupancyMetrics OccupancyA5::GetSubBlockData(const std::map<uint64_t, uint64_t> &eventMap, uint64_t totalCycles)
{
    // cube与vector核公式一致，eventMap不存在的pmu默认置0
    auto pmu = eventMap;
    std::string location = "occupancy";
    uint64_t readThroughput = GetDataNumber(pmu[EVENT_GM_MAIN_READ_A5], REQ_DATA_OF_A5.at(TransportType::READ_MAIN_MEMORY));
    uint64_t writeThroughput = GetDataNumber(pmu[EVENT_GM_MAIN_WRITE_A5], REQ_DATA_OF_A5.at(TransportType::WRITE_MAIN_MEMORY));
    uint64_t throughput = SafeAdd(readThroughput, writeThroughput, location);
    uint64_t hit = SafeAddAll<uint64_t>({pmu[EVENT_READ_CLOSE_HIT_A5], pmu[EVENT_READ_FAR_HIT_A5],
                                        pmu[EVENT_WRITE_CLOSE_HIT_A5], pmu[EVENT_WRITE_FAR_HIT_A5]}, location);
    uint64_t total = SafeAddAll<uint64_t>({hit, pmu[EVENT_READ_MISS_A5], pmu[EVENT_WRITE_MISS_A5]}, location);
    float cacheHitRate = 0;
    if (total > 0) {
        cacheHitRate = static_cast<float>(PERCENTAGE_CONVERSION * hit) / total;
    }
    return OccupancyMetrics{totalCycles, throughput, cacheHitRate};
}

std::map<uint64_t, uint64_t> Occupancy::MergeEventMap(std::map<uint64_t, uint64_t> &eventMap1,
                                                      std::map<uint64_t, uint64_t> &eventMap2) const
{
    std::map<uint64_t, uint64_t> eventMap;
    for (auto &event : eventMap1) {
        uint64_t eventId = event.first;
        if (eventMap2.find(eventId) == eventMap2.end()) {
            continue;
        }
        eventMap[eventId] = SafeAdd(eventMap1[eventId], eventMap2[eventId], "occupancy");
    }
    return eventMap;
}

void Occupancy::MergeSameCorePmu(std::vector<MemMapDetail> &pmuMapDetails)
{
    std::sort(pmuMapDetails.begin(), pmuMapDetails.end(), [](MemMapDetail detailA, MemMapDetail detailB) {
        return detailA.blockId < detailB.blockId;
    });
    std::sort(blockIdCoreIdPairVec_.begin(), blockIdCoreIdPairVec_.end(),
              [](std::pair<uint16_t, uint16_t> pairA, std::pair<uint16_t, uint16_t> pairB) {
        return pairA.first < pairB.first;
    });
    // mix 算子不需要融合pmu
    if (pmuMapDetails[0].opType == Common::OpType::MIX) {
        fusionPmuMapDetails_ = pmuMapDetails;
        return;
    }
    // 相同phyCore的pmu数据融合到一个Block
    std::map<std::uint16_t, uint16_t> phyCoreIdToBlockId;   // 记录phycoreId对应的最小blockId，映射表

    // 遍历最后一次重放记录的blockId和对应phycoreId
    for (const auto &blockIdCoreId : blockIdCoreIdPairVec_) {
        uint16_t phyCoreId = blockIdCoreId.second;
        uint16_t blockId = blockIdCoreId.first;
        if (blockId >= pmuMapDetails.size()) {
            return;
        }
        // 若映射表中已经记录了phycoreId的blockId，则记录的blockId一定是最小的minBlockId，把当前block的pmu值叠加到minBlock上
        if (phyCoreIdToBlockId.count(phyCoreId) != 0) {
            uint16_t minBlockId = phyCoreIdToBlockId[phyCoreId];
            if (minBlockId >= fusionPmuMapDetails_.size()) {
                return;
            }
            fusionPmuMapDetails_[minBlockId].totalCycles = Utility::SafeAdd(fusionPmuMapDetails_[minBlockId].totalCycles,
            pmuMapDetails[blockId].totalCycles, "merge cycle");
            fusionPmuMapDetails_[minBlockId].eventMap =
                MergeEventMap(fusionPmuMapDetails_[minBlockId].eventMap, pmuMapDetails[blockId].eventMap);
        } else {
            phyCoreIdToBlockId[phyCoreId] = blockId;
            fusionPmuMapDetails_.emplace_back(pmuMapDetails[blockId]);
        }
    }
}

std::vector<nlohmann::json> Occupancy::GetOccupancyBlockJson(MemMapDetail &memMapDetail)
{
    std::vector<nlohmann::json> occupancyBlockJson;
    if (memMapDetail.opType != Common::OpType::MIX) {
        auto subBlockData = GetSubBlockData(memMapDetail.eventMap, memMapDetail.totalCycles);
        metricsMap_[std::to_string(memMapDetail.blockId) + memMapDetail.blockType] = subBlockData;
        OccupancyEvent event = {memMapDetail.opType, "0", subBlockData};
        nlohmann::json occupancyBlockItem;
        event.ToJson(occupancyBlockItem);
        occupancyBlockJson.emplace_back(occupancyBlockItem);
        return occupancyBlockJson;
    }

    std::vector<OccupancyEvent> events;
    auto cubeData = GetSubBlockData(memMapDetail.eventMap, memMapDetail.cycMap["Cube"]);
    metricsMap_[std::to_string(memMapDetail.blockId) + "cube0"] = cubeData;
    events.emplace_back(OccupancyEvent{"cube", "0", cubeData});
    auto vector0Data = GetSubBlockData(memMapDetail.eventMapVec0, memMapDetail.cycMap["Vector"]);
    metricsMap_[std::to_string(memMapDetail.blockId) + "vector0"] = vector0Data;
    events.emplace_back(OccupancyEvent{"vector", "0", vector0Data});
    auto vector1Data = GetSubBlockData(memMapDetail.eventMapVec1, memMapDetail.cycMap["Vector1"]);
    metricsMap_[std::to_string(memMapDetail.blockId) + "vector1"] = vector1Data;
    events.emplace_back(OccupancyEvent{"vector", "1", vector1Data});
    for (const auto &event : events) {
        nlohmann::json occupancyBlockItem;
        event.ToJson(occupancyBlockItem);
        occupancyBlockJson.emplace_back(occupancyBlockItem);
    }
    return occupancyBlockJson;
}

std::vector<nlohmann::json> Occupancy::GetOccupancyJson()
{
    std::vector<nlohmann::json> occupancyJson;
    std::vector<MemMapDetail> memMapDetails = basicPmuObj_->GetMemMapDetails();
    if (memMapDetails.empty()) {
        return occupancyJson;
    }
    MergeSameCorePmu(memMapDetails);
    for (auto &fusionPmuMapDetail : fusionPmuMapDetails_) {
        nlohmann::json occupancyItem;
        occupancyItem["core_id"] = fusionPmuMapDetail.blockId;
        occupancyItem["core_detail"] = GetOccupancyBlockJson(fusionPmuMapDetail);
        occupancyJson.emplace_back(occupancyItem);
    }
    return occupancyJson;
}

bool Occupancy::GetOccupancyMap(nlohmann::json &occupancyMapJson)
{
    opType_ = opBasicInfoObj_->GetOpType();
    occupancyMapJson["soc"] = opBasicInfoObj_->GetSoc();
    occupancyMapJson["op_type"] = opType_;
    occupancyMapJson["op_detail"] = GetOccupancyJson();
    occupancyMapJson["advice"] = GetAdvice();
    return true;
}
}
