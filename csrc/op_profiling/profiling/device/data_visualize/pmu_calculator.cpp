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

#include "pmu_calculator.h"

#include "common/defs.h"
#include "number_operation.h"
#include "common/hal_helper.h"
#include "profiling/device/data_parse/device_defs.h"

using namespace Common;
using namespace Utility;
using namespace Profiling;
using namespace std;

namespace Visualize {

// L0A:MTE->L0A, L0B:MTE->L0B, MTE1 = (L0A + L0B)/2, FIXP = (L0C2GM + L0C2L1)/2
// MTE2 = max(GM->L0A,GM->L0B,GM->L1), MTE2 vector = MTE->UB, MTE3 vector = UB->MTE
const std::map<std::string, float> BW_910B1 = {{"L0A", 437.5}, {"L0B", 210.5},
    {"L0C2GM", 209.32}, {"L0C2L1", 216.88}, {"MTE1", 324.0}, {"FIXP", 208.155},
    {"MTE2", 340.1}, {"MTE3", 199.43}, {"MTE2 vector", 186.8}, {"MTE3 vector", 220.06}
};

const std::map<std::string, float> BW_910B4 = {{"L0A", 368.2}, {"L0B", 173.81},
    {"L0C2GM", 189.89}, {"L0C2L1", 190.7}, {"MTE1", 271.005}, {"FIXP", 190.295},
    {"MTE2", 222.17}, {"MTE3", 189.89}, {"MTE2 vector", 176.75}, {"MTE3 vector", 195.27}
};

const std::map<std::string, float> BW_910B1_CJ = {{"L0A", 439.32}, {"L0B", 220.1},
    {"L0C2GM", 202.41}, {"L0C2L1", 220.48}, {"MTE1", 329.71}, {"FIXP", 211.45},
    {"MTE2", 302.32}, {"MTE3", 187.72}, {"MTE2 vector", 197.82}, {"MTE3 vector", 219.14}
};

const std::map<std::string, float> BW_910B4_CJ = {{"L0A", 391.82}, {"L0B", 196.3},
    {"L0C2GM", 189.08}, {"L0C2L1", 196.27}, {"MTE1", 294.06}, {"FIXP", 192.675},
    {"MTE2", 274}, {"MTE3", 182.07}, {"MTE2 vector", 176.06}, {"MTE3 vector", 195.52}
};

const std::map<std::string, std::map<std::string, float>> MAX_BW_GM = {
    {"Ascend910B1", BW_910B1}, {"Ascend910B4", BW_910B4}, {"Ascend910B4-1", BW_910B4}
};

const std::map<std::string, std::map<std::string, float>> MAX_BW_GM_CJ = {
    {"Ascend910B1", BW_910B1_CJ}, {"Ascend910B4", BW_910B4_CJ}, {"Ascend910B4-1", BW_910B4_CJ}
};

const std::map<GmType, std::map<std::string, std::map<std::string, float>>> GM_PRODUCT_MAX_CHANNEL_BW = {
    {GmType::CJ, MAX_BW_GM_CJ},
    {GmType::SK, MAX_BW_GM},
    {GmType::SS, MAX_BW_GM},
    {GmType::DEFAULT, MAX_BW_GM}
};

float PmuCalculator::CalculatePer(uint64_t value1, uint64_t value2) const
{
    if (value2 == 0 || value1 > value2) {
        return 0.0f;
    }
    return (static_cast<float>(value1) / value2) * 100;  // 乘100获取百分比
}

std::map<std::string, uint64_t> PmuCalculator::GetCycleMap(const std::string &opType, MemMapDetail &detail) const
{
    std::map<std::string, uint64_t> cycMap;
    std::string location = "cube cycle";
    if (opType != Common::OpType::VECTOR) {
        cycMap["Cube"] = detail.eventMap[10];     // 使用pmu 10 计算cube利用率
    }
    if (opType == Common::OpType::MIX) {
        cycMap["Vector"] = detail.eventMapVec0[8];  // 使用pmu 8 计算vector利用率
        cycMap["Vector1"] = detail.eventMapVec1[8]; // 使用pmu 8 计算vector利用率
    } else if (opType != Common::OpType::CUBE) {
        cycMap["Vector"] = detail.eventMap[8];      // 使用pmu 8 计算vector利用率
    }
    return cycMap;
}

std::map<std::string, uint64_t> PmuCalculator::GetBasicPmu(const MemMapDetail &memMapDetail) const
{
    std::map<uint64_t, uint64_t> eventMap = memMapDetail.eventMap;
    std::map<uint64_t, uint64_t> eventMapVec0 = memMapDetail.eventMapVec0;
    std::map<uint64_t, uint64_t> eventMapVec1 = memMapDetail.eventMapVec1;
    std::string location = "get basic pmu value";
    std::vector<uint64_t> temp1{eventMap[84], eventMapVec0[84], eventMapVec1[84]};
    std::vector<uint64_t> temp2{eventMap[85], eventMapVec0[85], eventMapVec1[85]};
    std::map<std::string, uint64_t> basicPmu = {
        {"iCache total hit", SafeSub(SafeAddAll(temp1, location), SafeAddAll(temp2, location), location)},
        {"iCache total miss", SafeAddAll(temp2, location)},
        {"GM Main Read", SafeAdd(eventMap[1293], eventMap[1294], location)},
        {"GM Main Write", eventMap[1292]},
        {"GM Main Read vec0", SafeAdd(eventMapVec0[1293], eventMapVec0[1294], location)},
        {"GM Main Write vec0", eventMapVec0[1292]},
        {"GM Main Read vec1", SafeAdd(eventMapVec1[1293], eventMapVec1[1294], location)},\
        {"GM Main Write vec1", eventMapVec1[1292]},
        {"MTE1 Ins", eventMap[4]}, {"MTE1 Ins vec0", eventMapVec0[4]}, {"MTE1 Ins vec1", eventMapVec1[4]},
        {"MTE1 Wait", eventMap[90]}, {"MTE1 Wait vec0", eventMapVec0[90]}, {"MTE1 Wait vec1", eventMapVec1[90]},
        {"MTE2 Ins", eventMap[5]}, {"MTE2 Ins vec0", eventMapVec0[5]}, {"MTE2 Ins vec1", eventMapVec1[5]},
        {"MTE2 Cyc", eventMap[12]}, {"MTE2 Cyc vec0", eventMapVec0[12]}, {"MTE2 Cyc vec1", eventMapVec1[12]},
        {"MTE2 Wait", eventMap[91]}, {"MTE2 Wait vec0", eventMapVec0[91]}, {"MTE2 Wait vec1", eventMapVec1[91]},
        {"MTE3 Ins", eventMap[6]}, {"MTE3 Ins vec0", eventMapVec0[6]}, {"MTE3 Ins vec1", eventMapVec1[6]},
        {"MTE3 Cyc", eventMap[13]}, {"MTE3 Cyc vec0", eventMapVec0[13]}, {"MTE3 Cyc vec1", eventMapVec1[13]},
        {"MTE3 Wait", eventMap[92]}, {"MTE3 Wait vec0", eventMapVec0[92]}, {"MTE3 Wait vec1", eventMapVec1[92]},
        {"Scalar Cyc", eventMap[9]}, {"Scalar Cyc vec0", eventMapVec0[9]}, {"Scalar Cyc vec1", eventMapVec1[9]},
        {"Scalar Wait", eventMap[87]}, {"Scalar Wait vec0", eventMapVec0[87]}, {"Scalar Wait vec1", eventMapVec1[87]}
    };
    return basicPmu;
}

void PmuCalculator::Init(std::shared_ptr<BasicPmu> &basicPmuObj)
{
    std::vector<MemMapDetail> memMapDetail = basicPmuObj->GetMemMapDetails();
    for (auto &detail: memMapDetail) {
        LoadLineMap(detail.opType);
        SetMemInfoPipeMap(detail.opType, detail);
    }
}

void PmuCalculator910B::GetMaxBwRateByGmType(std::map<std::string, std::map<std::string, float>> &maxBwRate) const
{
    GmType type = HalHelper::Instance().GetGmType();
    if (GM_PRODUCT_MAX_CHANNEL_BW.find(type) != GM_PRODUCT_MAX_CHANNEL_BW.end()) {
        maxBwRate = GM_PRODUCT_MAX_CHANNEL_BW.at(type);
    } else {
        maxBwRate = GM_PRODUCT_MAX_CHANNEL_BW.at(GmType::DEFAULT);
    }
}

std::map<std::string, std::map<std::string, float>> PmuCalculator910B::GetBandWidthByWeight(uint64_t l0aDatas,
    uint64_t l0bDatas, uint64_t l0cToGmDatas, uint64_t l0cToL1Datas) const
{
    // 实验理论最大带宽，其中MTE1理论带宽待加权计算，初始化为1，避免作为除数时出现除0错误
    std::map<std::string, std::map<std::string, float>> maxBwRate;
    GetMaxBwRateByGmType(maxBwRate);
    float l0aRatio = (l0aDatas == 0 || (l0aDatas + l0bDatas) == 0) ?
        0.0f : static_cast<float>(l0aDatas) / (l0aDatas + l0bDatas);
    float l0bRatio = (l0bDatas == 0 || (l0aDatas + l0bDatas) == 0) ?
        0.0f : static_cast<float>(l0bDatas) / (l0aDatas + l0bDatas);
    if ((!Utility::SafeEqual(l0aRatio, 0.0f)) || (!Utility::SafeEqual(l0bRatio, 0.0f))) {
        maxBwRate["Ascend910B1"]["MTE1"] =
            maxBwRate["Ascend910B1"]["L0A"] * l0aRatio + maxBwRate["Ascend910B1"]["L0B"] * l0bRatio;
        maxBwRate["Ascend910B4"]["MTE1"] =
            maxBwRate["Ascend910B4"]["L0A"] * l0aRatio + maxBwRate["Ascend910B4"]["L0B"] * l0bRatio;
    }

    float l0c2GmRatio = (l0cToGmDatas == 0 || (l0cToGmDatas + l0cToL1Datas) == 0) ?
        0.0f : static_cast<float>(l0cToGmDatas) / (l0cToGmDatas + l0cToL1Datas);
    float l0c2L1Ratio = (l0cToL1Datas == 0 || (l0cToGmDatas + l0cToL1Datas) == 0) ?
        0.0f :  static_cast<float>(l0cToL1Datas) / (l0cToGmDatas + l0cToL1Datas);
    if ((!Utility::SafeEqual(l0c2GmRatio, 0.0f)) || (!Utility::SafeEqual(l0c2L1Ratio, 0.0f))) {
        maxBwRate["Ascend910B1"]["FIXP"] =
                maxBwRate["Ascend910B1"]["L0C2GM"] * l0c2GmRatio + maxBwRate["Ascend910B1"]["L0C2L1"] * l0c2L1Ratio;
        maxBwRate["Ascend910B4"]["FIXP"] =
                maxBwRate["Ascend910B4"]["L0C2GM"] * l0c2GmRatio + maxBwRate["Ascend910B4"]["L0C2L1"] * l0c2L1Ratio;
    }
    float freqRatio = static_cast<float>(FREQ_MAP.at("Ascend910B2")) / FREQ_MAP.at("Ascend910B1");
    maxBwRate["Ascend910B2"] = {
        {"MTE1", maxBwRate["Ascend910B1"]["MTE1"] * freqRatio},
        {"MTE2", maxBwRate["Ascend910B1"]["MTE2"] * freqRatio},
        {"MTE3", maxBwRate["Ascend910B1"]["MTE3"] * freqRatio},
        {"MTE2 vector", maxBwRate["Ascend910B1"]["MTE2 vector"] * freqRatio},
        {"MTE3 vector", maxBwRate["Ascend910B1"]["MTE3 vector"] * freqRatio}
    };
    maxBwRate["Ascend910B3"] = maxBwRate["Ascend910B2"];
    return maxBwRate;
}

void PmuCalculator910B::LoadLineMap(const std::string &opType)
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
    if (opType == Common::OpType::VECTOR) {
        tableLineAiCore_.insert(vecTable.begin(), vecTable.end());
    } else if (opType == Common::OpType::CUBE) {   // 910B cube
        tableLineAiCore_.insert(cubeTable_.begin(), cubeTable_.end());
    } else {
        tableLineAiCore_.erase("GM");
        tableLineAiCore_.erase("Pipe");
        tableLineAiCore_.insert(vecTable2.begin(), vecTable2.end());
        tableLineAiCore_.insert(cubeTable_.begin(), cubeTable_.end());
    }
}

