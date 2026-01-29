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

#include "storage_access.h"

#include "common/defs.h"
#include "data_visualize_const.h"

#include <cmath>
#include <limits>
#include <algorithm>
#include "json.hpp"
#include "log.h"
#include "smart_pointer.h"
#include "common/visualize.h"
#include "number_operation.h"

using namespace Common;
using namespace Profiling;
using namespace Utility;
using namespace std;
namespace Visualize {

std::vector<nlohmann::json> StorageAccess::GetAiCoreMemTableJson(std::map<std::string, uint64_t> &cycMap)
{
    std::vector<nlohmann::json> table;
    auto head = GenTableHead();
    for (const auto &aiCoreTable : tableLineAiCore_) {
        nlohmann::json t;
        t["table_name"] = aiCoreTable.first;
        // 行数为vector的长度，列数这里可以另外新建一个map，但是对于aiCoreTable，只有gm的列为2
        t["size"] = {aiCoreTable.second.size(), head.count(aiCoreTable.first) == 0 ? (head["Default"].size() - 1) :
            (head[aiCoreTable.first].size() - 1)};
        std::vector<nlohmann::json> headJson;
        auto headVec = (head.count(aiCoreTable.first) == 0) ? head["Default"] : head[aiCoreTable.first];

        for (size_t j = 0; j < headVec.size(); ++j) {
            headJson.emplace_back(headVec[j]);
        }
        t["header_name"] = headJson;
        std::vector<nlohmann::json> row;
        for (size_t j = 0; j < aiCoreTable.second.size(); ++j) {
            nlohmann::json r;
            r["name"] = aiCoreTable.second[j];
            auto tableRows = GetTableValue(aiCoreTable.first, j, cycMap, head);
            for (size_t i = 0; i < tableRows.size(); ++i) {
                r["value"].emplace_back(tableRows[i]);
            }
            row.emplace_back(r);
        }
        t["row"] = row;
        table.emplace_back(t);
    }
    return table;
}

std::vector<nlohmann::json> StorageAccess::GenPipeAdvice(string opType)
{
    // Measures the throughput and active cycles.
    std::vector<nlohmann::json> advice;
    std::map<std::string, std::string> pipeMap = { {"Pipe Cube MTE1", "aicore MTE1"},
        {"Pipe Cube MTE2", "aicore MTE2"}, {"Pipe Cube MTE3", "aicore MTE3"},
        {"Pipe Vector Core0 MTE1", "aivector MTE1"}, {"Pipe Vector Core0 MTE2", "aivector MTE2"},
        {"Pipe Vector Core0 MTE3", "aivector MTE3"}, {"Pipe Vector Core1 MTE1", "aivector MTE1"},
        {"Pipe Vector Core1 MTE2", "aivector MTE2"}, {"Pipe Vector Core1 MTE3", "aivector MTE3"},
        {"Pipe MTE1", "MTE1"}, {"Pipe MTE2", "MTE2"}, {"Pipe MTE3", "MTE3"},
    };
    // Generate optimization suggestions
    for (auto &pipe : peakMap_) {
        if (std::fabs(activeRate_[pipe.first]) <= std::numeric_limits<float>::epsilon()) {
            continue;
        }
        if (pipe.second / activeRate_[pipe.first] < idealRatio) {
            advice.emplace_back(pipeMap[pipe.first] + " bandwidth utilization lower than 80% when active");
            adviceInfoRecord_.insert(pipeMap[pipe.first] + " bandwidth utilization lower than 80%% when active");
        }
    }
    return advice;
}

void StorageAccess::GenCoreUsageAdvice(const std::string &coreName, float ratio,
                                       std::vector<nlohmann::json> &advice)
{
    static constexpr float aicoreUsageThreshold = 20.0f;  // aicore usage value is less than 20, the usage is low.
    std::map<std::string, std::string> aicoreMap = {
            {"Vector", "aivector"}, {"Vector1", "aivector"}, {"Cube", "aicore"}
    };
    if (ratio < aicoreUsageThreshold) {
        advice.emplace_back(aicoreMap[coreName] + " compute usage lower than 20%");
        adviceInfoRecord_.insert(aicoreMap[coreName] + " compute usage lower than 20%%");
    }
}

void StorageAccess::ShowAdviceInfo()
{
    if (adviceInfoRecord_.empty()) {
        return;
    }
    std::string summary;
    int summaryIndex = 1;
    for (auto &advice : adviceInfoRecord_) {
        summary += "\t" + std::to_string(summaryIndex++) + ") " + advice.c_str() + ".\n";
    }
    LogSummary("Performance Summary Report:\n\n" + summary);
    adviceInfoRecord_.clear();
}

void StorageAccess::StorageAccessToJson(bool isKernelScale, bool isMemoryDetail)
{
    PreProcess(isKernelScale, isMemoryDetail);
    StorageAccessHeatMap_["core_memory_map"] = heatMapJson_;
    StorageAccessTable_["table_per_block"] = mapTableJson_;
}

void StorageAccess::ClearStorageAccessJson()
{
    heatMapJson_.clear();
    mapTableJson_.clear();
    basicPmuObj_->GetMemMapDetails().clear();
    memInfoAiCoreMap_.clear();
    memInfoCacheMap_.clear();
    memInfoPipeMap_.clear();
    tableLineAiCore_.clear();
    activeRate_.clear();
    peakMap_.clear();
    StorageAccessHeatMap_.clear();
    StorageAccessTable_.clear();
}

// 以下为StorageAccess派生类的函数定义
std::map<std::string, uint64_t> StorageAccess910B::GetDataCube910B(std::map<uint64_t, uint64_t> &eventMap) const
{
    std::string location = "cube data";
    uint64_t mteGmRead = SafeSub(eventMap[50], eventMap[518], location);
    // eventMap[49]-4*eventMap[19]+eventMap[524]-eventMap[518]
    uint64_t l0aL0bWrite = SafeSub(SafeAdd(eventMap[49], eventMap[524], location),
        SafeAdd(4 * eventMap[19], eventMap[518], location), location);
    // 4*eventMap[19]-eventMap[524]+eventMap[518]
    uint64_t l1GmWrite = SafeSub(SafeAdd(4 * eventMap[19], eventMap[518], location), eventMap[524], location);
    uint64_t gmWrite = SafeSub(eventMap[524], eventMap[518], location);
    uint64_t maxValue = std::numeric_limits<uint64_t>::max();
    std::map<std::string, uint64_t> dataCube = {
        {"MTE/GM Read", mteGmRead == maxValue ? EMPTY_PMU_VALUE : mteGmRead},
        {"L0A/L0B Write", l0aL0bWrite == maxValue ? EMPTY_PMU_VALUE : l0aL0bWrite},
        {"L1_L0C Read", eventMap[518]},
        {"L1 GM Write", l1GmWrite == maxValue ? EMPTY_PMU_VALUE : l1GmWrite},
        {"L0C Read", eventMap[40]},
        {"GM Write", gmWrite == maxValue ? EMPTY_PMU_VALUE : gmWrite},
        {"MTE Write", eventMap[49]},
        {"L0A Read", eventMap[28]},
        {"L0A Cube Write", eventMap[27]},
        {"L0B Read", eventMap[34]},
        {"L0B Cube Write", eventMap[33]},
        {"L0C Write", eventMap[42]},
        {"L0C READ", eventMap[524]},
        {"Scalar Mte1 Stall", eventMap[107]},
        {"Scalar Cube Stall", eventMap[110]},
    };
    return dataCube;
}

std::map<std::string, uint64_t> StorageAccess910B::GetDataVector910BMix(MemMapDetail &memMapDetail) const
{
    std::map<std::string, uint64_t> dataVectors = {
        {"UB MTE Read",                     memMapDetail.eventMapVec0[62]},
        {"UB MTE Write",                    memMapDetail.eventMapVec0[61]},
        {"UB Vec Read",                     memMapDetail.eventMapVec0[67]},
        {"UB Vec Write",                    memMapDetail.eventMapVec0[68]},
        {"Scalar Read",                     memMapDetail.eventMapVec0[55]},
        {"Scalar Write",                    memMapDetail.eventMapVec0[56]},
        {"UB MTE Read1",                    memMapDetail.eventMapVec1[62]},
        {"UB MTE Write1",                   memMapDetail.eventMapVec1[61]},
        {"UB Vec Read1",                    memMapDetail.eventMapVec1[67]},
        {"UB Vec Write1",                   memMapDetail.eventMapVec1[68]},
        {"Scalar Read1",                    memMapDetail.eventMapVec1[55]},
        {"Scalar Write1",                   memMapDetail.eventMapVec1[56]},
    };
    LoadScalarMixPmu(" Vec0", memMapDetail.eventMapVec0, dataVectors);
    LoadScalarMixPmu(" Vec1", memMapDetail.eventMapVec1, dataVectors);
    return dataVectors;
}

void StorageAccess910B::LoadScalarMixPmu(const std::string &core, std::map<uint64_t, uint64_t> &pmuMap, std::map<std::string, uint64_t> &indexMap) const
{
    std::vector<std::string> index = {"Scalar Time", "Scalar Single", "Scalar Dual", "Scalar Mte2 Stall", "Scalar Mte3 Stall", "Scalar Vector Stall", "Scalar Ub Stall",
        "Scalar Wait IB", "Scalar Wait", "Scalar Internuclear ID0", "Scalar Internuclear ID1", "Scalar Internuclear ID2", "Scalar Internuclear ID3", "Scalar Internuclear ID4", 
        "Scalar Internuclear ID5", "Scalar Internuclear ID6", "Scalar Internuclear ID7", "Scalar Internuclear ID8", "Scalar Internuclear ID9", 
        "Scalar Internuclear ID10", "Scalar Internuclear ID11", "Scalar Internuclear ID12", "Scalar Internuclear ID13", "Scalar Internuclear ID14", "Scalar Internuclear ID15", 
    };
    std::vector<uint64_t> pmuValue = {9, 112, 113, 108, 109, 111, 106, 114, 87, 1792, 1793, 1794, 1795, 1796, 1797, 1798, 1799, 1780, 1781, 1782, 1783, 1784, 1785, 1786, 1787};
    for (uint32_t i = 0; i < index.size(); i++) {
        indexMap[index[i] + core] = pmuMap[pmuValue[i]];
    }
}

void StorageAccess910B::AddBasicPmu910B(const std::string &opType, MemMapDetail &memMapDetail,
                                        std::map<std::string, uint64_t> &basicPmu) const
{
    // eventMap的key为event id,value为其值
    // 对于mix算子，其vector core上的数据会预处理跟cube core在同一份memMapDetail中，表示vector core0和vector core1
    std::map<uint64_t, uint64_t> eventMap = memMapDetail.eventMap;
    std::map<uint64_t, uint64_t> eventMapVec0 = memMapDetail.eventMapVec0;
    std::map<uint64_t, uint64_t> eventMapVec1 = memMapDetail.eventMapVec1;
    std::vector<uint64_t> l2CacheWriteHit{eventMap[1280], eventMapVec0[1280], eventMapVec1[1280]};
    std::vector<uint64_t> l2CacheReadHit{eventMap[1284], eventMap[1288], eventMapVec0[1284],
        eventMapVec0[1288], eventMapVec1[1284], eventMapVec1[1288]};
    std::vector<uint64_t> l2CacheWriteMiss{eventMap[1282], eventMapVec0[1282], eventMapVec1[1282]};
    std::vector<uint64_t> l2CacheReadMiss{eventMap[1286], eventMap[1290], eventMapVec0[1286],
        eventMapVec0[1290], eventMapVec1[1286], eventMapVec1[1290]};
    std::vector<uint64_t> GmReadTotal{eventMap[1286], eventMap[1290], eventMapVec0[1286], eventMapVec0[1290],
        eventMapVec1[1286], eventMapVec1[1290]};
    std::vector<uint64_t> GmWriteTotal{eventMap[1282], eventMapVec0[1282], eventMapVec1[1282]};
    std::string location = "add basic pmu";
    uint64_t l2CacheWriteMissCount = SafeAddAll(l2CacheWriteMiss, location);
    uint64_t l2CacheRealWrite = l2CacheWriteMissCount;
    if (basicPmuObj_->GetL2cacheEvict() >= 0 && basicPmuObj_->GetComputeLoadBlockDetail().size() != 0) {
        uint32_t blockNum = opType == Common::OpType::MIX ? 3 : 1;
        uint64_t l2cacheEvict = static_cast<uint64_t>(basicPmuObj_->GetL2cacheEvict()) / (basicPmuObj_->GetComputeLoadBlockDetail().size() / blockNum);
        l2CacheRealWrite = std::min(l2CacheWriteMissCount, l2cacheEvict);
    }
    std::map<std::string, uint64_t> basic910B = {
        {"L2Cache Write hit", SafeAddAll(l2CacheWriteHit, location)},
        {"L2Cache Read hit", SafeAddAll(l2CacheReadHit, location)},
        {"L2Cache Write miss", l2CacheWriteMissCount},
        {"L2Cache Read miss", SafeAddAll(l2CacheReadMiss, location)},
        {"MTE1 Cyc", eventMap[770]}, {"MTE1 Cyc vec0", eventMapVec0[770]}, {"MTE1 Cyc vec1", eventMapVec1[770]},
        {"FIXP Ins", eventMap[526]}, {"FIXP Ins vec0", eventMapVec0[526]}, {"FIXP Ins vec1", eventMapVec1[526]},
        {"FIXP Cyc", eventMap[771]}, {"FIXP Cyc vec0", eventMapVec0[771]}, {"FIXP Cyc vec1", eventMapVec1[771]},
        {"GM Read Total", SafeAddAll(GmReadTotal, location)},
        {"GM Write Total", l2CacheRealWrite},
        {"Scalar Time", eventMap[9]},
        {"Scalar Single", eventMap[112]},
        {"Scalar Dual", eventMap[113]},
        {"Scalar Mte2 Stall", eventMap[108]},
        {"Scalar Mte3 Stall", eventMap[109]},
        {"Scalar Wait IB", eventMap[114]},
        {"Scalar Wait", eventMap[87]},
        {"Scalar Internuclear ID0", eventMap[1792]}, {"Scalar Internuclear ID1", eventMap[1793]}, {"Scalar Internuclear ID2", eventMap[1794]},
        {"Scalar Internuclear ID3", eventMap[1795]}, {"Scalar Internuclear ID4", eventMap[1796]}, {"Scalar Internuclear ID5", eventMap[1797]},
        {"Scalar Internuclear ID6", eventMap[1798]}, {"Scalar Internuclear ID7", eventMap[1799]}, {"Scalar Internuclear ID8", eventMap[1780]},
        {"Scalar Internuclear ID9", eventMap[1781]}, {"Scalar Internuclear ID10", eventMap[1782]}, {"Scalar Internuclear ID11", eventMap[1783]},
        {"Scalar Internuclear ID12", eventMap[1784]}, {"Scalar Internuclear ID13", eventMap[1785]}, {"Scalar Internuclear ID14", eventMap[1786]}, {"Scalar Internuclear ID15", eventMap[1787]},
        };
    basicPmu.insert(basic910B.begin(), basic910B.end());
    if (opType == Common::OpType::VECTOR) {
        std::map<std::string, uint64_t> dataVector = {
            {"UB MTE Read", eventMap[62]}, {"UB MTE Write", eventMap[61]}, {"UB Vec Read", eventMap[67]},
            {"UB Vec Write", eventMap[68]}, {"Scalar Read", eventMap[55]}, {"Scalar Write", eventMap[56]},
            {"Scalar Vector Stall", eventMap[111]}, {"Scalar Ub Stall", eventMap[106]},
        };
        basicPmu.insert(dataVector.begin(), dataVector.end());
    } else if (opType == Common::OpType::CUBE) {   // 910B vector
        std::map<std::string, uint64_t> dataCube = GetDataCube910B(eventMap);
        basicPmu.insert(dataCube.begin(), dataCube.end());
    } else {
        std::map<std::string, uint64_t> dataCube = GetDataCube910B(eventMap);
        basicPmu.insert(dataCube.begin(), dataCube.end());
        std::map<std::string, uint64_t> dataVectors = GetDataVector910BMix(memMapDetail);
        basicPmu.insert(dataVectors.begin(), dataVectors.end());
    }
}

float StorageAccess::GetDurCalBandWidth()
{
    auto dur = opBasicInfoObj_->GetDuration();
    if (std::fabs(dur) <= std::numeric_limits<float>::epsilon()) {
        LogError("Get op basic info [Task Duration] = %f failed.", dur);
        return 1.0f;
    }
    return dur;
}

std::map<std::string, std::vector<MemInfoAiCore>> StorageAccess910B::GetMemInfoAiCoreMapCube910B(std::map<std::string,
    uint64_t> &basicPmu, Profiling::Calculate &cal) const
{
    float l1L0cR = FunctionsFp.at(FuncType::BW_USAGE_REQ_FP)(cal, {basicPmu["L1_L0C Read"], 128});
    std::string l1L0cUsageR = Functions.at(FuncType::TRANSPORT_BW_USAGE_RATE)(cal, {basicPmu["L1_L0C Read"], 4});
    float mteGmR = basicPmu["MTE/GM Read"] == EMPTY_PMU_VALUE ? 0.0f :
        FunctionsFp.at(FuncType::BW_USAGE_REQ_FP)(cal, {basicPmu["MTE/GM Read"], 256});
    std::string mteGmUsageR = basicPmu["MTE/GM Read"] == EMPTY_PMU_VALUE ? "NA" :
        Functions.at(FuncType::TRANSPORT_BW_USAGE_RATE)(cal, {basicPmu["MTE/GM Read"], 2});
    float mteW = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, basicPmu["MTE Write"]});
    float l0aL0bW = basicPmu["L0A/L0B Write"] == EMPTY_PMU_VALUE ? 0.0f :
        FunctionsFp.at(FuncType::BW_USAGE_REQ_FP)(cal, {basicPmu["L0A/L0B Write"], 256});
    std::string l0aL0bUsageW = basicPmu["L0A/L0B Write"] == EMPTY_PMU_VALUE ? "NA" :
        Functions.at(FuncType::TRANSPORT_BW_USAGE_RATE)(cal, {basicPmu["L0A/L0B Write"], 3});
    float l1GmW = basicPmu["L1 GM Write"] == EMPTY_PMU_VALUE ? 0.0f :
        FunctionsFp.at(FuncType::BW_USAGE_REQ_FP)(cal, {basicPmu["L1 GM Write"], 128});
    std::string l1GmUsage = basicPmu["L1 GM Write"] == EMPTY_PMU_VALUE ? "NA" :
        Functions.at(FuncType::TRANSPORT_BW_USAGE_RATE)(cal, {basicPmu["L1 GM Write"], 3});
    float l0aR = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, basicPmu["L0A Read"]});
    float l0aCubeW = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 16, basicPmu["L0A Cube Write"]});
    float l0bR = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 8, basicPmu["L0B Read"]});
    float l0bCubeW = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 16, basicPmu["L0B Cube Write"]});
    float l0cW = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 32, basicPmu["L0C Write"]});
    float l0cR = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 32, basicPmu["L0C Read"]});
    float gmW = basicPmu["GM Write"] == EMPTY_PMU_VALUE ? 0.0f :
        FunctionsFp.at(FuncType::BW_USAGE_REQ_FP)(cal, {basicPmu["GM Write"], 128});
    std::string gmUsageW = basicPmu["GM Write"] == EMPTY_PMU_VALUE ? "NA" :
        Functions.at(FuncType::TRANSPORT_BW_USAGE_RATE)(cal, {basicPmu["GM Write"], 5});
    std::map<std::string, std::vector<MemInfoAiCore>> memInfoAiCoreMapCube = {
        {"L1", { {MemInfoAiCore{basicPmu["MTE/GM Read"], mteGmR, mteGmUsageR}},
            {MemInfoAiCore{basicPmu["MTE Write"], mteW, "NA"}},
            {MemInfoAiCore{basicPmu["L0A/L0B Write"], l0aL0bW, l0aL0bUsageW}},
            {MemInfoAiCore{basicPmu["L1_L0C Read"], l1L0cR, l1L0cUsageR}},
            {MemInfoAiCore{basicPmu["L1 GM Write"], l1GmW, l1GmUsage}},
            {MemInfoAiCore{basicPmu["MTE/GM Read"], mteGmR, mteGmUsageR} } } },
        {"L0A", { {MemInfoAiCore{basicPmu["L0A Read"], l0aR, "NA"}},
            {MemInfoAiCore{basicPmu["L0A Cube Write"], l0aCubeW, "NA"}},
            {MemInfoAiCore{basicPmu["L0A Read"], l0aR, "NA"}} } },
        {"L0B", { {MemInfoAiCore{basicPmu["L0B Read"], l0bR, "NA"}},
            {MemInfoAiCore{basicPmu["L0B Cube Write"], l0bCubeW, "NA"}},
            {MemInfoAiCore{basicPmu["L0B Read"], l0bR, "NA"}} } },
        {"Cube", { {MemInfoAiCore{basicPmu["L0A Cube Write"], l0aCubeW, "NA"}},
            {MemInfoAiCore{basicPmu["L0B Cube Write"], l0bCubeW, "NA"}},
            {MemInfoAiCore{basicPmu["L0C Write"], l0cW, "NA"}}, {MemInfoAiCore{basicPmu["L0C Read"], l0cR, "NA"}} } },
        {"L0C", { {MemInfoAiCore{basicPmu["L0C Write"], l0cW, "NA"}}, {MemInfoAiCore{basicPmu["L0C Read"], l0cR, "NA"}},
            {MemInfoAiCore{basicPmu["GM Write"], gmW, gmUsageW}},
            {MemInfoAiCore{basicPmu["L1_L0C Read"], l1L0cR, l1L0cUsageR}} } } };
    return memInfoAiCoreMapCube;
}

