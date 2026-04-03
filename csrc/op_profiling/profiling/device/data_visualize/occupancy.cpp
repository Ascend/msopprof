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
        if (!metric.hasSimtMetrics) {
            return;
        }
        nlohmann::json simtMetricsJson;
        simtMetricsJson["instructions"] = this->metric.simtMetrics.instrNum;
        simtMetricsJson["instruction_per_cycle"] = static_cast<double>(this->metric.simtMetrics.instrNum) / this->metric.simtMetrics.cycles;
        jsonData["simt_vf_instructions"] = simtMetricsJson;
    }
    std::string subcoreType;
    std::string subcoreId;
    OccupancyMetrics metric;
};

void Occupancy::ClearOccupancyJson()
{
    metricsMap_.clear();
    fusionPmuMapDetails_.clear();
    blockIdCoreIdPairVec_.clear();
}

template<typename T>
static std::string JoinVectorElements(const std::vector<T>& elements, const std::string& delimiter) {
    std::ostringstream oss;
    for (auto it = elements.begin(); it != elements.end(); ++it) {
        if (it != elements.begin()) oss << delimiter;
        oss << *it;
    }
    return oss.str();
}

static std::string GetAdviceForOccupancyType(OccupancyDataType ocpy, const std::string &coreType)
{
    static const std::unordered_map<OccupancyDataType, std::string> ocpyMap = {
        {OccupancyDataType::OCPY_CYCLES, "take more time than other "},
        {OccupancyDataType::OCPY_THROUGHPUT, "write/read more data than other "},
        {OccupancyDataType::OCPY_CACHE_HIT_RATE, "cache hit rate lower than other "},
        {OccupancyDataType::OCPY_SIMT_INSTR, "execute more instructions than other "},
    };
    auto it = ocpyMap.find(ocpy);
    if (it == ocpyMap.end()) {
        return "";
    }
    return it->second + coreType + " cores";
}

static void SummarizeCubeReport(const std::vector<uint16_t> &cubeCoreReport, OccupancyDataType ocpy, std::vector<std::string> &adviceList) {
    if (cubeCoreReport.empty()) {
        return;
    }
    std::ostringstream adviceOss;
    adviceOss << "cube0 of core[";
    adviceOss << JoinVectorElements(cubeCoreReport, ",");
    adviceOss << "] ";
    adviceOss << GetAdviceForOccupancyType(ocpy, "cube");
    adviceList.emplace_back(adviceOss.str());
}

static void SummarizeVecReport(const PairVector<uint16_t, uint16_t> &vecCoreReport, OccupancyDataType ocpy, std::vector<std::string> &adviceList) {
    if (vecCoreReport.empty()) {
        return;
    }
    std::map<uint16_t, uint16_t> vecCoreMap;
    for (std::pair<uint16_t, uint16_t> idPair : vecCoreReport) {
        vecCoreMap[idPair.first] += idPair.second == 0 ? 1 : 2;
    }
    std::vector<uint16_t> subCore0;
    std::vector<uint16_t> subCore1;
    std::vector<uint16_t> subCore2;
    for (auto it = vecCoreMap.begin(); it != vecCoreMap.end(); ++it) {
        if (it->second == 1) {
            subCore0.emplace_back(it->first);
        } else if (it->second == 2) {
            subCore1.emplace_back(it->first);
        } else {
            subCore2.emplace_back(it->first);
        }
    }
    PairVector<RefOf<std::vector<uint16_t>>, std::string> subCorePairs = {
        {subCore2, "vector0, vector1 of core["},
        {subCore0, "vector0 of core["},
        {subCore1, "vector1 of core["},
    };
    std::vector<std::string> subCoreNameList;
    for (auto it = subCorePairs.begin(); it != subCorePairs.end(); ++it) {
        if (it->first.get().empty()) {
            continue;
        }
        std::ostringstream oss;
        oss << it->second;
        oss << JoinVectorElements(it->first.get(), ",");
        oss << "]";
        subCoreNameList.emplace_back(oss.str());
    }
    std::ostringstream adviceOss;
    adviceOss << JoinVectorElements(subCoreNameList, ", ");
    adviceOss << " ";
    adviceOss << GetAdviceForOccupancyType(ocpy, "vector");
    adviceList.emplace_back(adviceOss.str());
}

static double ZScore(double x, double average, double stdDeviationInverse)
{
    return (x - average) * stdDeviationInverse;
}

static double Sigmoid(double x, bool reflect) {
    return 1.0 / (1.0 + exp(reflect ? x : -x));
}

const std::unordered_map<OccupancyDataType, double> Occupancy::ReportThresholds = {
    {OccupancyDataType::OCPY_CYCLES, 0.6},
    {OccupancyDataType::OCPY_THROUGHPUT, 0.6},
    {OccupancyDataType::OCPY_CACHE_HIT_RATE, 0.6},
    {OccupancyDataType::OCPY_SIMT_INSTR, 0.6},
};