void PmuCalculator310P::LoadLineMap(const std::string &opType)
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

void PmuCalculator310P::SetMemInfoPipeMap(const std::string &opType, MemMapDetail &memMapDetail)
{
    std::map<uint64_t, uint64_t> eventMap = memMapDetail.eventMap;
    // 基础的pmu数据信息
    std::map<std::string, uint64_t> basicPmu = GetBasicPmu(memMapDetail);

    // 310P不区分aic/aiv,补充所有缺失的pmu数据
    std::string location = "l2cache miss";
    uint64_t cacheMiss = ((Utility::SafeAdd(eventMap[120], eventMap[121], location)) > eventMap[106]) ?
        Utility::SafeSub((Utility::SafeAdd(eventMap[120], eventMap[121], location)), eventMap[106], location) :
        EMPTY_PMU_VALUE;
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

    memInfoPipeMap_ = {
        {"Pipe", { {MemInfoPipe{basicPmu["MTE1 Ins"], basicPmu["MTE1 Cyc"], basicPmu["MTE1 Wait"]}},
            {MemInfoPipe{basicPmu["MTE2 Ins"], basicPmu["MTE2 Cyc"], basicPmu["MTE2 Wait"]}},
            {MemInfoPipe{basicPmu["MTE3 Ins"], basicPmu["MTE3 Cyc"], basicPmu["MTE3 Wait"]}},
            {MemInfoPipe{0, basicPmu["Scalar Cyc"], basicPmu["Scalar Wait"]}} } } };
}