std::map<std::string, std::vector<MemInfoAiCore>> StorageAccess910B::GetMemInfoAiCoreMapVec910B(std::map<std::string,
    uint64_t> &basicPmu, Profiling::Calculate &cal) const
{
    std::map<std::string, std::vector<MemInfoAiCore>> memInfoAiCoreMapVec = {
        {"UB", {
            {MemInfoAiCore{basicPmu["UB MTE Read"],
                FunctionsFp.at(FuncType::BW_USAGE_REQ_FP)(cal, {basicPmu["UB MTE Read"], 128}),
                Functions.at(FuncType::TRANSPORT_BW_USAGE_RATE)(cal, {basicPmu["UB MTE Read"], 6})}},
            {MemInfoAiCore{basicPmu["UB MTE Write"],
                FunctionsFp.at(FuncType::BW_USAGE_REQ_FP)(cal, {basicPmu["UB MTE Write"], 128}),
                Functions.at(FuncType::TRANSPORT_BW_USAGE_RATE)(cal, {basicPmu["UB MTE Write"], 7})}},
            {MemInfoAiCore{basicPmu["UB MTE Read"],
                FunctionsFp.at(FuncType::BW_USAGE_REQ_FP)(cal, {basicPmu["UB MTE Read"], 128}),
                Functions.at(FuncType::TRANSPORT_BW_USAGE_RATE)(cal, {basicPmu["UB MTE Read"], 6})}},
            {MemInfoAiCore{basicPmu["UB MTE Write"],
                FunctionsFp.at(FuncType::BW_USAGE_REQ_FP)(cal, {basicPmu["UB MTE Write"], 128}),
                Functions.at(FuncType::TRANSPORT_BW_USAGE_RATE)(cal, {basicPmu["UB MTE Write"], 7})}},
            {MemInfoAiCore{basicPmu["UB Vec Read"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, basicPmu["UB Vec Read"]}), "NA"}},
            {MemInfoAiCore{basicPmu["UB Vec Write"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, basicPmu["UB Vec Write"]}), "NA"}},
            {MemInfoAiCore{basicPmu["Scalar Read"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 1, basicPmu["Scalar Read"]}), "NA"}},
            {MemInfoAiCore{basicPmu["Scalar Write"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 1, basicPmu["Scalar Write"]}), "NA"}} } },
        {"Vector", {
            {MemInfoAiCore{basicPmu["UB Vec Write"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, basicPmu["UB Vec Write"]}), "NA"}},
            {MemInfoAiCore{basicPmu["UB Vec Read"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, basicPmu["UB Vec Read"]}), "NA"}} } }
    };
    return memInfoAiCoreMapVec;
}

std::map<std::string, std::vector<MemInfoAiCore>> StorageAccess910B::GetMemInfoAiCoreMapMix910B(std::map<std::string,
    uint64_t> &basicPmu, Profiling::Calculate &cal) const
{
    float ubMteR = FunctionsFp.at(FuncType::BW_USAGE_REQ_FP)(cal, {basicPmu["UB MTE Read"], 128});
    std::string ubMteUsageR = Functions.at(FuncType::TRANSPORT_BW_USAGE_RATE)(cal, {basicPmu["UB MTE Read"], 6});
    float ubMteW = FunctionsFp.at(FuncType::BW_USAGE_REQ_FP)(cal, {basicPmu["UB MTE Write"], 128});
    std::string ubMteUsageW = Functions.at(FuncType::TRANSPORT_BW_USAGE_RATE)(cal, {basicPmu["UB MTE Write"], 7});
    float ubVecR = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, basicPmu["UB Vec Read"]});
    float ubVecW = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, basicPmu["UB Vec Write"]});
    float ubMteR1 = FunctionsFp.at(FuncType::BW_USAGE_REQ_FP)(cal, {basicPmu["UB MTE Read1"], 128});
    std::string ubMteUsageR1 = Functions.at(FuncType::TRANSPORT_BW_USAGE_RATE)(cal, {basicPmu["UB MTE Read1"], 6});
    float ubMteW1 = FunctionsFp.at(FuncType::BW_USAGE_REQ_FP)(cal, {basicPmu["UB MTE Write1"], 128});
    std::string ubMteUsageW1 = Functions.at(FuncType::TRANSPORT_BW_USAGE_RATE)(cal, {basicPmu["UB MTE Write1"], 7});
    std::map<std::string, std::vector<MemInfoAiCore>> memInfoAiCoreMapVecs = {
        {"UB Core0", {
            {MemInfoAiCore{basicPmu["UB MTE Read"], ubMteR, ubMteUsageR}},
            {MemInfoAiCore{basicPmu["UB MTE Write"], ubMteW, ubMteUsageW}},
            {MemInfoAiCore{basicPmu["UB MTE Read"], ubMteR, ubMteUsageR}},
            {MemInfoAiCore{basicPmu["UB MTE Write"], ubMteW, ubMteUsageW}},
            {MemInfoAiCore{basicPmu["UB Vec Read"], ubVecR, "NA"}},
            {MemInfoAiCore{basicPmu["UB Vec Write"], ubVecW, "NA"}},
            {MemInfoAiCore{basicPmu["Scalar Read"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 1, basicPmu["Scalar Read"]}), "NA"}},
            {MemInfoAiCore{basicPmu["Scalar Write"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 1, basicPmu["Scalar Write"]}), "NA"}} } },
        {"Vector Core0", {
            {MemInfoAiCore{basicPmu["UB Vec Write"], ubVecW, "NA"}},
            {MemInfoAiCore{basicPmu["UB Vec Read"], ubVecR, "NA"}} } },
        {"UB Core1", {
            {MemInfoAiCore{basicPmu["UB MTE Read1"], ubMteR1, ubMteUsageR1}},
            {MemInfoAiCore{basicPmu["UB MTE Write1"], ubMteW1, ubMteUsageW1}},
            {MemInfoAiCore{basicPmu["UB MTE Read1"], ubMteR1, ubMteUsageR1}},
            {MemInfoAiCore{basicPmu["UB MTE Write1"], ubMteW1, ubMteUsageW1}},
            {MemInfoAiCore{basicPmu["UB Vec Read1"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, basicPmu["UB Vec Read1"]}), "NA"}},
            {MemInfoAiCore{basicPmu["UB Vec Write1"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, basicPmu["UB Vec Write1"]}), "NA"}},
            {MemInfoAiCore{basicPmu["Scalar Read1"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 1, basicPmu["Scalar Read1"]}), "NA"}},
            {MemInfoAiCore{basicPmu["Scalar Write1"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 1, basicPmu["Scalar Write1"]}), "NA"}} } },
        {"Vector Core1", {
            {MemInfoAiCore{basicPmu["UB Vec Write1"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, basicPmu["UB Vec Write1"]}), "NA"}},
            {MemInfoAiCore{basicPmu["UB Vec Read1"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, basicPmu["UB Vec Read1"]}), "NA"}} } },
    };
    return memInfoAiCoreMapVecs;
}