void Occupancy::AnalyzeOccupy(PairVector<std::string, double> &Ocpy, OccupancyDataType OcpyType,
                              std::vector<std::string> &adviceList) const {

    if (ReportThresholds.find(OcpyType) == ReportThresholds.end()) {
        return;
    }
    std::smatch matchRes;
    uint16_t blockId = 0;
    uint16_t subBlockId = 0;
    std::vector<uint16_t> cubeCoreReport;
    PairVector<uint16_t, uint16_t> vecCoreReport;
    for (auto it = Ocpy.begin(); it != Ocpy.end(); ++it) {
        if (!std::regex_search(it->first, matchRes, pattern_)) {
            continue;
        }
        std::string blockIdStr = matchRes[1];    // 1表示blockId
        std::string blockTypeStr = matchRes[2];  // 2 表示block类型 cube vector
        std::string subBlockIdStr = matchRes[3]; // 3 表示subblockId
        if (!Utility::StringToNum(blockIdStr, blockId) || !Utility::StringToNum(subBlockIdStr, subBlockId)) {
            continue;
        }
        uint16_t coreId = (opType_ == Common::OpType::VECTOR) ? blockId / 2 : blockId;
        uint16_t subCoreId = (opType_ == Common::OpType::VECTOR) ? blockId % 2 : subBlockId;
        if (it->second < ReportThresholds.at(OcpyType)) {
            continue;
        }
        if (blockTypeStr == "cube") {
            cubeCoreReport.emplace_back(coreId);
        } else {
            vecCoreReport.emplace_back(coreId, subCoreId);
        }
    }
    std::sort(cubeCoreReport.begin(), cubeCoreReport.end());
    SummarizeCubeReport(cubeCoreReport, OcpyType, adviceList);
    SummarizeVecReport(vecCoreReport, OcpyType, adviceList);
}

bool Occupancy::NormalizeOccupy(PairVector<std::string, double> &Ocpy, OccupancyDataType OcpyType) const {
    if (Ocpy.empty()) {
        return false;
    }
    double numInverse = 1.0 / Ocpy.size();
    double average = 0.0;
    for (auto it = Ocpy.begin(); it != Ocpy.end(); ++it) {
        average += it->second;
    }
    average *= numInverse;
    double variance = 0.0;
    for (auto it = Ocpy.begin(); it != Ocpy.end(); ++it) {
        variance += (it->second - average) * (it->second - average);
    }
    variance *= numInverse;
    if (IsZero(variance)) {
        return false;
    }
    double stdDeviationInverse = 1.0 / std::sqrt(variance);
    for (auto it = Ocpy.begin(); it != Ocpy.end(); ++it) {
        // 对cache hit rate的zScore取反，让分数与缓存命中率反相关
        it->second = Sigmoid(ZScore(it->second, average, stdDeviationInverse), OcpyType == OccupancyDataType::OCPY_CACHE_HIT_RATE);
    }
    return true;
}

std::string Occupancy::GetAdviceFromOcpyData(const PairVector<RefOf<PairVector<std::string, double>>, OccupancyDataType> &ocpyData) const {
    std::vector<std::string> ocpySummary;
    for (auto it = ocpyData.begin(); it != ocpyData.end(); ++it) {
        if (!NormalizeOccupy(it->first.get(), it->second)) {
            continue;
        }
        AnalyzeOccupy(it->first.get(), it->second, ocpySummary);
    }
    if (ocpySummary.empty()) {
        return "";
    }
    std::ostringstream summaryOss;
    int summaryIndex = 1;
    for (auto &advice : ocpySummary) {
        summaryOss << "\t" << summaryIndex << ") " << advice << ".\n";
        summaryIndex++;
    }
    std::string summary = summaryOss.str();
    Utility::LogSummary("Occupancy Summary Report:\n\n%s", summary.c_str());
    return summary;
}