void PmuCalculator910B::SetMemInfoPipeMap(const std::string &opType, MemMapDetail &memMapDetail)
{
    // 基础的pmu数据信息
    std::map<std::string, uint64_t> basicPmu = GetBasicPmu(memMapDetail);
    // 当芯片类型为910B时，还需区分aic/aiv
    AddBasicPmu910B(opType, memMapDetail, basicPmu);
    SetMemInfo910B(opType, basicPmu);
}

void PmuCalculator910B::AddBasicPmu910B(const std::string &opType, MemMapDetail &memMapDetail,
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
    std::vector<uint64_t> gmReadTotal{eventMap[1286], eventMap[1290], eventMapVec0[1286],
        eventMapVec0[1290], eventMapVec1[1286], eventMapVec1[1290]};
    std::string location = "add basic pmu";
    std::map<std::string, uint64_t> basic910B = {
        {"L2Cache Write hit", SafeAddAll(l2CacheWriteHit, location)},
        {"L2Cache Read hit", SafeAddAll(l2CacheReadHit, location)},
        {"L2Cache Write miss", SafeAddAll(l2CacheWriteMiss, location)},
        {"L2Cache Read miss", SafeAddAll(l2CacheReadMiss, location)},
        {"MTE1 Cyc", eventMap[770]}, {"MTE1 Cyc vec0", eventMapVec0[770]}, {"MTE1 Cyc vec1", eventMapVec1[770]},
        {"FIXP Ins", eventMap[526]}, {"FIXP Ins vec0", eventMapVec0[526]}, {"FIXP Ins vec1", eventMapVec1[526]},
        {"FIXP Cyc", eventMap[771]}, {"FIXP Cyc vec0", eventMapVec0[771]}, {"FIXP Cyc vec1", eventMapVec1[771]},
        {"GM Read Total", SafeAddAll(gmReadTotal, location)}
    };
    basicPmu.insert(basic910B.begin(), basic910B.end());
    if (opType == Common::OpType::VECTOR) {
        std::map<std::string, uint64_t> dataVector = {
            {"UB MTE Read", eventMap[62]}, {"UB MTE Write", eventMap[61]}, {"UB Vec Read", eventMap[67]},
            {"UB Vec Write", eventMap[68]}, {"Scalar Read", eventMap[55]}, {"Scalar Write", eventMap[56]}
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

void PmuCalculator910B::SetMemInfo910B(const std::string &opType, std::map<std::string, uint64_t> &basicPmu)
{
    memInfoPipeMap_ = {
            {"Pipe", {
                {MemInfoPipe{basicPmu["MTE1 Ins"], basicPmu["MTE1 Cyc"], basicPmu["MTE1 Wait"]}},
                {MemInfoPipe{basicPmu["MTE2 Ins"], basicPmu["MTE2 Cyc"], basicPmu["MTE2 Wait"]}},
                {MemInfoPipe{basicPmu["MTE3 Ins"], basicPmu["MTE3 Cyc"], basicPmu["MTE3 Wait"]}},
                {MemInfoPipe{basicPmu["FIXP Ins"], basicPmu["FIXP Cyc"], 0}},
                {MemInfoPipe{0, basicPmu["Scalar Cyc"], basicPmu["Scalar Wait"]}} } },
    };
    if (opType == Common::OpType::MIX) {
        TableSplit(basicPmu);
    }
}

void PmuCalculator910B::TableSplit(std::map<std::string, uint64_t> &basicPmu)
{
    std::map<std::string, std::vector<MemInfoPipe>> memInfoPipeMapCore = {
        {"Pipe Cube", {
            {MemInfoPipe{basicPmu["MTE1 Ins"], basicPmu["MTE1 Cyc"], basicPmu["MTE1 Wait"]}},
            {MemInfoPipe{basicPmu["MTE2 Ins"], basicPmu["MTE2 Cyc"], basicPmu["MTE2 Wait"]}},
            {MemInfoPipe{basicPmu["MTE3 Ins"], basicPmu["MTE3 Cyc"], basicPmu["MTE3 Wait"]}},
            {MemInfoPipe{basicPmu["FIXP Ins"], basicPmu["FIXP Cyc"], 0}},
            {MemInfoPipe{0, basicPmu["Scalar Cyc"], basicPmu["Scalar Wait"]}}} },
        {"Pipe Vector Core0", {
            {MemInfoPipe{basicPmu["MTE1 Ins vec0"], basicPmu["MTE1 Cyc vec0"], basicPmu["MTE1 Wait vec0"]}},
            {MemInfoPipe{basicPmu["MTE2 Ins vec0"], basicPmu["MTE2 Cyc vec0"], basicPmu["MTE2 Wait vec0"]}},
            {MemInfoPipe{basicPmu["MTE3 Ins vec0"], basicPmu["MTE3 Cyc vec0"], basicPmu["MTE3 Wait vec0"]}},
            {MemInfoPipe{basicPmu["FIXP Ins vec0"], basicPmu["FIXP Cyc vec0"], 0}},
            {MemInfoPipe{0, basicPmu["Scalar Cyc vec0"], basicPmu["Scalar Wait vec0"]}}} },
        {"Pipe Vector Core1", {
            {MemInfoPipe{basicPmu["MTE1 Ins vec1"], basicPmu["MTE1 Cyc vec1"], basicPmu["MTE1 Wait vec1"]}},
            {MemInfoPipe{basicPmu["MTE2 Ins vec1"], basicPmu["MTE2 Cyc vec1"], basicPmu["MTE2 Wait vec1"]}},
            {MemInfoPipe{basicPmu["MTE3 Ins vec1"], basicPmu["MTE3 Cyc vec1"], basicPmu["MTE3 Wait vec1"]}},
            {MemInfoPipe{basicPmu["FIXP Ins vec1"], basicPmu["FIXP Cyc vec1"], 0}},
            {MemInfoPipe{0, basicPmu["Scalar Cyc vec1"], basicPmu["Scalar Wait vec1"]}}} },
    };
    memInfoPipeMap_.erase("Pipe");
    memInfoPipeMap_.insert(memInfoPipeMapCore.begin(), memInfoPipeMapCore.end());
    memInfoPipeMap_.erase("GM");
}

std::map<std::string, uint64_t> PmuCalculator910B::GetDataCube910B(std::map<uint64_t, uint64_t> &eventMap) const
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
    };
    return dataCube;
}

std::map<std::string, uint64_t> PmuCalculator910B::GetDataVector910BMix(MemMapDetail &memMapDetail) const
{
    std::map<std::string, uint64_t> dataVectors = {
        {"UB MTE Read", memMapDetail.eventMapVec0[62]},
        {"UB MTE Write", memMapDetail.eventMapVec0[61]},
        {"UB Vec Read", memMapDetail.eventMapVec0[67]},
        {"UB Vec Write", memMapDetail.eventMapVec0[68]},
        {"Scalar Read", memMapDetail.eventMapVec0[55]},
        {"Scalar Write", memMapDetail.eventMapVec0[56]},
        {"UB MTE Read1", memMapDetail.eventMapVec1[62]},
        {"UB MTE Write1", memMapDetail.eventMapVec1[61]},
        {"UB Vec Read1", memMapDetail.eventMapVec1[67]},
        {"UB Vec Write1", memMapDetail.eventMapVec1[68]},
        {"Scalar Read1", memMapDetail.eventMapVec1[55]},
        {"Scalar Write1", memMapDetail.eventMapVec1[56]}
    };
    return dataVectors;
}

float PmuCalculator910B::GetDurCalBandWidth(std::unique_ptr<OpBasicInfo> &opBasicInfoObj) const
{
    auto dur = opBasicInfoObj->GetDuration();
    if (std::fabs(dur) <= std::numeric_limits<float>::epsilon()) {
        Utility::LogError("Get op basic info [Task Duration] = %f failed.", dur);
        return 1.0f;
    }
    return dur;
}

map<string, float> PmuCalculatorA5::GetPipeBwMap(const string &socVersion)
{
    auto iter = MAX_BW_RATE_A5.find(socVersion);
    map<TransportType, float> bw;
    if (iter == MAX_BW_RATE_A5.end()) {
        bw = MAX_BW_RATE_A5.at("Ascend910_9599");
    } else {
        bw = iter->second;
    }
    return {{"MTE1", max(max(bw.at(TransportType::L1_TO_L0A), bw.at(TransportType::L1_TO_L0B)), bw.at(TransportType::L1_TO_UB))},
            {"MTE2", bw.at(TransportType::GM_TO_L1)},
            {"MTE3", bw.at(TransportType::UB_TO_L1)},
            {"FIXP", max(max(bw.at(TransportType::L0C_TO_GM), bw.at(TransportType::L0C_TO_L1)), bw.at(TransportType::L0C_TO_UB))},
            {"MTE2 vector", bw.at(TransportType::GM_TO_UB)},
            {"MTE3 vector", max(bw.at(TransportType::UB_TO_GM), bw.at(TransportType::UB_TO_L1))}
    };
}
}