std::map<std::string, std::vector<MemInfoAiCore>> StorageAccess310P::GetMemInfoAiCoreMap310P(std::map<std::string,
    uint64_t> &basicPmu, Profiling::Calculate &cal) const
{
    float l1R = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, basicPmu["L1 Read"]});
    float mteW = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {512, 8, basicPmu["MTE Write"]});
    float l0aR = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 16, basicPmu["L0A Read"]});
    float l0bR = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 8, basicPmu["L0B Read"]});
    float l0bCubeW = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 16, basicPmu["L0B Cube Write"]});
    float l0aCubeW = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 16, basicPmu["L0A Cube Write"]});
    float l0cW = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 32, basicPmu["L0C Write"]});
    float ubW = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, basicPmu["UB Write"]});
    float ubR = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, basicPmu["UB Read"]});
    float ubMteR = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {16, 8, basicPmu["UB MTE Read"]});
    float ubMteW = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {16, 8, basicPmu["UB MTE Write"]});
    float ubVecR = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {32, 8, basicPmu["UB Vec Read"]});
    float ubVecW = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {32, 8, basicPmu["UB Vec Write"]});
    float scalarR = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 1, basicPmu["Scalar Read"]});
    float scalarW = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 1, basicPmu["Scalar Write"]});
    float gmMainR = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {512, 8, basicPmu["GM Main Read"]});
    float gmMainW = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {512, 8, basicPmu["GM Main Write"]});
    std::map<std::string, std::vector<MemInfoAiCore>> memInfoAiCoreMap = {
        {"L1", {
            {MemInfoAiCore{basicPmu["L1 Read"], l1R}}, {MemInfoAiCore{basicPmu["MTE Write"], mteW}},
            {MemInfoAiCore{basicPmu["L1 Read"], l1R}}, {MemInfoAiCore{basicPmu["MTE Write"], mteW}} } },
        {"L0A", {
            {MemInfoAiCore{basicPmu["L0A Read"], l0aR}}, {MemInfoAiCore{basicPmu["L0A Cube Write"], l0aCubeW}},
            {MemInfoAiCore{basicPmu["L0A Read"], l0aR}} } },
        {"L0B", {
            {MemInfoAiCore{basicPmu["L0B Read"], l0bR}}, {MemInfoAiCore{basicPmu["L0B Cube Write"], l0bCubeW}},
            {MemInfoAiCore{basicPmu["L0B Read"], l0bR}} } },
        {"Cube", {
            {MemInfoAiCore{basicPmu["L0A Cube Write"], l0aCubeW}},
            {MemInfoAiCore{basicPmu["L0B Cube Write"], l0bCubeW}},
            {MemInfoAiCore{basicPmu["L0C Write"], l0cW}} } },
        {"L0C", {
            {MemInfoAiCore{basicPmu["L0C Write"], l0cW}}, {MemInfoAiCore{basicPmu["UB Write"], ubW}},
            {MemInfoAiCore{basicPmu["UB Read"], ubR}} } },
        {"UB", {
            {MemInfoAiCore{basicPmu["UB MTE Read"], ubMteR}}, {MemInfoAiCore{basicPmu["UB MTE Write"], ubMteW}},
            {MemInfoAiCore{basicPmu["UB MTE Read"], ubMteR}}, {MemInfoAiCore{basicPmu["UB MTE Write"], ubMteW}},
            {MemInfoAiCore{basicPmu["UB Vec Read"], ubVecR}}, {MemInfoAiCore{basicPmu["UB Vec Write"], ubVecW}},
            {MemInfoAiCore{basicPmu["Scalar Read"], scalarR}}, {MemInfoAiCore{basicPmu["Scalar Write"], scalarW}},
            {MemInfoAiCore{basicPmu["UB Write"], ubW}}, {MemInfoAiCore{basicPmu["UB Read"], ubR}} } },
        {"Vector", {
            {MemInfoAiCore{basicPmu["UB Vec Write"], ubVecW}},
            {MemInfoAiCore{basicPmu["UB Vec Read"], ubVecR}} } },
        {"GM", {
            {MemInfoAiCore{basicPmu["GM Main Read"], gmMainR}},
            {MemInfoAiCore{basicPmu["GM Main Write"], gmMainW}} } },
    };
    return memInfoAiCoreMap;
}

std::map<std::string, std::string> StorageAccess910B::CalCulateForVecOrCubeBw(
    const std::map<std::string, uint64_t> &basicPmu, const std::string &opType, Profiling::Calculate &cal,
    std::map<std::string, uint64_t> &dbiRequest) const
{
    std::map<std::string, uint64_t> pmu = basicPmu;
    std::map<std::string, std::string> bw;
    bw["MTE1"] = "NA";
    bw["MTE2"] = "NA";
    bw["MTE3"] = "NA";
    if (opType == OpType::CUBE) {
        if (!dbiRequest.empty()) {
            bw["MTE1"] = cal.CalAicMte1ActivateBw({256, 8, 128, 8, dbiRequest[GM_TO_L0A_DATA],
                dbiRequest[GM_TO_L0B_DATA], pmu["L0A Read"], pmu["L0B Read"], pmu["MTE1 Cyc"]});
            bw["MTE2"] = cal.CalAicMte2ActivateBw(dbiRequest[GM_TO_L1_DATA], dbiRequest[GM_TO_L0A_DATA],
                dbiRequest[GM_TO_L0B_DATA], pmu["MTE2 Cyc"]);
        }
        if (pmu["MTE3 Cyc"] != 0) {
            bw["MTE3"] = std::to_string(
                static_cast<float>(pmu["L1 GM Write"]) * 128 / pmu["MTE3 Cyc"]
                * cal.freq_ / BIT_CONVERSION / BIT_CONVERSION / BIT_CONVERSION
                * TIME_CONVERSION * TIME_CONVERSION
            );
        }
    }
    if (opType == OpType::VECTOR) {
        bw["MTE2"] = cal.CalAivMteActivateBw(128, pmu["UB MTE Read"], pmu["MTE2 Cyc"]);
        bw["MTE3"] = cal.CalAivMteActivateBw(128, pmu["UB MTE Write"], pmu["MTE3 Cyc"]);
    }
    bw["FIXP"] = cal.CalAivMteActivateBw(128, pmu["L0C READ"], pmu["FIXP Cyc"]);
    return bw;
}

std::map<std::string, std::string> StorageAccess910B::CalCulateForMixBw(
    const std::map<std::string, uint64_t> &basicPmu, Profiling::Calculate &cal,
    std::map<std::string, uint64_t> &dbiRequest) const
{
    std::map<std::string, std::string> bw;
    std::map<std::string, uint64_t> pmu = basicPmu;
    if (memoryDetail_ && !dbiRequest.empty()) {
        bw["Cube MTE1"] = cal.CalAicMte1ActivateBw({256, 8, 128, 8, dbiRequest[GM_TO_L0A_DATA],
            dbiRequest[GM_TO_L0B_DATA], pmu["L0A Read"], pmu["L0B Read"], pmu["MTE1 Cyc"]});
        bw["Cube MTE2"] = cal.CalAicMte2ActivateBw(dbiRequest[GM_TO_L1_DATA], dbiRequest[GM_TO_L0A_DATA],
            dbiRequest[GM_TO_L0B_DATA], pmu["MTE2 Cyc"]);
    } else {
        bw["Cube MTE1"] = "NA";
        bw["Cube MTE2"] = "NA";
    }

    if (pmu["MTE3 Cyc"] == 0) {
        bw["Cube MTE3"] = "NA";
    } else {
        bw["Cube MTE3"] = std::to_string(static_cast<float>(pmu["L1 GM Write"]) * 128 / pmu["MTE3 Cyc"]
            * cal.freq_ / BIT_CONVERSION / BIT_CONVERSION / BIT_CONVERSION * TIME_CONVERSION * TIME_CONVERSION);
    }
    bw["Cube FIXP"] = cal.CalAivMteActivateBw(128, pmu["L0C READ"], pmu["FIXP Cyc"]);

    bw["Vector0 MTE2"] = cal.CalAivMteActivateBw(128, pmu["UB MTE Write"], pmu["MTE2 Cyc vec0"]);
    bw["Vector0 MTE3"] = cal.CalAivMteActivateBw(128, pmu["UB MTE Read"], pmu["MTE3 Cyc vec0"]);

    bw["Vector1 MTE2"] = cal.CalAivMteActivateBw(128, pmu["UB MTE Write1"], pmu["MTE2 Cyc vec1"]);
    bw["Vector1 MTE3"] = cal.CalAivMteActivateBw(128, pmu["UB MTE Read1"], pmu["MTE3 Cyc vec1"]);
    return bw;
}

void StorageAccess910B::TableSplit(std::map<std::string, uint64_t> &basicPmu, Profiling::Calculate &cal,
                                   std::map<std::string, uint64_t> &dbiRequest)
{
    auto mixBwMap = CalCulateForMixBw(basicPmu, cal, dbiRequest);
    std::map<std::string, std::vector<MemInfoPipe>> memInfoPipeMapCore = {
        {"Pipe Cube", {
            {MemInfoPipe{basicPmu["MTE1 Ins"], basicPmu["MTE1 Cyc"], basicPmu["MTE1 Wait"], mixBwMap["Cube MTE1"]}},
            {MemInfoPipe{basicPmu["MTE2 Ins"], basicPmu["MTE2 Cyc"], basicPmu["MTE2 Wait"], mixBwMap["Cube MTE2"]}},
            {MemInfoPipe{basicPmu["MTE3 Ins"], basicPmu["MTE3 Cyc"], basicPmu["MTE3 Wait"], mixBwMap["Cube MTE3"]}},
            {MemInfoPipe{basicPmu["FIXP Ins"], basicPmu["FIXP Cyc"], 0, mixBwMap["Cube FIXP"]}},
            {MemInfoPipe{0, basicPmu["Scalar Cyc"], basicPmu["Scalar Wait"], "NA"}}} },
        {"Pipe Vector Core0", {
            {MemInfoPipe{basicPmu["MTE2 Ins vec0"], basicPmu["MTE2 Cyc vec0"], basicPmu["MTE2 Wait vec0"], mixBwMap["Vector0 MTE2"]}},
            {MemInfoPipe{basicPmu["MTE3 Ins vec0"], basicPmu["MTE3 Cyc vec0"], basicPmu["MTE3 Wait vec0"], mixBwMap["Vector0 MTE3"]}},
            {MemInfoPipe{0, basicPmu["Scalar Cyc vec0"], basicPmu["Scalar Wait vec0"], "NA"}}} },
        {"Pipe Vector Core1", {
            {MemInfoPipe{basicPmu["MTE2 Ins vec1"], basicPmu["MTE2 Cyc vec1"], basicPmu["MTE2 Wait vec1"], mixBwMap["Vector1 MTE2"]}},
            {MemInfoPipe{basicPmu["MTE3 Ins vec1"], basicPmu["MTE3 Cyc vec1"], basicPmu["MTE3 Wait vec1"], mixBwMap["Vector1 MTE3"]}},
            {MemInfoPipe{0, basicPmu["Scalar Cyc vec1"], basicPmu["Scalar Wait vec1"], "NA"}}} },
    };
    memInfoPipeMap_.erase("Pipe");
    memInfoPipeMap_.insert(memInfoPipeMapCore.begin(), memInfoPipeMapCore.end());
    std::map<std::string, std::vector<MemInfoAiCore>>  memInfoAiCoreMapCore = {
        {"GM Cube", {
            {MemInfoAiCore{basicPmu["GM Main Read"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 8, basicPmu["GM Main Read"]}), "NA"}},
            {MemInfoAiCore{basicPmu["GM Main Write"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 8, basicPmu["GM Main Write"]}), "NA"}} }
        },
        {"GM Vector Core0", {
            {MemInfoAiCore{basicPmu["GM Main Read vec0"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 8, basicPmu["GM Main Read vec0"]}), "NA"}},
            {MemInfoAiCore{basicPmu["GM Main Write vec0"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 8, basicPmu["GM Main Write vec0"]}), "NA"}} }
        },
        {"GM Vector Core1", {
            {MemInfoAiCore{basicPmu["GM Main Read vec1"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 8, basicPmu["GM Main Read vec1"]}), "NA"}},
            {MemInfoAiCore{basicPmu["GM Main Write vec1"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 8, basicPmu["GM Main Write vec1"]}), "NA"}} }
        }
    };
    memInfoAiCoreMap_.erase("GM");
    memInfoAiCoreMap_.insert(memInfoAiCoreMapCore.begin(), memInfoAiCoreMapCore.end());
}

void StorageAccess910B::GetPipePeak(std::map<std::string, uint64_t> basicPmu, MemMapDetail &memMapDetail,
    const std::string &opType, Profiling::Calculate &cal)
{
    // 1.Calculate the theoretical bandwidth of MTE1 based on the weighted channel bandwidth.
    uint64_t l0aDatas = basicPmu["L0A Read"] * REQ_DATA_OF_910B.at(TransportType::L1_TO_L0A);
    uint64_t l0bDatas = basicPmu["L0B Read"] * REQ_DATA_OF_910B.at(TransportType::L1_TO_L0B);
    auto maxBwRate = pmuCalculatorObj_->GetBandWidthByWeight(l0aDatas, l0bDatas);

    // 2.Obtains the peak value of each pipe channel.
    std::string socName = opBasicInfoObj_->GetSoc();
    socName = (FREQ_MAP.count(socName) == 0) ? "Ascend910B1" : socName;
    if (opType == Common::OpType::VECTOR) {
        peakMap_["Pipe MTE2"] = memInfoAiCoreMap_["UB"][0].throughput / maxBwRate[socName]["MTE2"];
        peakMap_["Pipe MTE3"] = memInfoAiCoreMap_["UB"][1].throughput / maxBwRate[socName]["MTE3"];
        return;
    }
    std::string mteCube = "Pipe MTE";
    if (opType != Common::OpType::CUBE) {
        mteCube = "Pipe Cube MTE";
        peakMap_["Pipe Vector Core0 MTE2"] =
            memInfoAiCoreMap_["UB Core0"][0].throughput / maxBwRate[socName]["MTE2 vector"];
        peakMap_["Pipe Vector Core0 MTE3"] =
            memInfoAiCoreMap_["UB Core0"][1].throughput / maxBwRate[socName]["MTE3 vector"];

        peakMap_["Pipe Vector Core1 MTE2"] =
            memInfoAiCoreMap_["UB Core1"][0].throughput / maxBwRate[socName]["MTE2 vector"];
        peakMap_["Pipe Vector Core1 MTE3"] =
            memInfoAiCoreMap_["UB Core1"][1].throughput / maxBwRate[socName]["MTE3 vector"];
    }
    std::vector<uint64_t> mteBytesVec = {
        basicPmu["MTE Write"] * REQ_DATA_OF_910B.at(TransportType::L1_TO_MTE) -
            basicPmu["L1 GM Write"] * REQ_DATA_OF_910B.at(TransportType::L1_TO_GM),
        basicPmu["L0A Read"] * REQ_DATA_OF_910B.at(TransportType::L1_TO_L0A) +
            basicPmu["L0B Read"] * REQ_DATA_OF_910B.at(TransportType::L1_TO_L0B) +
            basicPmu["MTE/GM Read"] * REQ_DATA_OF_910B.at(TransportType::GM_TO_L1) -
            basicPmu["L1 GM Write"] * REQ_DATA_OF_910B.at(TransportType::L1_TO_GM),
        memMapDetail.eventMap[static_cast<uint64_t>(Event::WRITE_DATA)] * REQ_DATA_OF_910B.at(TransportType::GM_WRITE)
            - basicPmu["GM Write"] * REQ_DATA_OF_910B.at(TransportType::L0C_TO_GM)
    };
    for (size_t i = 0; i < mteBytesVec.size(); ++i) {
        std::string pipeIndex = std::to_string(i + 1);
        peakMap_[mteCube + pipeIndex] = (FunctionsFp.at(FuncType::BW_USAGE_REQ_FP)(cal, {mteBytesVec[i], 1})) /
            maxBwRate[socName]["MTE" + pipeIndex];
    }
}

std::map<PipeAll, MemInfoAiCore> StorageAccess910B::GetCubeMap()
{
    std::map<PipeAll, MemInfoAiCore> cubeMap = {
        {PipeAll::GM_TO_L2, memInfoAiCoreMap_["Cache"][0]},
        {PipeAll::L2_TO_GM, memInfoAiCoreMap_["Cache"][1]}, {PipeAll::L2_TO_L1, memInfoAiCoreMap_["L1"][0]},
        {PipeAll::L1_TO_L2, memInfoAiCoreMap_["L1"][4]},
        {PipeAll::GM_OR_L1_TO_L0A, memInfoAiCoreMap_["L0A"][0]},
        {PipeAll::GM_OR_L1_TO_L0B, memInfoAiCoreMap_["L0B"][0]},
        {PipeAll::L0A_TO_CUBE, memInfoAiCoreMap_["L0A"][1]},
        {PipeAll::L0B_TO_CUBE, memInfoAiCoreMap_["L0B"][1]},
        {PipeAll::CUBE_TO_L0C, memInfoAiCoreMap_["L0C"][0]}, {PipeAll::L0C_TO_L1, memInfoAiCoreMap_["L1"][3]},
        {PipeAll::L0C_TO_L2, memInfoAiCoreMap_["L0C"][2]}, {PipeAll::L0C_TO_CUBE, memInfoAiCoreMap_["L0C"][1]},
    };
    if (!ApiDataTransMemInfo_.empty()) {
        cubeMap.insert({PipeAll::GM_TO_L0A_AIC, ApiDataTransMemInfo_[GM_TO_L0A]});
        cubeMap.insert({PipeAll::GM_TO_L0B_AIC, ApiDataTransMemInfo_[GM_TO_L0B]});
        cubeMap.insert({PipeAll::L1_TO_L0A_AIC, ApiDataTransMemInfo_[L1_TO_L0A]});
        cubeMap.insert({PipeAll::L1_TO_L0B_AIC, ApiDataTransMemInfo_[L1_TO_L0B]});
    }
    return cubeMap;
}