std::string Occupancy::GetAdvice() const {
    PairVector<std::string, double> cyclesOccupancyVec;
    PairVector<std::string, double> throughputOccupancyVec;
    PairVector<std::string, double> cacheHitRateOccupancyVec;
    PairVector<std::string, double> simtInstrOccupancyVec;
    PairVector<std::string, double> cyclesOccupancyCube;
    PairVector<std::string, double> throughputOccupancyCube;
    PairVector<std::string, double> cacheHitRateOccupancyCube;

    // 建立对比数据结构
    std::regex vecPattern("[0-9]{1,2}vector[0,1]");
    std::regex cubePattern("[0-9]{1,2}cube0");
    for (const auto &metrics : metricsMap_) {
        std::string coreName = metrics.first;
        if (std::regex_match(coreName, vecPattern)) {
            cyclesOccupancyVec.emplace_back(coreName, metrics.second.cycles);
            throughputOccupancyVec.emplace_back(coreName, metrics.second.throughput);
            cacheHitRateOccupancyVec.emplace_back(coreName, metrics.second.cacheHitRate);
            if (metrics.second.hasSimtMetrics) {
                simtInstrOccupancyVec.emplace_back(coreName, metrics.second.simtMetrics.instrNum);
            }
        } else if (std::regex_match(coreName, cubePattern)) {
            cyclesOccupancyCube.push_back({coreName, metrics.second.cycles});
            throughputOccupancyCube.push_back({coreName, metrics.second.throughput});
            cacheHitRateOccupancyCube.push_back({coreName, metrics.second.cacheHitRate});
        }
    }
    // 判断cycles是否负载均衡，并给出结论
    std::vector<std::string> ocpySummary;
    PairVector<RefOf<PairVector<std::string, double>>, OccupancyDataType> ocpyData = {
        {cyclesOccupancyVec, OccupancyDataType::OCPY_CYCLES},
        {cyclesOccupancyCube, OccupancyDataType::OCPY_CYCLES},
        {throughputOccupancyVec, OccupancyDataType::OCPY_THROUGHPUT},
        {throughputOccupancyCube, OccupancyDataType::OCPY_THROUGHPUT},
        {cacheHitRateOccupancyVec, OccupancyDataType::OCPY_CACHE_HIT_RATE},
        {cacheHitRateOccupancyCube, OccupancyDataType::OCPY_CACHE_HIT_RATE},
        {simtInstrOccupancyVec, OccupancyDataType::OCPY_SIMT_INSTR},
    };
    return GetAdviceFromOcpyData(ocpyData);
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
    return OccupancyMetrics{totalCycles, throughput, cacheHitRate, false};
}

OccupancyMetrics OccupancyA5::GetSubBlockData(const std::map<uint64_t, uint64_t> &eventMap, uint64_t totalCycles)
{
    // cube与vector核公式一致，eventMap不存在的pmu默认置0
    auto pmu = eventMap;
    std::string location = "occupancy";
    uint64_t readThroughput = GetDataNumber(pmu[EVENT_GM_MAIN_READ_A5], REQ_DATA_OF_A5.at(TransportType::READ_MAIN_MEMORY));
    uint64_t writeThroughput = GetDataNumber(pmu[EVENT_GM_MAIN_WRITE_A5], REQ_DATA_OF_A5.at(TransportType::WRITE_MAIN_MEMORY));
    uint64_t throughput = SafeAdd(readThroughput, writeThroughput, location);
    auto hit = SafeAddAll<uint64_t>({pmu[EVENT_READ_CLOSE_HIT],
                                    pmu[EVENT_READ_FAR_HIT],
                                    pmu[EVENT_WRITE_CLOSE_HIT],
                                    pmu[EVENT_WRITE_FAR_HIT]}, location);
    auto total = SafeAddAll<uint64_t>({pmu[EVENT_READ_CLOSE_HIT],  pmu[EVENT_READ_CLOSE_MISS],  pmu[EVENT_READ_CLOSE_VICTIM],
                                      pmu[EVENT_READ_FAR_HIT],    pmu[EVENT_READ_FAR_MISS],    pmu[EVENT_READ_FAR_VICTIM],
                                      pmu[EVENT_WRITE_CLOSE_HIT], pmu[EVENT_WRITE_CLOSE_MISS], pmu[EVENT_WRITE_CLOSE_VICTIM],
                                      pmu[EVENT_WRITE_FAR_HIT],   pmu[EVENT_WRITE_FAR_MISS],   pmu[EVENT_WRITE_FAR_VICTIM]}, location);
    float cacheHitRate = 0;
    if (total > 0) {
        cacheHitRate = static_cast<float>(PERCENTAGE_CONVERSION * hit) / total;
    }
    OccupancyMetrics metrics{};
    metrics.cycles = totalCycles;
    metrics.throughput = throughput;
    metrics.cacheHitRate = cacheHitRate;
    uint64_t simtCycles = pmu[PMU_IDC_AIV_VEC_INSTR_SIMT_VF_BUSY_O];
    if (simtCycles == 0) {
        metrics.hasSimtMetrics = false;
        return metrics;
    }
    uint64_t instrNum = SafeAddAll<uint64_t>({pmu[WSU_PMU_EXU_INS_ISSUE],
                                             pmu[WSU_PMU_DVG_INS_ISSUE],
                                             pmu[WSU_PMU_DCU_INS_ISSUE]}, location);
    metrics.hasSimtMetrics = true;
    metrics.simtMetrics.cycles = simtCycles;
    metrics.simtMetrics.instrNum = instrNum;
    return metrics;
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