void StorageAccess910B::SetMemInfo910B(const std::string &opType, std::map<std::string, uint64_t> &basicPmu,
    Profiling::Calculate &cal, std::map<std::string, uint64_t> &dbiRequest)
{
    std::string location = "calculate memory info";
    uint64_t l2cacheHit = SafeAdd(basicPmu["L2Cache Write hit"], basicPmu["L2Cache Read hit"], location, false);
    uint64_t l2cacheMiss = SafeAdd(basicPmu["L2Cache Write miss"], basicPmu["L2Cache Read miss"], location, false);
    auto mteBwMap = CalCulateForVecOrCubeBw(basicPmu, opType, cal, dbiRequest);
    memInfoCacheMap_ = {
        {"Cache", {
            {MemInfoCache{basicPmu["L2Cache Write hit"], basicPmu["L2Cache Write miss"]}},
            {MemInfoCache{basicPmu["L2Cache Read hit"], basicPmu["L2Cache Read miss"]}},
            {MemInfoCache{l2cacheHit, l2cacheMiss}},
            {MemInfoCache{basicPmu["iCache total hit"], basicPmu["iCache total miss"]}} } } };

    memInfoPipeMap_ = {
        {"Pipe", {
            {MemInfoPipe{basicPmu["MTE1 Ins"], basicPmu["MTE1 Cyc"], basicPmu["MTE1 Wait"], mteBwMap["MTE1"]}},
            {MemInfoPipe{basicPmu["MTE2 Ins"], basicPmu["MTE2 Cyc"], basicPmu["MTE2 Wait"], mteBwMap["MTE2"]}},
            {MemInfoPipe{basicPmu["MTE3 Ins"], basicPmu["MTE3 Cyc"], basicPmu["MTE3 Wait"], mteBwMap["MTE3"]}},
            {MemInfoPipe{basicPmu["FIXP Ins"], basicPmu["FIXP Cyc"], 0, mteBwMap["FIXP"]}},
            {MemInfoPipe{0, basicPmu["Scalar Cyc"], basicPmu["Scalar Wait"], "NA"}} } },
    };
    memInfoAiCoreMap_ = {
        {"GM", {
            {MemInfoAiCore{basicPmu["GM Main Read"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 8, basicPmu["GM Main Read"]}), "NA"}},
            {MemInfoAiCore{basicPmu["GM Main Write"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 8, basicPmu["GM Main Write"]}), "NA"}} }
        },
        // 这里需要加上cache的，因为L2Cache Read miss对应着GM_TO_L1,没有计算出带宽和peak ratio
        {"Cache", {
            {MemInfoAiCore{basicPmu["GM Read Total"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 8, basicPmu["GM Read Total"]}), "NA"}},
            {MemInfoAiCore{basicPmu["GM Write Total"],
                FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {512, 8, basicPmu["GM Write Total"]}), "NA"}} },
        }
    };
    if (opType == Common::OpType::MIX) {
        TableSplit(basicPmu, cal, dbiRequest);
    }
    SetScalarMemInfo(opType, basicPmu, cal);
    AddInternuclearScalarIndex(opType, basicPmu, cal);
}

void StorageAccess910B::SetScalarMemInfo(const std::string &opType, std::map<std::string, uint64_t> &basicPmu,
    Profiling::Calculate &cal)
{
    std::map<std::string, MemInfoScalar> scalarIndex;
    std::vector<std::string> scalarIndexVec = {"Scalar Time", "Scalar Single", "Scalar Dual", "Scalar Mte2 Stall", "Scalar Mte3 Stall", "Scalar Wait IB", "Scalar Wait"};
    scalarIndex = CalculateScalarIndex(scalarIndexVec, basicPmu, cal);

    if (opType == Common::OpType::VECTOR) {
        scalarIndex["Scalar Ub Stall"] = MemInfoScalar{basicPmu["Scalar Ub Stall"],         Functions.at(FuncType::PIPE_TIME)(cal, {basicPmu["Scalar Ub Stall"]})};
        scalarIndex["Scalar Vector Stall"] = MemInfoScalar{basicPmu["Scalar Vector Stall"], Functions.at(FuncType::PIPE_TIME)(cal, {basicPmu["Scalar Vector Stall"]})};
        memInfoScalarMap_["Scalar"] = {scalarIndex["Scalar Time"], scalarIndex["Scalar Single"], scalarIndex["Scalar Dual"], scalarIndex["Scalar Mte2 Stall"], scalarIndex["Scalar Mte3 Stall"],
            scalarIndex["Scalar Wait IB"], scalarIndex["Scalar Wait"], scalarIndex["Scalar Ub Stall"], scalarIndex["Scalar Vector Stall"]};
        return;
    }

    if (opType == Common::OpType::CUBE) {
        scalarIndex["Scalar Cube Stall"] = MemInfoScalar{basicPmu["Scalar Cube Stall"], Functions.at(FuncType::PIPE_TIME)(cal, {basicPmu["Scalar Cube Stall"]})};
        scalarIndex["Scalar Mte1 Stall"] = MemInfoScalar{basicPmu["Scalar Mte1 Stall"], Functions.at(FuncType::PIPE_TIME)(cal, {basicPmu["Scalar Mte1 Stall"]})};
         memInfoScalarMap_["Scalar"] = {scalarIndex["Scalar Time"], scalarIndex["Scalar Single"], scalarIndex["Scalar Dual"],  scalarIndex["Scalar Mte1 Stall"], scalarIndex["Scalar Mte2 Stall"],
            scalarIndex["Scalar Mte3 Stall"], scalarIndex["Scalar Wait IB"], scalarIndex["Scalar Wait"], scalarIndex["Scalar Cube Stall"]};
        return;
    }
        
    if (opType == Common::OpType::MIX) {
        std::vector<std::string> scalarIndexMixVec = {"Scalar Cube Stall", "Scalar Mte1 Stall", "Scalar Time Vec0", "Scalar Single Vec0", "Scalar Dual Vec0", "Scalar Mte2 Stall Vec0", "Scalar Mte3 Stall Vec0",
            "Scalar Vector Stall Vec0", "Scalar Wait IB Vec0", "Scalar Wait Vec0", "Scalar Ub Stall Vec0", "Scalar Time Vec1", "Scalar Single Vec1", "Scalar Dual Vec1", "Scalar Mte2 Stall Vec1", "Scalar Mte3 Stall Vec1",
            "Scalar Vector Stall Vec1", "Scalar Wait IB Vec1", "Scalar Wait Vec1", "Scalar Ub Stall Vec1"
        };
        std::map<std::string, MemInfoScalar> scalarIndexMix = CalculateScalarIndex(scalarIndexMixVec, basicPmu, cal);
        scalarIndex.insert(scalarIndexMix.begin(), scalarIndexMix.end());
        memInfoScalarMap_["Scalar Cube"] = {scalarIndex["Scalar Time"], scalarIndex["Scalar Single"], scalarIndex["Scalar Dual"],  scalarIndex["Scalar Mte1 Stall"], scalarIndex["Scalar Mte2 Stall"],
            scalarIndex["Scalar Mte3 Stall"], scalarIndex["Scalar Wait IB"], scalarIndex["Scalar Wait"], scalarIndex["Scalar Cube Stall"]};
        memInfoScalarMap_["Scalar Vectore Core0"] = {scalarIndex["Scalar Time Vec0"], scalarIndex["Scalar Single Vec0"], scalarIndex["Scalar Dual Vec0"], scalarIndex["Scalar Mte2 Stall Vec0"],
            scalarIndex["Scalar Mte3 Stall Vec0"], scalarIndex["Scalar Wait IB Vec0"], scalarIndex["Scalar Wait Vec0"], scalarIndex["Scalar Ub Stall Vec0"], scalarIndex["Scalar Vector Stall Vec0"]};
        memInfoScalarMap_["Scalar Vectore Core1"] = {scalarIndex["Scalar Time Vec1"], scalarIndex["Scalar Single Vec1"], scalarIndex["Scalar Dual Vec1"], scalarIndex["Scalar Mte2 Stall Vec1"],
            scalarIndex["Scalar Mte3 Stall Vec1"], scalarIndex["Scalar Wait IB Vec1"], scalarIndex["Scalar Wait Vec1"], scalarIndex["Scalar Ub Stall Vec1"], scalarIndex["Scalar Vector Stall Vec1"]};
        return;
    }
}

void StorageAccess910B::AddInternuclearScalarIndex(const std::string &opType, std::map<std::string, uint64_t> &basicPmu,
    Profiling::Calculate &cal)
{
    std::vector<std::string> scalarIndexVec = {"Scalar Internuclear ID0", "Scalar Internuclear ID1", "Scalar Internuclear ID2", "Scalar Internuclear ID3", "Scalar Internuclear ID4",
        "Scalar Internuclear ID5", "Scalar Internuclear ID6", "Scalar Internuclear ID7", "Scalar Internuclear ID8", "Scalar Internuclear ID9", "Scalar Internuclear ID10",
        "Scalar Internuclear ID11", "Scalar Internuclear ID12", "Scalar Internuclear ID13", "Scalar Internuclear ID14", "Scalar Internuclear ID15"};
    std::map<std::string, MemInfoScalar> scalarIndex = CalculateScalarIndex(scalarIndexVec, basicPmu, cal);

    std::vector<std::string> validIndex {};
    for (const auto & index : scalarIndexVec) {
        if (scalarIndex[index].cycle != 0) {
            validIndex.emplace_back(index);
        }
    }
    // 当算子mix，默认计算的scalarIndex是cube指标
    if (opType == OpType::MIX) {
        for (const auto &index : validIndex) {
            tableLineAiCore_["Scalar Cube"].emplace_back(index);
            memInfoScalarMap_["Scalar Cube"].emplace_back(scalarIndex[index]);
        }
        scalarIndex.clear();
        validIndex.clear();
        auto GetTime = [&cal ,&basicPmu](std::string indexValue) -> std::string {
            return Functions.at(FuncType::PIPE_TIME)(cal, {basicPmu[indexValue]});;
        };
        for (const auto &index : scalarIndexVec) {
            std::string indexValue = index + " Vec0";
            if (basicPmu[indexValue] != 0) {
                std::string time = GetTime(indexValue);
                tableLineAiCore_["Scalar Vectore Core0"].emplace_back(index);
                memInfoScalarMap_["Scalar Vectore Core0"].emplace_back(MemInfoScalar{basicPmu[indexValue], time});
            }
            indexValue = index + " Vec1";
            if (basicPmu[indexValue] != 0) {
                std::string time = GetTime(indexValue);
                tableLineAiCore_["Scalar Vectore Core1"].emplace_back(index);
                memInfoScalarMap_["Scalar Vectore Core1"].emplace_back(MemInfoScalar{basicPmu[indexValue], time});
            }
        }
        return;
    }

    std::vector<std::string> &scalarHeadModify = opType == OpType::VECTOR ? scalarVec_ : scalarCube_;
    scalarHeadModify.insert(scalarHeadModify.end(), validIndex.begin(), validIndex.end());
    for (const auto &index : validIndex) {
        tableLineAiCore_["Scalar"].emplace_back(index);
        memInfoScalarMap_["Scalar"].emplace_back(scalarIndex[index]);
    }
}

std::map<std::string, MemInfoScalar> StorageAccess910B::CalculateScalarIndex(const std::vector<std::string> &indexVec, std::map<std::string, uint64_t> &basicPmu, Profiling::Calculate &cal)
{
    std::map<std::string, MemInfoScalar> res;
    for (const auto &index : indexVec) {
        std::string time = Functions.at(FuncType::PIPE_TIME)(cal, {basicPmu[index]});
        res[index] = {basicPmu[index], time};
    }
    return res;
}

std::vector<nlohmann::json> StorageAccess310P::GetAiCoreMemUnitJson(const std::string &opType)
{
    std::map<PipeAll, MemInfoAiCore> aiCoreMap;
    // 获取310P的通路数据
    aiCoreMap = { {PipeAll::GM_TO_L2, memInfoAiCoreMap_["GM"][0]},
        {PipeAll::L2_TO_GM, memInfoAiCoreMap_["GM"][1]}, {PipeAll::L2_OR_UB_TO_L1, memInfoAiCoreMap_["L1"][0]},
        {PipeAll::L1_TO_L0A, memInfoAiCoreMap_["L0A"][0]}, {PipeAll::L2_OR_L1_TO_L0B, memInfoAiCoreMap_["L0B"][0]},
        {PipeAll::L0A_TO_CUBE, memInfoAiCoreMap_["L0A"][1]}, {PipeAll::L0B_TO_CUBE, memInfoAiCoreMap_["L0B"][1]},
        {PipeAll::CUBE_TO_L0C, memInfoAiCoreMap_["L0C"][0]}, {PipeAll::VEC_TO_L0C, memInfoAiCoreMap_["L0C"][1]},
        {PipeAll::L0C_TO_VEC, memInfoAiCoreMap_["L0C"][2]}, {PipeAll::L2_OR_L1_TO_UB, memInfoAiCoreMap_["UB"][0]},
        {PipeAll::UB_TO_L2, memInfoAiCoreMap_["UB"][1]}, {PipeAll::UB_TO_VEC, memInfoAiCoreMap_["UB"][4]},
        {PipeAll::VEC_TO_UB, memInfoAiCoreMap_["UB"][5]} };
    std::vector<nlohmann::json> memoryUnit;
    for (const auto &aiCorePair : aiCoreMap) {
        nlohmann::json units;
        units["memory_path"] = aiCorePair.first;
        auto memInfo = aiCorePair.second;
        units["request"] = (memInfo.request == EMPTY_PMU_VALUE) ? "NA" : std::to_string(memInfo.request);
        units["bandwidth"] = (memInfo.request == EMPTY_PMU_VALUE) ? "NA" : std::to_string(memInfo.throughput);
        units["peak_ratio"] = "NA";
        units["display"] = 1;
        units["bandwidth_suffix"] = "";
        units["requests_suffix"] = "";
        memoryUnit.emplace_back(units);
    }
    return memoryUnit;
}

std::map<PipeAll, MemInfoAiCore> StorageAccessA5::GetVecMap()
{
    std::map<PipeAll, MemInfoAiCore> aiCoreMap = { {PipeAll::GM_TO_L2, memInfoAiCoreMap_["Cache"][0]},
        {PipeAll::L2_TO_GM, memInfoAiCoreMap_["Cache"][1]}, {PipeAll::L2_TO_UB, memInfoAiCoreMap_["UB"][2]},
        {PipeAll::UB_TO_L2, memInfoAiCoreMap_["GM"][1]}, {PipeAll::UB_TO_VEC, memInfoAiCoreMap_["UB"][4]},
        {PipeAll::VEC_TO_UB, memInfoAiCoreMap_["UB"][3]},
        {PipeAll::L2_TO_DCACHE, memInfoAiCoreMap_["DCache"][0]},
        {PipeAll::VEC_TO_DCACHE, memInfoAiCoreMap_["DCache"][1]},
        {PipeAll::DCACHE_TO_VEC, memInfoAiCoreMap_["DCache"][2]}};
    return aiCoreMap;
}

std::map<PipeAll, MemInfoAiCore> StorageAccessA5::GetCubeMap()
{
    std::map<PipeAll, MemInfoAiCore> aiCoreMap = { {PipeAll::GM_TO_L2, memInfoAiCoreMap_["Cache"][0]},
        {PipeAll::L2_TO_GM, memInfoAiCoreMap_["Cache"][1]}, {PipeAll::L2_TO_L1, memInfoAiCoreMap_["L1"][4]},
        {PipeAll::L1_TO_L0A_AIC, memInfoAiCoreMap_["L0A"][0]},
        {PipeAll::L1_TO_L0B_AIC, memInfoAiCoreMap_["L0B"][0]},
        {PipeAll::L0A_TO_CUBE, memInfoAiCoreMap_["L0A"][1]},
        {PipeAll::L0B_TO_CUBE, memInfoAiCoreMap_["L0B"][1]},
        {PipeAll::CUBE_TO_L0C, memInfoAiCoreMap_["L0C"][0]},
        {PipeAll::L0C_TO_FIXP, memInfoAiCoreMap_["L0C"][5]},
        {PipeAll::FIXP_TO_L1, memInfoAiCoreMap_["L0C"][3]},
        {PipeAll::FIXP_TO_L2, memInfoAiCoreMap_["L0C"][2]} };
    return aiCoreMap;
}

std::map<PipeAll, MemInfoAiCore> StorageAccessA5::GetMixMap()
{
    std::map<PipeAll, MemInfoAiCore> aiCoreMap = { {PipeAll::FIXP_TO_UB, memInfoAiCoreMap_["UB Core0"][5]},
        {PipeAll::FIXP_TO_UB_2, memInfoAiCoreMap_["UB Core1"][5]},
        {PipeAll::UB_TO_L1, memInfoAiCoreMap_["L1"][6]},
        {PipeAll::L1_TO_UB, memInfoAiCoreMap_["L1"][5]},
        {PipeAll::L2_TO_UB, memInfoAiCoreMap_["UB Core0"][2]},
        {PipeAll::L2_TO_UB_2, memInfoAiCoreMap_["UB Core1"][2]},
        {PipeAll::UB_TO_L2, memInfoAiCoreMap_["GM Vector Core0"][1]},
        {PipeAll::UB_TO_L2_2, memInfoAiCoreMap_["GM Vector Core1"][1]},
        {PipeAll::UB_TO_VEC, memInfoAiCoreMap_["UB Core0"][4]},
        {PipeAll::UB_TO_VEC_2, memInfoAiCoreMap_["UB Core1"][4]},
        {PipeAll::VEC_TO_UB, memInfoAiCoreMap_["UB Core0"][3]},
        {PipeAll::VEC_TO_UB_2, memInfoAiCoreMap_["UB Core1"][3]},
        {PipeAll::L2_TO_DCACHE, memInfoAiCoreMap_["DCache Core0"][0]},
        {PipeAll::L2_TO_DCACHE_2, memInfoAiCoreMap_["DCache Core1"][0]},
        {PipeAll::VEC_TO_DCACHE, memInfoAiCoreMap_["DCache Core0"][1]},
        {PipeAll::VEC_TO_DCACHE_2, memInfoAiCoreMap_["DCache Core1"][1]},
        {PipeAll::DCACHE_TO_VEC, memInfoAiCoreMap_["DCache Core0"][2]},
        {PipeAll::DCACHE_TO_VEC_2, memInfoAiCoreMap_["DCache Core1"][2]},
    };
    return aiCoreMap;
}

std::vector<nlohmann::json> StorageAccessA5::GetAiCoreMemUnitJson(const string &opType)
{
    std::map<PipeAll, MemInfoAiCore> aiCoreMap;
    if (opType == Common::OpType::VECTOR) {
        aiCoreMap = GetVecMap();
    } else if (opType == Common::OpType::CUBE) {
        aiCoreMap = GetCubeMap();
    } else {
        std::map<PipeAll, MemInfoAiCore> aiCubeCoreMap = GetCubeMap();
        std::map<PipeAll, MemInfoAiCore> mixMap = GetMixMap();
        aiCoreMap.insert(aiCubeCoreMap.begin(), aiCubeCoreMap.end());
        aiCoreMap.insert(mixMap.begin(), mixMap.end());
    }
    std::vector<nlohmann::json> memoryUnit;
    for (const auto &aiCorePair : aiCoreMap) {
        nlohmann::json units;
        units["memory_path"] = aiCorePair.first;
        auto memInfo = aiCorePair.second;
        units["request"] = (memInfo.request == EMPTY_PMU_VALUE) ? "NA" : std::to_string(memInfo.request);
        units["bandwidth"] = (memInfo.request == EMPTY_PMU_VALUE) ? "NA" : std::to_string(memInfo.throughput);
        units["peak_ratio"] = "NA";
        units["display"] = 1;
        memoryUnit.emplace_back(units);
    }
    return memoryUnit;
}

std::vector<nlohmann::json> StorageAccess910B::GetAiCoreMemUnitJson(const std::string &opType)
{
    std::map<PipeAll, MemInfoAiCore> aiCoreMap;
    std::map<PipeAll, MemInfoAiCore> cubeMap;
    if (opType == Common::OpType::VECTOR) {
        aiCoreMap = { {PipeAll::GM_TO_L2, memInfoAiCoreMap_["Cache"][0]},
            {PipeAll::L2_TO_GM, memInfoAiCoreMap_["Cache"][1]}, {PipeAll::L2_TO_UB, memInfoAiCoreMap_["UB"][0]},
            {PipeAll::UB_TO_L2, memInfoAiCoreMap_["UB"][1]}, {PipeAll::UB_TO_VEC, memInfoAiCoreMap_["Vector"][1]},
            {PipeAll::VEC_TO_UB, memInfoAiCoreMap_["Vector"][0]} };
    } else if (opType == Common::OpType::CUBE) {   // 910B vector
        cubeMap = GetCubeMap();
        aiCoreMap.insert(cubeMap.begin(), cubeMap.end());
    } else {
        cubeMap = GetCubeMap();
        std::map<PipeAll, MemInfoAiCore> vectorMap2 = { {PipeAll::L2_TO_UB, memInfoAiCoreMap_["UB Core0"][0]},
            {PipeAll::UB_TO_L2, memInfoAiCoreMap_["UB Core0"][1]},
            {PipeAll::UB_TO_VEC, memInfoAiCoreMap_["Vector Core0"][1]},
            {PipeAll::VEC_TO_UB, memInfoAiCoreMap_["Vector Core0"][0]},
            {PipeAll::L2_TO_UB_2, memInfoAiCoreMap_["UB Core1"][0]},
            {PipeAll::UB_TO_L2_2, memInfoAiCoreMap_["UB Core1"][1]},
            {PipeAll::UB_TO_VEC_2, memInfoAiCoreMap_["Vector Core1"][1]},
            {PipeAll::VEC_TO_UB_2, memInfoAiCoreMap_["Vector Core1"][0]} };
        aiCoreMap.insert(cubeMap.begin(), cubeMap.end());
        aiCoreMap.insert(vectorMap2.begin(), vectorMap2.end());
    }

    std::vector<nlohmann::json> memoryUnit;
    for (const auto &aiCorePair : aiCoreMap) {
        nlohmann::json units;
        units["memory_path"] = aiCorePair.first;
        auto memInfo = aiCorePair.second;
        units["request"] = (memInfo.request == EMPTY_PMU_VALUE) ? "NA" : std::to_string(memInfo.request);
        units["bandwidth"] = (memInfo.request == EMPTY_PMU_VALUE) ? "NA" : std::to_string(memInfo.throughput);
        units["peak_ratio"] = memInfo.peak;
        units["display"] = 1;
        units["bandwidth_suffix"] = "";
        units["request_suffix"] = "";
        if (aiCorePair.first == PipeAll::L2_TO_GM) {
            units["bandwidth_suffix"] = "(theoretical value)";
            units["request_suffix"] = "(theoretical value)";
        }
        memoryUnit.emplace_back(units);
    }
    return memoryUnit;
}

std::map<std::string, std::vector<std::string>> StorageAccess910B::GenTableHead()
{
    std::vector<std::string> memTableWithPeak = {"", "requests", "throughput(GB/s)", "peak(%)"};
    // 910B独有
    std::map<std::string, std::vector<std::string>> headInsert = { {"UB", memTableWithPeak},
        {"UB Core0", memTableWithPeak}, {"UB Core1", memTableWithPeak}, {"L1", memTableWithPeak},
        {"L0C", memTableWithPeak} };
    head_.insert(headInsert.begin(), headInsert.end());

    return head_;
}

std::map<std::string, std::vector<std::string>> StorageAccess310P::GenTableHead()
{
    return head_;
}

std::vector<std::string> StorageAccess310P::GetTableValue(const std::string &tableName, size_t colum,
    std::map<std::string, uint64_t> &cycMap, const std::map<std::string, std::vector<std::string>> &head)
{
    std::vector<std::string> rowValue;
    if (tableName == "Cache") {
        MemInfoCache ress = memInfoCacheMap_[tableName][colum];
        if (ress.miss == EMPTY_PMU_VALUE) {
            rowValue = { "NA", "NA", "NA", "NA" };
        } else {
            rowValue = { std::to_string(ress.hit), std::to_string(ress.miss), std::to_string(ress.hit + ress.miss),
                std::to_string(pmuCalculatorObj_->CalculatePer(ress.hit, (ress.hit + ress.miss))) };
        }
    } else if (tableName.find("Pipe") != std::string::npos) {
        MemInfoPipe ress = memInfoPipeMap_[tableName][colum];
        uint64_t cyc = pipeMap_.count(tableName) == 0 ? cycMap["totalCycles"] : cycMap[pipeMap_[tableName]];
        float activeRate = pmuCalculatorObj_->CalculatePer(ress.cycle, cyc);
        auto pipe = tableLineAiCore_[tableName][colum];
        activeRate_[tableName + " " + pipe] = activeRate;
        pipeLineRatio_[pipe] = (pipeLineRatio_.count(pipe) != 0) ? max(pipeLineRatio_[pipe], activeRate) : activeRate;
        rowValue = { std::to_string(ress.instr), std::to_string(ress.cycle), std::to_string(ress.waitCycle),
            std::to_string(activeRate) };
    } else {
        MemInfoAiCore ress = memInfoAiCoreMap_[tableName][colum];
        rowValue.emplace_back((ress.request == EMPTY_PMU_VALUE) ? "NA" : std::to_string(ress.request));
        rowValue.emplace_back((ress.request == EMPTY_PMU_VALUE) ? "NA" : std::to_string(ress.throughput));
    }
    return rowValue;
}

std::vector<std::string> StorageAccess910B::GetTableValue(const std::string &tableName, size_t colum,
    std::map<std::string, uint64_t> &cycMap, const std::map<std::string, std::vector<std::string>> &head)
{
    std::vector<std::string> rowValue;
    if (tableName == "Cache") {
        MemInfoCache ress = memInfoCacheMap_[tableName][colum];
        if (ress.miss == EMPTY_PMU_VALUE) {
            rowValue = { "NA", "NA", "NA", "NA" };
        } else {
            rowValue = { std::to_string(ress.hit), std::to_string(ress.miss), std::to_string(ress.hit + ress.miss),
                std::to_string(pmuCalculatorObj_->CalculatePer(ress.hit, (ress.hit + ress.miss))) };
        }
    } else if (tableName.find("Pipe") != std::string::npos) {
        MemInfoPipe ress = memInfoPipeMap_[tableName][colum];
        uint64_t cyc = pipeMap_.count(tableName) == 0 ? cycMap["totalCycles"] : cycMap[pipeMap_[tableName]];
        float activeRate = pmuCalculatorObj_->CalculatePer(ress.cycle, cyc);
        auto pipe = tableLineAiCore_[tableName][colum];
        activeRate_[tableName + " " + pipe] = activeRate;
        pipeLineRatio_[pipe] = (pipeLineRatio_.count(pipe) != 0) ? max(pipeLineRatio_[pipe], activeRate) : activeRate;
        rowValue.emplace_back(ress.instr == EMPTY_PMU_VALUE ? "NA" : std::to_string(ress.instr));
        rowValue.emplace_back(ress.cycle == EMPTY_PMU_VALUE ? "NA" : std::to_string(ress.cycle));
        rowValue.emplace_back(ress.waitCycle == EMPTY_PMU_VALUE ? "NA" : std::to_string(ress.waitCycle));
        rowValue.emplace_back(ress.cycle == EMPTY_PMU_VALUE || cyc == EMPTY_PMU_VALUE ? "NA" : std::to_string(activeRate));
        rowValue.emplace_back(ress.bandWidth);
    } else if (tableName.find("Scalar") != std::string::npos) {
        MemInfoScalar ress = memInfoScalarMap_[tableName][colum];
        rowValue.emplace_back(ress.cycle == EMPTY_PMU_VALUE ? "NA" : std::to_string(ress.cycle));
        rowValue.emplace_back(ress.cycle == EMPTY_PMU_VALUE ? "NA" : ress.time);
    } else {
        MemInfoAiCore ress = memInfoAiCoreMap_[tableName][colum];
        rowValue.emplace_back((ress.request == EMPTY_PMU_VALUE) ? "NA" : std::to_string(ress.request));
        rowValue.emplace_back((ress.request == EMPTY_PMU_VALUE) ? "NA" : std::to_string(ress.throughput));
        if ((head.count(tableName) != 0)) {
            rowValue.emplace_back(ress.peak);
        }
    }
    return rowValue;
}

void StorageAccess910B::PreProcess(bool isKernelScale, bool isMemoryDetail)
{
    kernelScale_ = isKernelScale;
    memoryDetail_ = isMemoryDetail;
    std::vector<MemMapDetail> memMapDetail = basicPmuObj_->GetMemMapDetails();
    // 将每个block的数据加到json里
    for (size_t i = 0; i < memMapDetail.size(); ++i) {
        std::string opType = memMapDetail[i].opType;
        LoadLineMap(opType);                          // 根据芯片类型，确定表格名称
        LoadMapData(opType, memMapDetail[i]);         // 确定表格内所有值
        // 生成热力图的json数据格式
        nlohmann::json coreMemJson;
        coreMemJson["core_no"] = std::to_string(memMapDetail[i].blockId);
        coreMemJson["op_type"] = opType;
        coreMemJson["soc"] = "910B";
        coreMemJson["memory_unit"] = GetAiCoreMemUnitJson(opType);
        nlohmann::json caches;
        uint64_t hit = (memInfoCacheMap_["Cache"][0].hit + memInfoCacheMap_["Cache"][1].hit);
        uint64_t miss = (memInfoCacheMap_["Cache"][0].miss + memInfoCacheMap_["Cache"][1].miss);
        caches["hit"] = (miss == EMPTY_PMU_VALUE) ? "NA" : std::to_string(hit);
        caches["miss"] = (miss == EMPTY_PMU_VALUE) ? "NA" : std::to_string(miss);
        caches["total_request"] = (miss == EMPTY_PMU_VALUE) ? "NA" : std::to_string(hit + miss);
        caches["hit_ratio"] = (miss == EMPTY_PMU_VALUE) ? "NA" :
            std::to_string(pmuCalculatorObj_->CalculatePer(hit, hit + miss));
        coreMemJson["L2cache"] = caches;
        auto cycMap = pmuCalculatorObj_->GetCycleMap(opType, memMapDetail[i]);
        std::vector<nlohmann::json> advice;
        for (const auto &cyc : cycMap) {
            nlohmann::json cycUsage;
            uint64_t totalCycle = (opType == Common::OpType::MIX) ? memMapDetail[i].cycMap[cyc.first] :
                memMapDetail[i].totalCycles;
            cycUsage["cycle"] = cyc.second;
            cycUsage["total_cycles"] = totalCycle;
            float ratio = pmuCalculatorObj_->CalculatePer(cyc.second, totalCycle);
            pipeLineRatio_[cyc.first] = (pipeLineRatio_.count(cyc.first) != 0) ? max(pipeLineRatio_[cyc.first], ratio) : ratio;
            GenCoreUsageAdvice(cyc.first, ratio, advice);
            cycUsage["ratio"] = ratio;
            coreMemJson[cyc.first] = cycUsage;
        }
        coreMemJson["advice"] = advice;
        heatMapJson_.emplace_back(coreMemJson);
        // 生成内存表格
        memMapDetail[i].cycMap["totalCycles"] = memMapDetail[i].totalCycles;
        nlohmann::json memTableJson;
        memTableJson["block_id"] = std::to_string(memMapDetail[i].blockId);
        memTableJson["table_op_type"] = opType;
        memTableJson["table_detail"] = GetAiCoreMemTableJson(memMapDetail[i].cycMap);
        memTableJson["advice"] = GenPipeAdvice(opType);
        mapTableJson_.emplace_back(memTableJson);
    }
}

void StorageAccess310P::PreProcess(bool isKernelScale, bool isMemoryDetail)
{
    kernelScale_ = false;
    std::vector<MemMapDetail> memMapDetail = basicPmuObj_->GetMemMapDetails();
    // 将每个block的数据加到json里
    for (size_t i = 0; i < memMapDetail.size(); ++i) {
        std::string opType = memMapDetail[i].opType;
        LoadLineMap(opType);                          // 根据芯片类型，确定表格名称
        LoadMapData(opType, memMapDetail[i]);         // 确定表格内所有值
        // 生成热力图的json数据格式
        nlohmann::json coreMemJson;
        coreMemJson["core_no"] = "NA";
        coreMemJson["op_type"] = opType;
        coreMemJson["soc"] = "310P";
        coreMemJson["memory_unit"] = GetAiCoreMemUnitJson(opType);
        nlohmann::json caches;
        uint64_t hit = memInfoCacheMap_["Cache"][0].hit;
        uint64_t miss = memInfoCacheMap_["Cache"][0].miss;
        caches["hit"] = (miss == EMPTY_PMU_VALUE) ? "NA" : std::to_string(hit);
        caches["miss"] = (miss == EMPTY_PMU_VALUE) ? "NA" : std::to_string(miss);
        caches["total_request"] = (miss == EMPTY_PMU_VALUE) ? "NA" : std::to_string(hit + miss);
        caches["hit_ratio"] = (miss == EMPTY_PMU_VALUE) ? "NA" :
            std::to_string(pmuCalculatorObj_->CalculatePer(hit, hit + miss));
        coreMemJson["L2cache"] = caches;
        auto cycMap = pmuCalculatorObj_->GetCycleMap(opType, memMapDetail[i]);
        std::vector<nlohmann::json> advice;
        for (const auto &cyc : cycMap) {
            nlohmann::json cycUsage;
            uint64_t totalCycle = (opType == Common::OpType::MIX) ? memMapDetail[i].cycMap[cyc.first] :
                memMapDetail[i].totalCycles;
            cycUsage["cycle"] = cyc.second;
            cycUsage["total_cycles"] = totalCycle;
            float ratio = pmuCalculatorObj_->CalculatePer(cyc.second, totalCycle);
            pipeLineRatio_[cyc.first] = (pipeLineRatio_.count(cyc.first) != 0) ? max(pipeLineRatio_[cyc.first], ratio) : ratio;
            GenCoreUsageAdvice(cyc.first, ratio, advice);
            cycUsage["ratio"] = ratio;
            coreMemJson[cyc.first] = cycUsage;
        }
        coreMemJson["advice"] = advice;
        heatMapJson_.emplace_back(coreMemJson);
        // 生成内存表格
        memMapDetail[i].cycMap["totalCycles"] = memMapDetail[i].totalCycles;
        nlohmann::json memTableJson;
        memTableJson["block_id"] = "NA";
        memTableJson["table_op_type"] = opType;
        memTableJson["table_detail"] = GetAiCoreMemTableJson(memMapDetail[i].cycMap);
        memTableJson["advice"] = GenPipeAdvice(opType);
        mapTableJson_.emplace_back(memTableJson);
    }
}


void StorageAccess910B::LoadLineMap(const std::string &opType)
{
    std::vector<std::string> ubMem = {"UB Read MTE", "UB Write MTE", "UB Read GM",
        "UB Write GM", "UB Write Vector", "UB Read Vector", "UB Read Scalar", "UB Write Scalar"};
    std::map<std::string, std::vector<std::string>> vecTable = { {"UB", ubMem}, {"Vector", vectorMem_} };
    std::map<std::string, std::vector<std::string>> vecTable2 = {
        {"UB Core0", ubMem}, {"Vector Core0", vectorMem_}, {"UB Core1", ubMem}, {"Vector Core1", vectorMem_},
        {"GM Cube", gm_}, {"GM Vector Core0", gm_}, {"GM Vector Core1", gm_},
        {"Pipe Cube", pipeCube_}, {"Pipe Vector Core0", pipeVec_}, {"Pipe Vector Core1", pipeVec_} };
    tableLineAiCore_ = {
        {"GM", gm_}, {"Cache", {"L2 Cache Write", "L2 Cache Read", "L2 Cache Total", "iCache Total"}},
        {"Pipe", {"MTE1", "MTE2", "MTE3", "FIXP", "Scalar"}}
    };
    
    std::map<std::string, std::vector<std::string>> scalarVec = {
        {"Scalar", scalarVec_},
    };
    std::map<std::string, std::vector<std::string>> scalarCube = {
        {"Scalar", scalarCube_},
    };
    std::map<std::string, std::vector<std::string>> scalarMix = {
        {"Scalar Cube", scalarCube_},
        {"Scalar Vectore Core0", scalarVec_},
        {"Scalar Vectore Core1", scalarVec_}
    };
    if (opType == Common::OpType::VECTOR) {
        tableLineAiCore_.insert(vecTable.begin(), vecTable.end());
        tableLineAiCore_.insert(scalarVec.begin(), scalarVec.end());
    } else if (opType == Common::OpType::CUBE) {   // 910B cube
        tableLineAiCore_.insert(cubeTable_.begin(), cubeTable_.end());
        tableLineAiCore_.insert(scalarCube.begin(), scalarCube.end());
    } else {
        tableLineAiCore_.erase("GM");
        tableLineAiCore_.erase("Pipe");
        tableLineAiCore_.insert(vecTable2.begin(), vecTable2.end());
        tableLineAiCore_.insert(cubeTable_.begin(), cubeTable_.end());
        tableLineAiCore_.insert(scalarMix.begin(), scalarMix.end());
    }
}

void StorageAccess310P::LoadLineMap(const std::string &opType)
{
    std::vector<std::string> ubMem = {"UB Read MTE", "UB Write MTE", "UB Read GM",
        "UB Write GM", "UB Write Vector", "UB Read Vector", "UB Read Scalar", "UB Write Scalar"};
    ubMem.push_back("UB Write L0C");
    ubMem.push_back("UB Read L0C");
    tableLineAiCore_ = {
        {"L1", {"L1 Read MTE", "L1 Write MTE", "L1 Read L0A/L0B/UB", "L1 Write GM/UB"}},
        {"L0A", {"L0A Read MTE", "L0A Write Cube", "L0A Read L1"}},
        {"L0B", {"L0B Read MTE", "L0B Write Cube", "L0B Read L1"}},
        {"Cube", {"Cube Read L0A", "Cube Read L0B", "Cube Write L0C"}},
        {"L0C", {"L0C Read Cube", "L0C Write UB", "L0C Read UB"}}, {"UB", ubMem}, {"Vector", vectorMem_},
        {"GM", {"Read Main Memory", "Write Main Memory"}}, {"Cache", {"L2 Cache Total", "iCache Total"}},
        {"Pipe", {"MTE1", "MTE2", "MTE3", "Scalar"}}
    };
}

void StorageAccess910B::LoadMapData(const std::string &opType, MemMapDetail &memMapDetail)
{
    memInfoAiCoreMap_.clear();
    ApiDataTransMemInfo_.clear();
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, memMapDetail.freq, 0, 0, memMapDetail.soc};
    float duration;
    uint64_t cycles;
    if (opType == Common::OpType::MIX && !kernelScale_) {
        duration = GetDurCalBandWidth();
        cycles = static_cast<uint64_t>(duration * memMapDetail.freq);
    } else {
        duration = 0.0f;
        cycles = memMapDetail.totalCycles;
    }
    Profiling::Calculate cal(std::map<uint16_t, uint64_t>(), cycles, calDeviceInfo, duration);
    std::map<uint64_t, uint64_t> eventMap = memMapDetail.eventMap;
    // 基础的pmu数据信息
    std::map<std::string, uint64_t> basicPmu = pmuCalculatorObj_->GetBasicPmu(memMapDetail);
    // 当芯片类型为910B时，还需区分aic/aiv
    AddBasicPmu910B(opType, memMapDetail, basicPmu);
    std::map<std::string, uint64_t> dbiRequest = memMapDetail.ApiDataTransVolume_;
    SetMemInfo910B(opType, basicPmu, cal, dbiRequest);
    // 910B需要区分计算Aic和Aiv数据
    auto memInfoAiCoreMapCube = GetMemInfoAiCoreMapCube910B(basicPmu, cal);
    auto memInfoAiCoreMapVec = GetMemInfoAiCoreMapVec910B(basicPmu, cal);
    if (opType == Common::OpType::VECTOR) {
        memInfoAiCoreMap_.insert(memInfoAiCoreMapVec.begin(), memInfoAiCoreMapVec.end());
    } else if (opType == Common::OpType::CUBE) {   // 910B vector
        memInfoAiCoreMap_.insert(memInfoAiCoreMapCube.begin(), memInfoAiCoreMapCube.end());
    } else {
        memInfoAiCoreMap_.insert(memInfoAiCoreMapCube.begin(), memInfoAiCoreMapCube.end());
        auto memInfoAiCoreMapVecs = GetMemInfoAiCoreMapMix910B(basicPmu, cal);
        memInfoAiCoreMap_.insert(memInfoAiCoreMapVecs.begin(), memInfoAiCoreMapVecs.end());
    }
    LoadApiMemInfo(memMapDetail, cal);
    GetPipePeak(basicPmu, memMapDetail, opType, cal);
}

void StorageAccess910B::LoadApiMemInfo(const MemMapDetail &memMapDetail, Calculate &cal)
{
    if (memInfoAiCoreMap_.count("L0A") == 0 || memInfoAiCoreMap_.count("L0B") == 0) {
        return;
    }
    auto gmTol0aIter = memMapDetail.ApiDataTransVolume_.find(GM_TO_L0A);
    auto gmTol0bIter = memMapDetail.ApiDataTransVolume_.find(GM_TO_L0B);
    if (gmTol0aIter == memMapDetail.ApiDataTransVolume_.end() ||
        gmTol0bIter == memMapDetail.ApiDataTransVolume_.end()) {
        LogDebug("Raw data do not has GM_TO_L0A/B volume");
        return;
    }
    uint64_t gmTol0A = gmTol0aIter->second;
    uint64_t gmTol0B = gmTol0bIter->second;
    if (memInfoAiCoreMap_["L0A"][0].request < gmTol0A || memInfoAiCoreMap_["L0B"][0].request < gmTol0B) {
        LogWarn("Request num is smaller than api memory data");
        return;
    }
    uint64_t l1Tol0A = memInfoAiCoreMap_["L0A"][0].request <= gmTol0A ? 0
        : memInfoAiCoreMap_["L0A"][0].request - gmTol0A;
    uint64_t l1Tol0B = memInfoAiCoreMap_["L0B"][0].request <= gmTol0B ? 0
        : memInfoAiCoreMap_["L0B"][0].request - gmTol0B;
    float l1Tol0AR = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, l1Tol0A});
    float l1Tol0BR = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 8, l1Tol0B});
    float gmTol0AR = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {256, 8, gmTol0A});
    float gmTol0BR = FunctionsFp.at(FuncType::BANDWIDTH_FP)(cal, {128, 8, gmTol0B});
    ApiDataTransMemInfo_[GM_TO_L0A] = {gmTol0A, gmTol0AR, "NA"};
    ApiDataTransMemInfo_[GM_TO_L0B] = {gmTol0B, gmTol0BR, "NA"};
    ApiDataTransMemInfo_[L1_TO_L0A] = {l1Tol0A, l1Tol0AR, "NA"};
    ApiDataTransMemInfo_[L1_TO_L0B] = {l1Tol0B, l1Tol0BR, "NA"};
}

void StorageAccess310P::LoadMapData(const std::string &opType, MemMapDetail &memMapDetail)
{
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND310P, memMapDetail.freq, memMapDetail.aiCoreNum,
                                              memMapDetail.blockDim, memMapDetail.soc};
    uint64_t cycles = memMapDetail.totalCycles;
    Profiling::Calculate cal(std::map<uint16_t, uint64_t>(), cycles, calDeviceInfo);
    std::map<uint64_t, uint64_t> eventMap = memMapDetail.eventMap;
    // 基础的pmu数据信息
    std::map<std::string, uint64_t> basicPmu = pmuCalculatorObj_->GetBasicPmu(memMapDetail);

    // 310P不区分aic/aiv,补充所有缺失的pmu数据
    uint64_t cacheMiss = ((eventMap[120] + eventMap[121]) > eventMap[106]) ?
        (eventMap[120] + eventMap[121]) - eventMap[106] : EMPTY_PMU_VALUE;
    std::map<std::string, uint64_t> data = { {"MTE Write", eventMap[49]}, {"L0A Read", eventMap[28]},
        {"L0A Cube Write", eventMap[27]}, {"L0B Read", eventMap[34]}, {"L0B Cube Write", eventMap[33]},
        {"L0C Write", eventMap[42]}, {"L1 Read", eventMap[50]}, {"UB Write", eventMap[41]},
        {"UB MTE Read", eventMap[62]}, {"UB MTE Write", eventMap[61]}, {"UB Vec Read", eventMap[67]},
        {"UB Vec Write", eventMap[68]}, {"Scalar Read", eventMap[55]}, {"Scalar Write", eventMap[56]},
        {"UB Read", eventMap[39]}, {"L2 Cache Total hit", eventMap[106]}, {"L2 Cache Total miss", cacheMiss},
        {"MTE1 Cyc", eventMap[11]} };
    basicPmu.insert(data.begin(), data.end());
    basicPmu["GM Main Read"] = eventMap[18];   // 310P设备使用pmu18计算GM Main Read
    basicPmu["GM Main Write"] = eventMap[19];  // 310P设备使用pmu19计算GM Main Write
    memInfoAiCoreMap_ = GetMemInfoAiCoreMap310P(basicPmu, cal);
    memInfoCacheMap_ = {
        {"Cache", { {MemInfoCache{basicPmu["L2 Cache Total hit"], basicPmu["L2 Cache Total miss"]}},
            {MemInfoCache{basicPmu["iCache total hit"], basicPmu["iCache total miss"]}} } } };
    memInfoPipeMap_ = {
        {"Pipe", { {MemInfoPipe{basicPmu["MTE1 Ins"], basicPmu["MTE1 Cyc"], basicPmu["MTE1 Wait"]}},
            {MemInfoPipe{basicPmu["MTE2 Ins"], basicPmu["MTE2 Cyc"], basicPmu["MTE2 Wait"]}},
            {MemInfoPipe{basicPmu["MTE3 Ins"], basicPmu["MTE3 Cyc"], basicPmu["MTE3 Wait"]}},
            {MemInfoPipe{0, basicPmu["Scalar Cyc"], basicPmu["Scalar Wait"]}} } } };
}

std::map<std::string, uint64_t> StorageAccessA5::GetCycleMap(const std::string &opType, MemMapDetail &detail) const
{
    std::map<std::string, uint64_t> cycMap;
    if (opType == Common::OpType::CUBE) {
        cycMap["Cube"] = detail.eventMap[810];
    } else if (opType == Common::OpType::VECTOR) {
        cycMap["Vector"] = detail.eventMap[1281];
    } else {
        cycMap["Cube"] = detail.eventMap[810];
        cycMap["Vector"] = detail.eventMapVec0[1281];
        cycMap["Vector1"] = detail.eventMapVec1[1281];
    }
    return cycMap;
}

void StorageAccessA5::PreProcess(bool isKernelScale, bool isMemoryDetail)
{
    kernelScale_ = isKernelScale;
    memoryDetail_ = isMemoryDetail;
    mteBwMap_ = pmuCalculatorObj_->GetPipeBwMap(opBasicInfoObj_->GetSoc());
    std::vector<MemMapDetail> memMapDetail = basicPmuObj_->GetMemMapDetails();
    // 将每个block的数据加到json里
    for (size_t i = 0; i < memMapDetail.size(); ++i) {
        // 生成热力图的json数据格式
        nlohmann::json coreMemJson;
        std::string opType = memMapDetail[i].opType;
        LoadLineMap(opType);                          // 根据芯片类型，确定表格名称
        LoadMapData(opType, memMapDetail[i]);         // 确定表格内所有值

        // 生成内存表格
        memMapDetail[i].cycMap["totalCycles"] = memMapDetail[i].totalCycles;
        nlohmann::json memTableJson;
        memTableJson["block_id"] = std::to_string(memMapDetail[i].blockId);
        memTableJson["table_op_type"] = opType;
        memTableJson["table_detail"] = GetAiCoreMemTableJson(memMapDetail[i].cycMap);
        memTableJson["advice"] = GenPipeAdvice(opType);
        nlohmann::json caches;
        uint64_t hit = memInfoCacheMap_["Cache"][2].hit;
        uint64_t miss = memInfoCacheMap_["Cache"][2].miss;
        caches["hit"] = (hit == EMPTY_PMU_VALUE) ? "NA" : std::to_string(hit);
        caches["miss"] = (miss == EMPTY_PMU_VALUE) ? "NA" : std::to_string(miss);
        caches["total_request"] = (hit == EMPTY_PMU_VALUE || miss == EMPTY_PMU_VALUE) ? "NA" : std::to_string(hit + miss);
        caches["hit_ratio"] = (hit == EMPTY_PMU_VALUE || miss == EMPTY_PMU_VALUE) ? "NA" :
                              std::to_string(pmuCalculatorObj_->CalculatePer(hit, hit + miss));
        coreMemJson["L2cache"] = caches;
        auto cycMap = GetCycleMap(opType, memMapDetail[i]);
        std::vector<nlohmann::json> advice;
        for (const auto &cyc : cycMap) {
            nlohmann::json cycUsage;
            uint64_t totalCycle = (opType == Common::OpType::MIX) ? memMapDetail[i].cycMap[cyc.first] :
                                  memMapDetail[i].totalCycles;
            cycUsage["cycle"] = cyc.second;
            cycUsage["total_cycles"] = totalCycle;
            float ratio = pmuCalculatorObj_->CalculatePer(cyc.second, totalCycle);
            pipeLineRatio_[cyc.first] = (pipeLineRatio_.count(cyc.first) != 0) ? max(pipeLineRatio_[cyc.first], ratio) : ratio;
            GenCoreUsageAdvice(cyc.first, ratio, advice);
            cycUsage["ratio"] = ratio;
            coreMemJson[cyc.first] = cycUsage;
        }
        coreMemJson["advice"] = advice;
        coreMemJson["core_no"] = std::to_string(memMapDetail[i].blockId);
        coreMemJson["op_type"] = opType;
        coreMemJson["soc"] = "910A5";
        coreMemJson["memory_unit"] = GetAiCoreMemUnitJson(opType);
        mapTableJson_.emplace_back(memTableJson);
        heatMapJson_.emplace_back(coreMemJson);
    }
}

void StorageAccessA5::LoadLineMap(const string &opType)
{
    map<string, vector<string>> cubeTable1 = {
        {"L1", {"L1 Read MTE", "L1 Write MTE", "L1 Write L0A/L0B", "L1 Read L0C", "L1 Read GM"}},
        {"L0A", {"L0A Read MTE", "L0A Write Cube", "L0A Read L1/GM"}},
        {"L0B", {"L0B Read MTE", "L0B Write Cube", "L0B Read L1/GM"}},
        {"Cube", {"Cube Read L0A", "Cube Read L0B", "Cube Write L0C", "Cube Read L0C"}},
        {"L0C", {"L0C Read Cube", "L0C Write Cube", "L0C Write GM", "L0C Write L1", "L0C Write FIXP"}}
    };
    vector<string> ubMem = {"UB Read MTE", "UB Write MTE", "UB Read GM", "UB Read Vector", "UB Write Vector"};
    vector<string> dcacheMem = {"DCache Read GM", "DCache Read Vector", "DCache Write Vector"};
    tableLineAiCore_ = {{"Cache", {"L2 Cache Write", "L2 Cache Read", "L2 Cache Total", "iCache Total"}}};
    if (opType == Common::OpType::VECTOR) {
        map<string, vector<string>> vecTable = {{"UB", ubMem}, {"Vector", vectorMem_}, {"Pipe", pipeVec_}, {"GM", gm_}, {"DCache", dcacheMem}};
        tableLineAiCore_.insert(vecTable.begin(), vecTable.end());
    } else if (opType == Common::OpType::CUBE) {
        map<string, vector<string>> cubeTable2 = {{"Pipe", pipeCube_}, {"GM", gm_}};
        tableLineAiCore_.insert(cubeTable1.begin(), cubeTable1.end());
        tableLineAiCore_.insert(cubeTable2.begin(), cubeTable2.end());
    } else {
        vector<string> ubMixMem = {"UB Read L0C"};
        ubMem.insert(ubMem.end(), ubMixMem.begin(), ubMixMem.end());
        std::map<std::string, std::vector<std::string>> mixTable = {
            {"UB Core0", ubMem}, {"Vector Core0", vectorMem_}, {"UB Core1", ubMem}, {"Vector Core1", vectorMem_},
            {"GM Cube", gm_}, {"GM Vector Core0", gm_}, {"GM Vector Core1", gm_},
            {"Pipe Cube", pipeCube_}, {"Pipe Vector Core0", pipeVec_}, {"Pipe Vector Core1", pipeVec_},
            {"DCache Core0", dcacheMem}, {"DCache Core1", dcacheMem}};
        cubeTable1.at("L1").emplace_back("L1 Write UB");
        cubeTable1.at("L1").emplace_back("L1 Read UB");
        cubeTable1.at("L0C").emplace_back("L0C Write UB");
        tableLineAiCore_.insert(cubeTable1.begin(), cubeTable1.end());
        tableLineAiCore_.insert(mixTable.begin(), mixTable.end());
    }
}

std::map<std::string, std::vector<std::string>> StorageAccessA5::GenTableHead()
{
    std::vector<std::string> memTableWithPeak = {"", "requests", "throughput(GB/s)", "peak(%)"};
    std::set<std::string> headInsert = {"L0A", "L0B", "L0C", "L1", "UB", "UB Core0", "UB Core1"};
    for (const auto& h: headInsert) {
        head_.insert({h, memTableWithPeak});
    }
    return head_;
}

MemInfoAiCore StorageAccessA5::CalAiCore(float time, uint64_t pmu, TransportType type, bool calPeak)
{
    float dataNumber = GetDataNumberFp(pmu, REQ_DATA_OF_A5.at(type));
    float bandWidth = BandWidthFp(dataNumber, time);
    if (calPeak && pmu != EMPTY_PMU_VALUE) {
        std::string bandWidthUsage = BandWidthUsage(dataNumber, time, type, opBasicInfoObj_->GetSoc());
        return MemInfoAiCore{pmu, bandWidth, bandWidthUsage};
    } else {
        return MemInfoAiCore{pmu, bandWidth, "NA"};
    }
}

vector<MemInfoCache> StorageAccessA5::LoadCacheData(map<uint64_t, uint64_t> &eventMap) const
{
    string location = "cache";
    uint64_t pmuWriteHit = SafeAdd(eventMap[1066], eventMap[1069], location);
    auto pmuWriteMiss = SafeAddAll<uint64_t>({eventMap[1067], eventMap[1068], eventMap[1070], eventMap[1071]}, location);
    uint64_t pmuWriteTotal = SafeAdd(pmuWriteHit, pmuWriteMiss, location);
    uint64_t pmuReadHit = SafeAdd(eventMap[1060], eventMap[1063], location);
    auto pmuReadMiss = SafeAddAll<uint64_t>({eventMap[1061], eventMap[1062], eventMap[1064], eventMap[1065]}, location);
    uint64_t pmuReadTotal = SafeAdd(pmuReadHit, pmuReadMiss, location);
    return {
        {MemInfoCache{pmuWriteHit, pmuWriteMiss, pmuWriteTotal}},
        {MemInfoCache{pmuReadHit,  pmuReadMiss,  pmuReadTotal}},
        {MemInfoCache{SafeAdd(pmuWriteHit, pmuReadHit, location), SafeAdd(pmuWriteMiss, pmuReadMiss, location),
                      SafeAdd(pmuWriteTotal, pmuReadTotal, location)}},
        {MemInfoCache{SafeSub(eventMap[52], eventMap[53], location, false), eventMap[53], eventMap[52]}},
    };
}

vector<MemInfoPipe> StorageAccessA5::LoadCubePipeData(map<uint64_t, uint64_t> &eventMap, uint64_t totalCycles, int64_t freq, float time, const std::map<std::string, float> &pipeData)
{
    auto bwPipe = [&freq](float data, uint64_t pipeCycle) {
        float pipeTime = freq == 0 ? 0.0f : static_cast<float>(pipeCycle) / freq;
        return BandWidth(data, pipeTime);
    };
    float mte1Data = GetDataNumberFp(eventMap[1799], REQ_DATA_OF_A5.at(TransportType::L1_TO_MTE));
    float mte2Data = GetDataNumberFp(eventMap[1058], REQ_DATA_OF_A5.at(TransportType::GM_TO_L1));
    float mte3Data = pipeData.at("MTE3");
    float fixpData = pipeData.at("FIXP");
    float mte1ActRate = pmuCalculatorObj_->CalculatePer(eventMap[1794], totalCycles);
    float mte2ActRate = pmuCalculatorObj_->CalculatePer(eventMap[514], totalCycles);
    float mte3ActRate = pmuCalculatorObj_->CalculatePer(eventMap[515], totalCycles);
    // add pipe advice if ratio < advicePipe_
    map<string, pair<float, float>> mtePipe = {
        {"MTE1", {mte1ActRate, mte1Data}},
        {"MTE2", {mte2ActRate, mte2Data}},
        {"MTE3", {mte3ActRate, mte3Data}},
    };
    for (const auto& mte: mtePipe) {
        if (!IsZero(mte.second.first) && !IsZero(mteBwMap_[mte.first])) {
            if (BandWidthFp(mte.second.second, time) / mte.second.first / mteBwMap_[mte.first] < idealRatio) {
                advicePipe_.insert(mte.first);
            }
        }
    }
    return {
        {MemInfoPipe{eventMap[1792], eventMap[1794], eventMap[13], bwPipe(mte1Data, eventMap[1794]), mte1ActRate}},
        {MemInfoPipe{eventMap[512],  eventMap[514],  eventMap[14], bwPipe(mte2Data, eventMap[514]),  mte2ActRate}},
        {MemInfoPipe{eventMap[513],  eventMap[515],  eventMap[15], bwPipe(mte3Data, eventMap[515]),  mte3ActRate}},
        {MemInfoPipe{eventMap[1813], eventMap[1812], eventMap[36], bwPipe(fixpData, eventMap[1812]), pmuCalculatorObj_->CalculatePer(eventMap[1812], totalCycles)}},
        {MemInfoPipe{eventMap[0],    eventMap[1],    eventMap[10], "NA", pmuCalculatorObj_->CalculatePer(eventMap[1], totalCycles)}},
    };
}

StorageCubeTableData StorageAccessA5::LoadCubeData(const map<uint64_t, uint64_t> &eventMap, uint64_t totalCycles, int64_t freq, float time)
{
    // eventMap不存在的pmu值默认置0
    auto pmu = eventMap;
    if (IsZero(time)) {
        time = freq == 0 ? 0.0f : static_cast<float>(totalCycles) / freq;
    }
    vector<MemInfoCache> cache = LoadCacheData(pmu);
    string location = "cube storage";
    auto pmuL0cToFixp = SafeAddAll<uint64_t>({pmu[1804], pmu[1806], pmu[1815], pmu[1059]}, location);
    uint64_t pmuL0cToUb = SafeAdd(pmu[1804], pmu[1815], location);
    uint64_t pmuL1ToL0aAndL0b = SafeAdd(pmu[1795], pmu[1797], location);
    uint64_t pmuL1ToUb = SafeSub(pmu[1799], pmuL1ToL0aAndL0b, location, false);
    uint64_t pmuUbToL1 = SafeSub(pmu[1801], SafeAdd(pmu[1806], pmu[1058] / 2, location), location, false);
    auto pmuGmToL2 = SafeAddAll<uint64_t>({pmu[1061], pmu[1062], pmu[1064], pmu[1065]}, location);
    auto pmuL2ToGm = SafeAddAll<uint64_t>({pmu[1062], pmu[1065], pmu[1068], pmu[1071]}, location);
    uint64_t pmuMteToL1 = SafeSub(pmu[1801], pmu[1806], location, false);
    fixpToUbVec0_ = pmu[1804];
    fixpToUbVec1_ = pmu[1815];

    float mte3Data = GetDataNumberFp(pmuUbToL1, REQ_DATA_OF_A5.at(TransportType::MTE_TO_L1));
    float fixpData = GetDataNumberFp(pmuL0cToFixp, REQ_DATA_OF_A5.at(TransportType::L0C_TO_FIXP));
    vector<MemInfoPipe> pipe = LoadCubePipeData(pmu, totalCycles, freq, time, {{"MTE3", mte3Data}, {"FIXP", fixpData}});

    return StorageCubeTableData{cache, pipe,
        // gm
        {CalAiCore(time, pmu[1058], TransportType::READ_MAIN_MEMORY),
         CalAiCore(time, pmu[1059], TransportType::WRITE_MAIN_MEMORY)},
        // cube
        {CalAiCore(time, pmu[772],  TransportType::L0A_TO_CUBE),
         CalAiCore(time, pmu[774],  TransportType::L0B_TO_CUBE),
         CalAiCore(time, pmu[776],  TransportType::CUBE_TO_L0C),
         CalAiCore(time, pmu[778],  TransportType::L0C_TO_CUBE)},
        // l0a
        {CalAiCore(time, pmu[1795],  TransportType::L1_TO_L0A, true),
         CalAiCore(time, pmu[772],   TransportType::L0A_TO_CUBE),
         CalAiCore(time, pmu[1795],  TransportType::L1_TO_L0A, true)},
        // l0b
        {CalAiCore(time, pmu[1797],  TransportType::L1_TO_L0B, true),
         CalAiCore(time, pmu[774],   TransportType::L0B_TO_CUBE),
         CalAiCore(time, pmu[1797],  TransportType::L1_TO_L0B, true)},
        // l0c
        {CalAiCore(time, pmu[776],   TransportType::CUBE_TO_L0C),
         CalAiCore(time, pmu[778],   TransportType::L0C_TO_CUBE),
         CalAiCore(time, pmu[1059],  TransportType::L0C_TO_GM, true),
         CalAiCore(time, pmu[1806],  TransportType::L0C_TO_L1, true),
         CalAiCore(time, pmuL0cToFixp, TransportType::L0C_TO_FIXP),
         CalAiCore(time, pmuL0cToUb, TransportType::L0C_TO_UB, true)},
        // l1
        {CalAiCore(time, pmuMteToL1, TransportType::MTE_TO_L1),
         CalAiCore(time, pmu[1799],  TransportType::L1_TO_MTE),
         CalAiCore(time, pmuL1ToL0aAndL0b, TransportType::L1_TO_L0B, true),
         CalAiCore(time, pmu[1806],  TransportType::L0C_TO_L1, true),
         CalAiCore(time, pmu[1058],  TransportType::GM_TO_L1, true),
         CalAiCore(time, pmuL1ToUb,  TransportType::L1_TO_UB, true),
         CalAiCore(time, pmuUbToL1,  TransportType::MTE_TO_L1)},
        // l2cache
        {CalAiCore(time, pmuGmToL2,  TransportType::READ_MAIN_MEMORY),   // GM -> l2cache
         CalAiCore(time, pmuL2ToGm,  TransportType::WRITE_MAIN_MEMORY)}, // l2cache -> GM
    };
}

vector<MemInfoPipe> StorageAccessA5::LoadVecPipeData(map<uint64_t, uint64_t> &eventMap, uint64_t totalCycles, int64_t freq, float time, const std::map<std::string, float> &mteData)
{
    auto bwPipe = [&freq](float data, uint64_t pipeCycle) {
        float pipeTime = freq == 0 ? 0.0f : static_cast<float>(pipeCycle) / freq;
        return BandWidth(data, pipeTime);
    };
    float mte2Data = mteData.at("MTE2");
    float mte2ActRate = pmuCalculatorObj_->CalculatePer(eventMap[514], totalCycles);
    float mte3ActRate = pmuCalculatorObj_->CalculatePer(eventMap[515], totalCycles);

    // add pipe advice if ratio < advicePipe_
    // MTE3数据量暂无法获取
    map<string, pair<float, float>> mtePipe = {
        {"MTE2 vector", {mte2ActRate, mte2Data}},
    };
    for (const auto& mte: mtePipe) {
        if (!IsZero(mte.second.first) && !IsZero(mteBwMap_[mte.first])) {
            if (BandWidthFp(mte.second.second, time) / mte.second.first / mteBwMap_[mte.first] < idealRatio) {
                advicePipe_.insert(mte.first);
            }
        }
    }
    return {
        {MemInfoPipe{eventMap[512], eventMap[514], eventMap[14], bwPipe(mte2Data, eventMap[514]),  mte2ActRate}},
        {MemInfoPipe{eventMap[513], eventMap[515], eventMap[15], "NA", mte3ActRate}},
        {MemInfoPipe{eventMap[0],   eventMap[1],   eventMap[10], "NA", pmuCalculatorObj_->CalculatePer(eventMap[1], totalCycles)}},
    };
}

StorageVecTableData StorageAccessA5::LoadVecData(const map<uint64_t, uint64_t> &eventMap, uint64_t totalCycles, int64_t freq, float time, int subCoreId)
{
    // eventMap不存在的pmu值默认置0
    auto pmu = eventMap;
    if (IsZero(time)) {
        time = freq == 0 ? 0.0f : static_cast<float>(totalCycles) / freq;
    }
    vector<MemInfoCache> cache = LoadCacheData(pmu);
    string location = "vec storage";
    uint64_t pmuL0cToUb = subCoreId == 0 ? fixpToUbVec0_ : fixpToUbVec1_;
    uint64_t gmToDcache = SafeAdd(pmu[1407], pmu[1408], location);
    uint64_t pmuGmToUb = SafeSub(pmu[1058], gmToDcache, location, false);
    uint64_t dcacheToVec = SafeAdd(pmu[1395], pmu[1396], location);
    uint64_t vecToDcache = SafeAdd(pmu[1397], pmu[1398], location);
    auto pmuGmToL2 = SafeAddAll<uint64_t>({pmu[1061], pmu[1062], pmu[1064], pmu[1065]}, location);
    auto pmuL2ToGm = SafeAddAll<uint64_t>({pmu[1062], pmu[1065], pmu[1068], pmu[1071]}, location);

    float mte2Data = GetDataNumberFp(pmuGmToUb, REQ_DATA_OF_A5.at(TransportType::GM_TO_UB));
    vector<MemInfoPipe> pipe = LoadVecPipeData(pmu, totalCycles, freq, time, {{"MTE2", mte2Data}});

    return StorageVecTableData{cache, pipe,
        // gm
        {CalAiCore(time, pmu[1058],   TransportType::READ_MAIN_MEMORY),
         CalAiCore(time, pmu[1059],   TransportType::WRITE_MAIN_MEMORY)},
        // ub
        {CalAiCore(time, pmu[518],    TransportType::MTE_TO_UB),
         CalAiCore(time, pmu[516],    TransportType::UB_TO_MTE),
         CalAiCore(time, pmuGmToUb,   TransportType::GM_TO_UB, true),
         CalAiCore(time, pmu[1394],   TransportType::VEC_TO_UB),
         CalAiCore(time, pmu[1393],   TransportType::UB_TO_VEC),
         CalAiCore(time, pmuL0cToUb,  TransportType::L0C_TO_UB)},
        // vec
        {CalAiCore(time, pmu[1394],   TransportType::VEC_TO_UB),
         CalAiCore(time, pmu[1393],   TransportType::UB_TO_VEC)},
        // dcache
        {CalAiCore(time, gmToDcache,  TransportType::GM_TO_DCACHE),
         CalAiCore(time, vecToDcache, TransportType::VEC_TO_DCACHE),
         CalAiCore(time, dcacheToVec, TransportType::DCACHE_TO_VEC)},
        // l2cache
        {CalAiCore(time, pmuGmToL2,   TransportType::READ_MAIN_MEMORY),   // GM -> l2cache
         CalAiCore(time, pmuL2ToGm,   TransportType::WRITE_MAIN_MEMORY)}, // l2cache -> GM
    };
}

void StorageAccessA5::LoadMapData(const string &opType, MemMapDetail &memMapDetail)
{
    // time to calculate bandwidth
    float bwTime = 0.0f;
    if (opType == Common::OpType::MIX && !kernelScale_) {
        bwTime = GetDurCalBandWidth();
    }
    std::vector<MemInfoAiCore> l2CacheTransData;
    if (opType == Common::OpType::VECTOR) {
        auto vecData = LoadVecData(memMapDetail.eventMap, memMapDetail.totalCycles, memMapDetail.freq, bwTime);
        memInfoCacheMap_ = {{"Cache", vecData.cache}};
        memInfoAiCoreMap_ = {{"GM", vecData.gm}, {"UB", vecData.ub}, {"Vector", vecData.vec}, {"DCache", vecData.dcache}, {"Cache", vecData.l2cache}};
        memInfoPipeMap_ = {{"Pipe", vecData.pipe}};
    } else if (opType == Common::OpType::CUBE) {
        auto cubeData = LoadCubeData(memMapDetail.eventMap, memMapDetail.totalCycles, memMapDetail.freq, bwTime);
        memInfoCacheMap_ = {{"Cache", cubeData.cache}};
        memInfoAiCoreMap_ = {{"GM", cubeData.gm}, {"Cube", cubeData.cube}, {"L0A", cubeData.l0a},
                             {"L0B", cubeData.l0b}, {"L0C", cubeData.l0c}, {"L1", cubeData.l1}, {"Cache", cubeData.l2cache}};
        memInfoPipeMap_ = {{"Pipe", cubeData.pipe}};
    } else {
        auto cubeData = LoadCubeData(memMapDetail.eventMap, memMapDetail.cycMap["Cube"], memMapDetail.freq, bwTime);
        auto vecData0 = LoadVecData(memMapDetail.eventMapVec0, memMapDetail.cycMap["Vector"], memMapDetail.freq, bwTime, 0);
        auto vecData1 = LoadVecData(memMapDetail.eventMapVec1, memMapDetail.cycMap["Vector1"], memMapDetail.freq, bwTime, 1);
        vector<MemInfoCache> mixCache;
        string location = "mix storage";
        for (size_t i = 0; i < cubeData.cache.size(); i++) { // cubeData and vecDatas have same size
            auto hit = SafeAddAll<uint64_t>({cubeData.cache[i].hit, vecData0.cache[i].hit, vecData1.cache[i].hit}, location);
            auto miss = SafeAddAll<uint64_t>({cubeData.cache[i].miss, vecData0.cache[i].miss, vecData1.cache[i].miss}, location);
            auto total = SafeAddAll<uint64_t>({cubeData.cache[i].total, vecData0.cache[i].total, vecData1.cache[i].total}, location);
            mixCache.emplace_back(MemInfoCache{hit, miss, total});
        }
        vector<MemInfoAiCore> mixL2Cache;
        for (size_t i = 0; i < cubeData.l2cache.size(); i++) { // cubeData and vecDatas have same size
            auto req = SafeAddAll<uint64_t>({cubeData.l2cache[i].request, vecData0.l2cache[i].request, vecData1.l2cache[i].request}, location);
            auto throughput = cubeData.l2cache[i].throughput + vecData0.l2cache[i].throughput + vecData1.l2cache[i].throughput;
            mixL2Cache.emplace_back(MemInfoAiCore{req, throughput});
        }
        memInfoCacheMap_ = {{"Cache", mixCache}};
        memInfoAiCoreMap_ = {
            {"UB Core0", vecData0.ub},         {"UB Core1", vecData1.ub},         {"Vector Core0", vecData0.vec},
            {"Vector Core1", vecData1.vec},    {"GM Vector Core0", vecData0.gm},  {"GM Vector Core1", vecData1.gm},
            {"DCache Core0", vecData0.dcache}, {"DCache Core1", vecData1.dcache}, {"Cache", mixL2Cache},
            {"GM Cube", cubeData.gm},          {"Cube", cubeData.cube},           {"L0A", cubeData.l0a},
            {"L0B", cubeData.l0b},             {"L0C", cubeData.l0c},             {"L1", cubeData.l1}
        };
        memInfoPipeMap_ = {{"Pipe Cube", cubeData.pipe}, {"Pipe Vector Core0", vecData0.pipe}, {"Pipe Vector Core1", vecData1.pipe}};
    }
}

vector<string> StorageAccessA5::GetTableValue(const string &tableName, size_t colum, map<string, uint64_t> &cycMap,
                                              const map<string, vector<string>> &head)
{
    vector<string> rowValue;
    if (tableName == "Cache") {
        MemInfoCache res = memInfoCacheMap_[tableName][colum];
        if (res.hit == EMPTY_PMU_VALUE || res.miss == EMPTY_PMU_VALUE || res.total == EMPTY_PMU_VALUE) {
            rowValue = { "NA", "NA", "NA", "NA" };
        } else {
            rowValue = {to_string(res.hit), to_string(res.miss), to_string(res.total),
                        to_string(pmuCalculatorObj_->CalculatePer(res.hit, res.total))};
        }
    } else if (tableName.find("Pipe") != string::npos) {
        MemInfoPipe res = memInfoPipeMap_[tableName][colum];
        auto pipe = tableLineAiCore_[tableName][colum];
        pipeLineRatio_[pipe] = (pipeLineRatio_.count(pipe) != 0) ? max(pipeLineRatio_[pipe], res.activeRate) : res.activeRate;
        rowValue.emplace_back(res.instr == EMPTY_PMU_VALUE ? "NA" : std::to_string(res.instr));
        rowValue.emplace_back(res.cycle == EMPTY_PMU_VALUE ? "NA" : std::to_string(res.cycle));
        rowValue.emplace_back(res.waitCycle == EMPTY_PMU_VALUE ? "NA" : std::to_string(res.waitCycle));
        rowValue.emplace_back(res.cycle == EMPTY_PMU_VALUE ? "NA" : std::to_string(res.activeRate));
        rowValue.emplace_back(res.cycle == EMPTY_PMU_VALUE ? "NA" : res.bandWidth);
    } else {
        MemInfoAiCore res = memInfoAiCoreMap_[tableName][colum];
        rowValue.emplace_back((res.request == EMPTY_PMU_VALUE) ? "NA" : to_string(res.request));
        rowValue.emplace_back((res.request == EMPTY_PMU_VALUE) ? "NA" : to_string(res.throughput));
        if ((head.count(tableName) != 0)) {
            rowValue.emplace_back(res.peak);
        }
    }
    return rowValue;
}

std::vector<nlohmann::json> StorageAccessA5::GenPipeAdvice(string opType)
{
    std::vector<nlohmann::json> advice;
    // if opType is mix or vector, transfer pipe display
    const static std::map<std::string, std::string> mixPipeMap = {
        {"MTE1",          "aicore MTE1"},
        {"MTE2",          "aicore MTE2"},
        {"MTE3",          "aicore MTE3"},
        {"MTE2 vector",   "aivector MTE2"},
        {"MTE3 vector",   "aivector MTE3"},
    };
    const static std::map<std::string, std::string> vectorPipeMap = {
        {"MTE2 vector",   "MTE2"},
        {"MTE3 vector",   "MTE3"},
    };
    // Generate optimization suggestions
    for (auto &pipe : advicePipe_) {
        auto display = pipe;
        display = opType == OpType::MIX ? mixPipeMap.at(pipe) : display;
        display = opType == OpType::VECTOR ? vectorPipeMap.at(pipe) : display;
        advice.emplace_back(display + " bandwidth utilization lower than 80% when active");
        adviceInfoRecord_.insert(display + " bandwidth utilization lower than 80%% when active");
    }
    return advice;
}
}
