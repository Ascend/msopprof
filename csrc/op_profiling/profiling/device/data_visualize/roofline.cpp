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

#include "roofline.h"
#include "log.h"
#include "number_operation.h"
#include "common/hal_helper.h"

using namespace std;
using namespace Common;
using namespace Utility;
using namespace Profiling;

namespace Visualize {

static constexpr char const *MEMORY_UNIT_CUBE = "Memory Unit(Cube)";
static constexpr char const *MEMORY_UNIT_VEC = "Memory Unit(Vector)";
static constexpr char const *GM_AND_L2CACHE = "GM/L2";
static constexpr char const *PIPE_LINE_CUBE = "Pipeline(Cube)";
static constexpr char const *PIPE_LINE_VEC = "Pipeline(Vector)";
static constexpr char const *MEMORY_PIPE_CUBE = "Memory Transfer(Cube)";
static constexpr char const *MEMORY_PIPE_VEC = "Memory Transfer(Vector)";

AnalysisPoint RoofLine::analysisPoint_ = { 1.0f, "" };

void RoofLine::Init()
{
    // init calculation
    Profiling::CalDeviceInfo calDeviceInfo = {chipType_, freq_, aiCoreNum_, 0, opBasicInfoObj_->GetSoc()};
    Profiling::Calculate cal(map<uint16_t, uint64_t>(), 0, calDeviceInfo);
    cal_ = cal;
    opType_ = opBasicInfoObj_->GetOpType();
    // sum all sub block pmu value by specified events
    for (const auto &pair: basicPmuObj_->GetTotalPmuData()) {
        std::vector<Event> sumEvents = {};
        if (pair.second.blockType == OpType::CUBE) {
            sumEvents = cubeCoreEvents_;
        } else {
            sumEvents = vecCoreEvents_;
        }
        for (const auto &eventId: sumEvents) {
            uint64_t pmuValue = GetPmuValue(pair.second.pmuEventValueMap, static_cast<const uint64_t>(eventId));
            if (pmuValue == EMPTY_PMU_VALUE) {
                pmuValue = 0;
            }
            std::string location = "roofline";
            pmuValues_[eventId] = SafeAdd(pmuValues_[eventId], pmuValue, location);
        }
    }
}

void RoofLine::CalCubeBaseData(int64_t cubeNum)
{
    // Include cubeFops for all cube core data, cubeTheoryTfops for max cube core bandwidth, cubeTheoryComputility for
    // theory computility percentage.
    uint64_t cubeFos = pmuValues_[Event::L0B_READ_REQ];
    vector<string> computilities;
    if (pmuValues_[Event::CUBE_INT_EXECUTED] != 0) {
        vector<uint64_t> cubeFopsPmus{0, cubeFos};
        cubeProperty_.fops = FunctionsInt.at(FuncType::CUBE_FOPS_INT)(cal_, cubeFopsPmus);
        computilities.emplace_back("Cube_INT(100%)");
    } else {
        vector<uint64_t> cubeFopsPmus{cubeFos, 0};
        cubeProperty_.fops = FunctionsInt.at(FuncType::CUBE_FOPS_INT)(cal_, cubeFopsPmus);
        computilities.emplace_back("Cube_FP(100%)");
    }

    cubeProperty_.computilityName = Join(computilities.begin(), computilities.end(), ",");
    if (cubeFos != 0) {
        cubeProperty_.theoryTfops = static_cast<float>(cubeProperty_.fops) * freq_ * cubeNum / cubeFos /
            TIME_CONVERSION / TIME_CONVERSION;
    }
}

void RoofLine::CalVecBaseData(int64_t vectorNum)
{
    // Include vecFops for all vector core data, vecTheoryTfops for max vector core bandwidth, vecTheoryComputility for
    // theory computility percentage.
    uint64_t fp32Pmu = pmuValues_[Event::VEC_FP32_EXECUTED];
    if (chipType_ == Common::ChipType::ASCEND910B) {
        // fp32Pmu-fp16Lane128Pmu only for 910B davinci bug
        fp32Pmu = (pmuValues_[Event::VEC_FP32_EXECUTED] >= pmuValues_[Event::VEC_FP16_EXECUTED_128]) ?
            pmuValues_[Event::VEC_FP32_EXECUTED] - pmuValues_[Event::VEC_FP16_EXECUTED_128] : 0;
    }

    vector<uint64_t> vecFopsPmus{fp32Pmu, pmuValues_[Event::VEC_FP16_EXECUTED_128],
        pmuValues_[Event::VEC_FP16_EXECUTED_64], pmuValues_[Event::VEC_FP16_EXECUTED_32],
        pmuValues_[Event::VEC_S32_EXECUTED], pmuValues_[Event::VEC_S16_EXECUTED], pmuValues_[Event::VEC_MISC_EXECUTED]};
    vecProperty_.fops = FunctionsInt.at(FuncType::VEC_FOPS_INT)(cal_, vecFopsPmus);
    std::string location = "vector base data";
    uint64_t vecTotalPmu = SafeAddAll(vecFopsPmus, location);
    map<string, uint64_t> computilityMap = {
        {"Vec_FP32", fp32Pmu},
        {"Vec_FP16_128", pmuValues_[Event::VEC_FP16_EXECUTED_128]},
        {"Vec_FP16_64", pmuValues_[Event::VEC_FP16_EXECUTED_64]},
        {"Vec_FP16_32", pmuValues_[Event::VEC_FP16_EXECUTED_32]},
        {"Vec_S32", pmuValues_[Event::VEC_S32_EXECUTED]},
        {"Vec_S16", pmuValues_[Event::VEC_S16_EXECUTED]},
        {"Vec_MISC", pmuValues_[Event::VEC_MISC_EXECUTED]},
    };
    vector<string> computilities;
    for (const auto &pair: computilityMap) {
        if (pair.second != 0 && vecTotalPmu != 0) {
            float percentage = static_cast<float>(pair.second) * PERCENTAGE_CONVERSION / vecTotalPmu;
            computilities.emplace_back(pair.first + "(" + to_string(percentage) + "%)");
        }
    }
    vecProperty_.computilityName = Join(computilities.begin(), computilities.end(), ",");
    if (vecTotalPmu != 0) {
        vecProperty_.theoryTfops = static_cast<float>(vecProperty_.fops  * static_cast<uint64_t>(freq_) *
             static_cast<uint64_t>(vectorNum)) / vecTotalPmu / TIME_CONVERSION / TIME_CONVERSION;
    }
}

std::string RoofLine::RoofLineAnalysis(float bandwidth, float theoryTfops, float horizontalPoint,
                                       float verticalPoint) const
{
    //        ^(OPS(OPS/s))
    //        |
    //        |   P1(OPS/BW,theoryTfops)
    //        |    *________________(theoryTfops(TFLOPS/s))
    //        |   /             P2
    //        |  /               *(horizontalPoint, verticalPoint)
    //        | /(BW(TB/s))
    //        |/
    //    ----+------------------------->(OPs/Mem(Ops/Bytes))
    // P1(OPS/BW, theoryTfops)  P2(horizontalPoint, verticalPoint)
    if (verticalPoint > theoryTfops || SafeEqual(bandwidth, 0.0f) || SafeEqual(verticalPoint, theoryTfops)) {
        return "NA";
    }
    if ((horizontalPoint < (theoryTfops / bandwidth)) && (horizontalPoint * bandwidth < verticalPoint)) {
        return "NA";
    }
    float highPullThroughPoint = horizontalPoint * bandwidth;
    std::string boundType = BoundType::MEMORY_BOUND;
    if (horizontalPoint > theoryTfops / bandwidth) {
        highPullThroughPoint = theoryTfops;
        boundType = BoundType::COMPUTE_BOUND;
    }
    if (SafeEqual(highPullThroughPoint, 0.0f)) {
        return "NA";
    }
    std::string location = "analysis roofline";
    float usageRatio = SafeSub(highPullThroughPoint, verticalPoint, location) / highPullThroughPoint;
    if (usageRatio > 0.0f && usageRatio < analysisPoint_.usageRatio) {
        analysisPoint_.usageRatio = usageRatio;
        analysisPoint_.boundType = boundType;
    }
    return (usageRatio < 1.0f) ? std::to_string((1.0f - usageRatio)) : "NA";
}

void RoofLine::SetPipeLineRatio(const std::string& pipeName, float ratio)
{
    if (pipeLineRatio_.count(pipeName) != 0) {
        pipeLineRatio_[pipeName] = std::max(pipeLineRatio_[pipeName], ratio);
    } else {
        pipeLineRatio_[pipeName] = ratio;
    }
}

std::string RoofLine::GenerateAdvice()
{
    static constexpr float boundThreshold = 0.2f;
    static constexpr float pipeThreshold = 80.0f;
    if (analysisPoint_.usageRatio < 0.0f) {
        return "";
    }
    if (analysisPoint_.usageRatio < boundThreshold) {
        return analysisPoint_.boundType;
    }
    float maxPipeRatio = -1.0f;
    std::string pipeName;
    for (auto &ratio : pipeLineRatio_) {
        if (ratio.second > maxPipeRatio) {
            maxPipeRatio = ratio.second;
            pipeName = ratio.first;
        }
    }
    if (maxPipeRatio < pipeThreshold) {
        return BoundType::LATENCY_BOUND_PIPELINE_CAUSED;
    } else {
        if (pipeName.find("MTE") != std::string::npos) {
            return BoundType::LATENCY_BOUND_MEMORY_CAUSED;
        }
        return BoundType::LATENCY_BOUND_COMPUTE_CAUSED;
    }
}

void RoofLine::AddJson(const string &title, const vector<SubCoreProperty> &propertyVec,
                       const vector<RoofLineData> &roofLineDatas)
{
    float duration = opBasicInfoObj_->GetDuration();

    nlohmann::json singleTypeJson;
    vector<nlohmann::json> roofLines;
    for (const auto& property: propertyVec) {
        // Vertical coordinate = (FP ops + INT ops) / duration, uint is Tops/s.
        float verticalPoint = 0.0f;
        if (!SafeEqual(duration, 0.0f)) {
            verticalPoint = static_cast<float>(property.fops) / duration / TIME_CONVERSION / TIME_CONVERSION;
        }
        for (const auto& singleLineData: roofLineDatas) {
            // Horizontal coordinate = (FP ops + INT ops) / dataVolume, uint is ops/Byte
            // if dataVolume is 0, this point is meaningless
            if (singleLineData.dataVolume == 0) {
                continue;
            }
            float horizontalPoint = static_cast<float>(property.fops) / singleLineData.dataVolume;
            nlohmann::json singleLineJson;
            singleLineJson["computility_name"] = property.computilityName;
            singleLineJson["computility"] = property.theoryTfops;
            singleLineJson["bw_name"] = singleLineData.bandwidthName;
            singleLineJson["bw"] = singleLineData.bandwidth;
            singleLineJson["point"] = {horizontalPoint, verticalPoint};
            auto ratio = RoofLineAnalysis(singleLineData.bandwidth, property.theoryTfops, horizontalPoint, verticalPoint);
            singleLineJson["ratio"] = ratio;
            roofLines.emplace_back(singleLineJson);
        }
    }
    singleTypeJson["title"] = title;
    singleTypeJson["rooflines"] = roofLines;
    roofLineJson_.emplace_back(singleTypeJson);
}

void RoofLine::CalMemoryUnitBw(int64_t coreNum, const std::map<std::string, uint16_t> &memoryUnitBitsPerSecond)
{
    for (auto &memoryUnitBits : memoryUnitBitsPerSecond) {
        // freq unit is MHZ, res unit is TB/s
        maxBwRates_[memoryUnitBits.first] = static_cast<float>(memoryUnitBits.second) * freq_ * coreNum *
            TIME_CONVERSION * TIME_CONVERSION / BIT_CONVERSION / BIT_CONVERSION / BIT_CONVERSION / BIT_CONVERSION;
    }
}

vector<RoofLineData> RoofLine::GetRoofLineData(map<string, uint64_t> &computilityDataMap)
{
    vector<RoofLineData> roofLineDatas;
    for (const auto &pair: computilityDataMap) {
        roofLineDatas.push_back({pair.first, pair.second, maxBwRates_[pair.first]});
    }
    return roofLineDatas;
}

void RoofLine::RoofLineToJson()
{
    visualRoofLineJson_["multiple_rooflines"] = GenerateRoofLines();
    std::string roofLineAdvice = GenerateAdvice();
    visualRoofLineJson_["advice"] = roofLineAdvice;
    if (!roofLineAdvice.empty()) {
        roofLineAdvice = ("\t" + roofLineAdvice + "\n");
        Utility::LogSummary("RoofLine Summary Report:\n\n" + roofLineAdvice);
    }
}

void RoofLine::ClearRoofLineJson()
{
    visualRoofLineJson_.clear();
}

void RoofLine::SetAllPipeLineRatio()
{
    std::vector<MemMapDetail> memMapDetail = basicPmuObj_->GetMemMapDetails();
    // 将每个block的数据加到json里
    for (size_t i = 0; i < memMapDetail.size(); ++i) {
        std::string opType = memMapDetail[i].opType;
        auto cycMap = pmuCalculatorObj_->GetCycleMap(opType, memMapDetail[i]);
        for (const auto &cyc : cycMap) {
            uint64_t totalCycle = (opType == Common::OpType::MIX) ? memMapDetail[i].cycMap[cyc.first] :
                memMapDetail[i].totalCycles;
            float ratio = pmuCalculatorObj_->CalculatePer(cyc.second, totalCycle);
            SetPipeLineRatio(cyc.first, ratio);
        }
    }
}

vector<nlohmann::json> RoofLineOf910B::GenerateRoofLines()
{
    Init();
    if (opType_ == OpType::CUBE || opType_ == OpType::MIX) {
        cubeNum_ = aiCoreNum_;
        CalMemoryUnitBw(cubeNum_, memoryUnitBitsPerSecondCube_);
        CalCubeBaseData(cubeNum_);
        // only use cube core theory tfops, actual fops
        CubeMemoryUnit();
        CubePipeLine();
        CubeMemoryPipe();
    }
    if (opType_ == OpType::VECTOR || opType_ == OpType::MIX) {
        uint16_t aivNumOfAiCore = 2;
        vecNum_ = aiCoreNum_ * aivNumOfAiCore;
        CalMemoryUnitBw(vecNum_, memoryUnitBitsPerSecondVec_);
        CalVecBaseData(vecNum_);
        // only use vector core theory tfops, actual fops
        VecMemoryUnit();
        VectorPipeLine();
        VectorMemoryPipe();
    }
    // use cube and vector core theory tfops, actual fops
    GmAndL2cache();
    return roofLineJson_;
}

void RoofLineOf910B::CubeMemoryUnit()
{
    uint64_t mteToL1Req = pmuValues_[Event::MTE_TO_L1_REQ] >= pmuValues_[Event::FIXP_TO_L1_REQ] ?
        pmuValues_[Event::MTE_TO_L1_REQ] - pmuValues_[Event::FIXP_TO_L1_REQ] : 0;
    std::map<std::string, uint64_t> cubeMemoryUnitPmu = {
        {"mteToL1Data", mteToL1Req * REQ_DATA_OF_910B.at(TransportType::MTE_TO_L1)},
        {"l1ToMteData", pmuValues_[Event::L1_TO_MTE_REQ] * REQ_DATA_OF_910B.at(TransportType::L1_TO_MTE)},
        {"fixpToL1Data", pmuValues_[Event::FIXP_TO_L1_REQ] * REQ_DATA_OF_910B.at(TransportType::FIXP_TO_L1)},
        {"mteToL0aData", pmuValues_[Event::MTE_TO_L0A_REQ] * REQ_DATA_OF_910B.at(TransportType::MTE_TO_L0A)},
        {"mteToL0bData", pmuValues_[Event::MTE_TO_L0B_REQ] * REQ_DATA_OF_910B.at(TransportType::MTE_TO_L0B)},
        {"l0cToFixpData", pmuValues_[Event::L0C_TO_FIXP_REQ] * REQ_DATA_OF_910B.at(TransportType::L0C_TO_FIXP)}
    };
    basicPmu_.insert(cubeMemoryUnitPmu.begin(), cubeMemoryUnitPmu.end());
    map<string, uint64_t> computilityDataMap = {
        {string(MemoryUnit::L1_TOTAL),      basicPmu_["mteToL1Data"] + basicPmu_["l1ToMteData"] +
            basicPmu_["fixpToL1Data"]},
        {string(MemoryUnit::WRITE_TO_L1),   basicPmu_["mteToL1Data"] + basicPmu_["fixpToL1Data"]},
        {string(MemoryUnit::READ_FROM_L1),  basicPmu_["l1ToMteData"]},
        {string(MemoryUnit::WRITE_TO_L0A),  basicPmu_["mteToL0aData"]},
        {string(MemoryUnit::WRITE_TO_L0B),  basicPmu_["mteToL0bData"]},
        {string(MemoryUnit::READ_FROM_L0C), basicPmu_["l0cToFixpData"]}
    };
    auto roofLineDatas = GetRoofLineData(computilityDataMap);
    AddJson(MEMORY_UNIT_CUBE, {cubeProperty_}, roofLineDatas);
}

void RoofLineOf910B::InsertPipeLineMaxBw(int64_t coreNum)
{
    std::string socVersion = opBasicInfoObj_->GetSoc();
    std::string socName = (FREQ_MAP.count(socVersion) == 0) ? "Ascend910B1" : socVersion;
    auto maxBwRate = pmuCalculatorObj_->GetBandWidthByWeight(basicPmu_["mteToL0aData"],
        basicPmu_["mteToL0bData"], basicPmu_["l0cWriteGm"], basicPmu_["fixpToL1Data"]);
    for (auto &bw : maxBwRate[socName]) {
        bw.second /= BIT_CONVERSION;
        bw.second *= coreNum;
    }
    maxBwRates_.insert(maxBwRate[socName].begin(), maxBwRate[socName].end());
}

void RoofLineOf910B::InsertMemPipeMaxBw(int64_t coreNum)
{
    std::map<std::string, std::map<std::string, float>> memoryPipeMaxBwRate;
    GetPipeMaxBwByGmType(memoryPipeMaxBwRate);
    std::string socVersion = opBasicInfoObj_->GetSoc();
    std::string socName = (FREQ_MAP.count(socVersion) == 0) ? "Ascend910B1" : socVersion;
    float freqRatio = static_cast<float>(FREQ_MAP.at("Ascend910B2")) / FREQ_MAP.at("Ascend910B1");
    for (auto &bwPair : memoryPipeMaxBwRate["Ascend910B1"]) {
        memoryPipeMaxBwRate["Ascend910B2"][bwPair.first] = bwPair.second * freqRatio;
    }
    memoryPipeMaxBwRate["Ascend910B3"] = memoryPipeMaxBwRate["Ascend910B2"];
    for (auto &bw : memoryPipeMaxBwRate[socName]) {
        bw.second /= BIT_CONVERSION;
        bw.second *= coreNum;
    }
    maxBwRates_.insert(memoryPipeMaxBwRate[socName].begin(), memoryPipeMaxBwRate[socName].end());
}

void RoofLineOf910B::CubePipeLine()
{
    std::string location = "cube pipe";
    uint64_t l1WriteReq = SafeAdd(4 * pmuValues_[Event::MAIN_MEM_REQ], pmuValues_[Event::FIXP_TO_L1_REQ],
        location) >= pmuValues_[Event::L0C_TO_FIXP_REQ] ?
        (4 * pmuValues_[Event::MAIN_MEM_REQ] - pmuValues_[Event::L0C_TO_FIXP_REQ] +
        pmuValues_[Event::FIXP_TO_L1_REQ]) : 0;
    uint64_t l0cWriteGmReq = (pmuValues_[Event::L0C_TO_FIXP_REQ] >= pmuValues_[Event::FIXP_TO_L1_REQ]) ?
        (pmuValues_[Event::L0C_TO_FIXP_REQ] - pmuValues_[Event::FIXP_TO_L1_REQ]) : 0;
    std::map<std::string, uint64_t> cubePipeLinePmu = {
        {"l1WriteGm", SafeMul(l1WriteReq,
            static_cast<uint64_t>(REQ_DATA_OF_910B.at(TransportType::L1_TO_GM)), location)},
        {"writeMainMem", SafeMul(pmuValues_[Event::WRITE_DATA],
            static_cast<uint64_t>(REQ_DATA_OF_910B.at(TransportType::WRITE_MAIN_MEMORY)), location)},
        {"l0cWriteGm", SafeMul(l0cWriteGmReq,
            static_cast<uint64_t>(REQ_DATA_OF_910B.at(TransportType::L0C_TO_GM)), location)},
    };
    basicPmu_.insert(cubePipeLinePmu.begin(), cubePipeLinePmu.end());
    uint64_t mte1Data = (basicPmu_["l1ToMteData"] >= basicPmu_["l1WriteGm"]) ?
        basicPmu_["l1ToMteData"] - basicPmu_["l1WriteGm"] : 0;
    std::vector<uint64_t> mte2DataPmu{basicPmu_["mteToL0aData"], basicPmu_["mteToL0bData"], basicPmu_["mteToL1Data"]};
    uint64_t mte2Data = SafeAddAll(mte2DataPmu, location);
    mte2Data = mte2Data >= basicPmu_["l1WriteGm"] ? mte2Data - basicPmu_["l1WriteGm"] : 0;
    uint64_t mte3Data = basicPmu_["writeMainMem"] >= basicPmu_["l0cWriteGm"] ?
        (basicPmu_["writeMainMem"] - basicPmu_["l0cWriteGm"]) : 0;
    map<string, uint64_t> computilityDataMap = {
        {"MTE1", mte1Data},
        {"MTE2", mte2Data},
        {"MTE3", mte3Data},
        {"FIXP", basicPmu_["l0cToFixpData"]},
    };
    InsertPipeLineMaxBw(cubeNum_);
    auto roofLineDatas = GetRoofLineData(computilityDataMap);
    AddJson(PIPE_LINE_CUBE, {cubeProperty_}, roofLineDatas);
}

void RoofLineOf910B::CubeMemoryPipe()
{
    map<string, uint64_t> computilityDataMap = {
        {string(MemoryPipe::L1_TO_GM), basicPmu_["l1WriteGm"]},
        {string(MemoryPipe::L0C_TO_GM), basicPmu_["l0cWriteGm"]},
        {string(MemoryPipe::L0C_TO_L1), basicPmu_["fixpToL1Data"]},
        {string(MemoryPipe::GM_L1_TO_L0A), basicPmu_["mteToL0aData"]},
        {string(MemoryPipe::GM_L1_TO_L0B), basicPmu_["mteToL0bData"]},
    };
    InsertMemPipeMaxBw(cubeNum_);
    auto roofLineDatas = GetRoofLineData(computilityDataMap);
    AddJson(MEMORY_PIPE_CUBE, {cubeProperty_}, roofLineDatas);
}

void RoofLineOf910B::VecMemoryUnit()
{
    std::string location = "vector memory uint";
    basicPmu_["ubReadMte"] = SafeMul(pmuValues_[Event::MTE_TO_UB_REQ],
        static_cast<uint64_t>(REQ_DATA_OF_910B.at(TransportType::MTE_TO_UB)), location);
    basicPmu_["ubWriteMte"] = SafeMul(pmuValues_[Event::UB_TO_MTE_REQ],
        static_cast<uint64_t>(REQ_DATA_OF_910B.at(TransportType::UB_TO_MTE)), location);
    basicPmu_["ubToVecData"] = SafeMul(pmuValues_[Event::UB_TO_VEC_REQ],
        static_cast<uint64_t>(REQ_DATA_OF_910B.at(TransportType::UB_TO_VEC)), location);
    basicPmu_["vecToUbData"] = SafeMul(pmuValues_[Event::VEC_TO_UB_REQ],
        static_cast<uint64_t>(REQ_DATA_OF_910B.at(TransportType::VEC_TO_UB)), location);
    std::vector<uint64_t> ubToTal{basicPmu_["ubReadMte"], basicPmu_["ubWriteMte"],
        basicPmu_["ubToVecData"], basicPmu_["vecToUbData"]};
    map<string, uint64_t> computilityDataMap = {
        {string(MemoryUnit::UB_TOTAL),        SafeAddAll(ubToTal, location)},
        {string(MemoryUnit::READ_FROM_UB),    basicPmu_["ubWriteMte"]},
        {string(MemoryUnit::WRITE_TO_UB),     basicPmu_["ubReadMte"]},
        {string(MemoryUnit::VECTOR_READ_UB),  basicPmu_["ubToVecData"]},
        {string(MemoryUnit::VECTOR_WRITE_UB), basicPmu_["vecToUbData"]},
    };
    auto roofLineDatas = GetRoofLineData(computilityDataMap);
    AddJson(MEMORY_UNIT_VEC, {vecProperty_}, roofLineDatas);
}

void RoofLineOf910B::VectorPipeLine()
{
    InsertPipeLineMaxBw(vecNum_);
    vector<RoofLineData> roofLineDatas = {
        {"MTE2", basicPmu_["ubReadMte"], maxBwRates_["MTE2 vector"]},
        {"MTE3", basicPmu_["ubWriteMte"], maxBwRates_["MTE3 vector"]}
    };
    AddJson(PIPE_LINE_VEC, {vecProperty_}, roofLineDatas);
}

void RoofLineOf910B::VectorMemoryPipe()
{
    map<string, uint64_t> computilityDataMap = {
        {string(MemoryPipe::UB_TO_GM), basicPmu_["ubWriteMte"]},
        {string(MemoryPipe::GM_TO_UB), basicPmu_["ubReadMte"]},
    };
    InsertMemPipeMaxBw(vecNum_);
    auto roofLineDatas = GetRoofLineData(computilityDataMap);
    AddJson(MEMORY_PIPE_VEC, {vecProperty_}, roofLineDatas);
}

void RoofLineOf910B::GmAndL2cache()
{
    std::string location = "gm and l2cache";
    // pmu(1280)
    uint64_t coreToL2 = pmuValues_[Event::WRITE_HIT];
    // pmu(1284) + pmu(1288)
    uint64_t l2ToCore = SafeAdd(pmuValues_[Event::READ_HIT_R0], pmuValues_[Event::READ_HIT_R1], location);
    // pmu(1286) + pmu(1290)
    uint64_t gmToL2 = SafeAdd(pmuValues_[Event::READ_MISS_R0], pmuValues_[Event::READ_MISS_R1], location);
    // min(l2cacheEvict, pmu(1282))
    uint64_t l2ToGm =  pmuValues_[Event::WRITE_MISS];
    uint64_t l2ToGmReal = pmuValues_[Event::WRITE_MISS];
    if (l2CacheEvict_ >= 0) { l2ToGmReal = min(static_cast<uint64_t>(l2CacheEvict_), pmuValues_[Event::WRITE_MISS]); }
    std::vector<uint16_t> gmByte {REQ_DATA_OF_910B.at(TransportType::GM_TO_L2),
        REQ_DATA_OF_910B.at(TransportType::L2_TO_GM)};
    std::vector<uint64_t> gmReq {gmToL2, l2ToGm};
    std::vector<uint64_t> gmReqReal {gmToL2, l2ToGmReal};
    std::vector<uint16_t> l2Byte {REQ_DATA_OF_910B.at(TransportType::CORE_TO_L2),
        REQ_DATA_OF_910B.at(TransportType::L2_TO_CORE)};
    std::vector<uint64_t> l2Req {coreToL2, l2ToCore};
 
    uint64_t gmReadAndWriteData = SafeMulAddAll(gmReq, gmByte, location);
    uint64_t gmReadAndWriteRealData = SafeMulAddAll(gmReqReal, gmByte, location);
    uint64_t l2ReadAndWriteData = SafeMulAddAll(l2Req, l2Byte, location);
    l2ReadAndWriteData = SafeAdd(l2ReadAndWriteData, gmReadAndWriteData, location);

    string socVersion = opBasicInfoObj_->GetSoc();
    float gmReadAndWriteBw;
    std::map<std::string, float> gmBw;
    std::map<std::string, float> l2CacheBw;
    GetTheoryBwByGmType(gmBw);
    GetTheoryL2CacheByGmType(l2CacheBw);
    auto gmIt = gmBw.find(socVersion);
    if (gmIt == gmBw.end()) {
        LogDebug("Missing theoretical bandwidth for soc %s, using default value", socVersion.c_str());
        gmReadAndWriteBw = gmBw.at("Ascend910B1");
    } else {
        gmReadAndWriteBw = gmBw.at(socVersion);
    }
    float l2ReadAndWriteBw;
    auto l2cacheIt = l2CacheBw.find(socVersion);
    if (l2cacheIt == l2CacheBw.end()) {
        LogDebug("Missing theoretical L2 bandwidth for soc %s, using default value.", socVersion.c_str());
        l2ReadAndWriteBw = l2CacheBw.at("Ascend910B1");
    } else {
        l2ReadAndWriteBw = l2CacheBw.at(socVersion);
    }
    vector<RoofLineData> roofLineDatas{
        {string(GmAndL2cacheUnit::GM), gmReadAndWriteRealData, gmReadAndWriteBw},
        {string(GmAndL2cacheUnit::L2_CACHE), l2ReadAndWriteData, l2ReadAndWriteBw},
    };
    vector<string> computilities = {cubeProperty_.computilityName, vecProperty_.computilityName};
    string computilityName = Join(computilities.begin(), computilities.end(), " + ");
    SubCoreProperty property = {SafeAdd(cubeProperty_.fops, vecProperty_.fops, location),
                                SafeAdd(cubeProperty_.theoryTfops, vecProperty_.theoryTfops, location), computilityName};
    AddJson(GM_AND_L2CACHE, {property}, roofLineDatas);
}

vector<nlohmann::json> RoofLineOfA5::GenerateRoofLines()
{
    Init();
    InsertMaxBw();
    if (opType_ == OpType::CUBE || opType_ == OpType::MIX) {
        cubeNum_ = aiCoreNum_;
        CalMemoryUnitBw(cubeNum_, memoryUnitBitsPerSecondCube_);
        CalCubeBaseData(cubeNum_);
        // only use cube core theory tfops, actual fops
        CubeMemoryUnit();
        CubePipeLine();
        CubeMemoryPipe();
    }
    if (opType_ == OpType::VECTOR || opType_ == OpType::MIX) {
        uint16_t aivNumOfAiCore = 2;
        vecNum_ = aiCoreNum_ * aivNumOfAiCore;
        CalMemoryUnitBw(vecNum_, memoryUnitBitsPerSecondVec_);
        CalVecBaseData(vecNum_);
        // only use vector core theory tfops, actual fops
        VecMemoryUnit();
    }
    // use cube and vector core theory tfops, actual fops
    GmAndL2cache();
    return roofLineJson_;
}

void RoofLineOfA5::CalCubeBaseData(int64_t cubeNum)
{
    // Include cubeFops for all cube core data, cubeTheoryTfops for max cube core bandwidth, cubeTheoryComputility for theory computility percentage.
    static const map<OperandType, string> dataTypeMap = {
        {OperandType::DATA_S8,   "Integer"},           // INT8
        {OperandType::DATA_F32,  "Single Precision"},
        {OperandType::DATA_F16,  "Half Precision"},
        {OperandType::DATA_BF16, "Half Precision"},
        {OperandType::DATA_E4M3, "Quarter Precision"}, // FP8
        {OperandType::DATA_E5M2, "Quarter Precision"}, // FP8
        {OperandType::DATA_E1M2, "Eighth Precision"},  // FP4
        {OperandType::DATA_E2M1, "Eighth Precision"},  // FP4
    };
    static const map<string, uint64_t> typeOpsMap = {
        {"Single Precision",  4096},
        {"Half Precision",    8192},
        {"Quarter Precision", 16384},
        {"Integer",           16384},
        {"Eighth Precision",  32768},
    };
    map<string, OperandRecord> records;
    const vector<ComputeLoadBlockDetail>& details = basicPmuObj_->GetComputeLoadBlockDetail();
    for (const auto &detail: details) {
        auto simdMap = detail.operandRecordMap.simdMap;
        if (detail.blockType.find("cube") == std::string::npos) {
            continue;
        }
        for (const auto &simd: simdMap) {
            OperandType operandType = simd.first;
            auto it = dataTypeMap.find(operandType);
            if (it == dataTypeMap.end() || simd.second.operands == 0) {
                continue;
            }
            records[it->second].operands = SafeAdd(records[it->second].operands, simd.second.operands, "roofline operands");
        }
    }
    for (const auto &record: records) {
        SubCoreProperty property;
        property.computilityName = record.first;
        property.fops = record.second.operands;
        property.theoryTfops = static_cast<float>(typeOpsMap.at(record.first)) * freq_ * cubeNum / TIME_CONVERSION / TIME_CONVERSION;
        cubeProperties_.emplace_back(property);
    }
}

static void UpdateOperandRecordsForCalVecBaseData(map<string, OperandRecord>& records, OperandType operandType, uint64_t operands)
{
    if (operands == 0) {
        return;
    }
    if (operandType <= OperandType::DATA_FLOAT_MAX) {
        records["Float"].operands = SafeAdd(records["Float"].operands, operands, "roofline operands");
    } else {
        records["Integer"].operands = SafeAdd(records["Integer"].operands, operands, "roofline operands");
    }
}

void RoofLineOfA5::CalVecBaseData(int64_t vectorNum)
{
    map<string, OperandRecord> records;
    const vector<ComputeLoadBlockDetail>& details = basicPmuObj_->GetComputeLoadBlockDetail();
    for (const auto &detail: details) {
        auto simtMap = detail.operandRecordMap.simtMap;
        if (detail.blockType.find("vector") == std::string::npos) {
            continue;
        }
        for (const auto &simt: simtMap) {
            UpdateOperandRecordsForCalVecBaseData(records, simt.first, simt.second.operands);
        }
    }
    constexpr float FLOAT_THREAD_PER_WARP = 32;
    constexpr float INT_THREAD_PER_WARP = 16;
    constexpr float WARP_PER_CORE = 4;
    const float theoryTfopsFloat = FLOAT_THREAD_PER_WARP * WARP_PER_CORE * vectorNum * freq_ / (TIME_CONVERSION * TIME_CONVERSION);
    const float theoryTfopsInt = INT_THREAD_PER_WARP * WARP_PER_CORE * vectorNum * freq_ / (TIME_CONVERSION * TIME_CONVERSION);
    const map<string, float> theoryTfopsMap = {
        {"Float", theoryTfopsFloat},
        {"Integer", theoryTfopsInt},
    };
    for (const auto &record: records) {
        SubCoreProperty property;
        property.computilityName = record.first;
        property.fops = record.second.operands;
        auto it = theoryTfopsMap.find(record.first);
        if (it != theoryTfopsMap.end()) {
            property.theoryTfops = it->second;
        }
        vecSimtProperties_.emplace_back(property);
    }
}

void RoofLineOfA5::InsertMaxBw()
{
    // 部分带宽直接获取，部分通过CalMemoryUnitBw计算
    std::string socVersion = opBasicInfoObj_->GetSoc();
    auto pipeBwMap = pmuCalculatorObj_->GetPipeBwMap(socVersion);
    for (auto &bw : pipeBwMap) {
        bw.second /= BIT_CONVERSION;
    }
    maxBwRates_.insert(pipeBwMap.begin(), pipeBwMap.end());
    auto iter = MAX_BW_RATE_A5.find(socVersion);
    map<TransportType, float> bw;
    if (iter == MAX_BW_RATE_A5.end()) {
        bw = MAX_BW_RATE_A5.at("Ascend910_9599");
    } else {
        bw = iter->second;
    }
    maxBwRates_[string(MemoryUnit::WRITE_TO_L0A)] = bw.at(TransportType::L1_TO_L0A) / BIT_CONVERSION;
    maxBwRates_[string(MemoryUnit::WRITE_TO_L0B)] = bw.at(TransportType::L1_TO_L0B) / BIT_CONVERSION;
    maxBwRates_[string(MemoryPipe::L0C_TO_GM)] = bw.at(TransportType::L0C_TO_GM) / BIT_CONVERSION;
    maxBwRates_[string(MemoryPipe::L0C_TO_L1)] = bw.at(TransportType::L0C_TO_L1) / BIT_CONVERSION;
    maxBwRates_[string(MemoryPipe::GM_L1_TO_L0A)] = maxBwRates_[string(MemoryUnit::WRITE_TO_L0A)];
    maxBwRates_[string(MemoryPipe::GM_L1_TO_L0B)] = maxBwRates_[string(MemoryUnit::WRITE_TO_L0B)];
}

void RoofLineOfA5::CubeMemoryUnit()
{
    uint64_t mteToL1Data = GetDataNumber(pmuValues_[Event::WR_L1_INSTR], REQ_DATA_OF_A5.at(TransportType::MTE_TO_L1));
    uint64_t l0cToL1Data = GetDataNumber(pmuValues_[Event::FIXP_WR_L1_INSTR], REQ_DATA_OF_A5.at(TransportType::L0C_TO_L1));
    uint64_t l1ToMteData = GetDataNumber(pmuValues_[Event::RD_L1_INSTR], REQ_DATA_OF_A5.at(TransportType::L1_TO_MTE));
    uint64_t l1ToL0aData = GetDataNumber(pmuValues_[Event::WR_L0A_INSTR], REQ_DATA_OF_A5.at(TransportType::L1_TO_L0A));
    uint64_t l1ToL0bData = GetDataNumber(pmuValues_[Event::WR_L0B_INSTR], REQ_DATA_OF_A5.at(TransportType::L1_TO_L0B));
    uint64_t l0cToFixpData = GetDataNumber(pmuValues_[Event::FIXP_RD_L0C_INSTR], REQ_DATA_OF_A5.at(TransportType::L0C_TO_FIXP));

    std::map<std::string, uint64_t> cubeMemoryUnitPmu = {
        {"l0cToL1Data", l0cToL1Data},
        {"l1ToL0aData", l1ToL0aData},
        {"l1ToL0bData", l1ToL0bData},
        {"l0cToFixpData", l0cToFixpData},
    };
    basicPmu_.insert(cubeMemoryUnitPmu.begin(), cubeMemoryUnitPmu.end());
    map<string, uint64_t> computilityDataMap = {
        {string(MemoryUnit::L1_TOTAL),      mteToL1Data + l0cToL1Data + l1ToMteData},
        {string(MemoryUnit::WRITE_TO_L1),   mteToL1Data + l0cToL1Data},
        {string(MemoryUnit::READ_FROM_L1),  l1ToMteData},
        {string(MemoryUnit::WRITE_TO_L0A),  l1ToL0aData},
        {string(MemoryUnit::WRITE_TO_L0B),  l1ToL0bData},
        {string(MemoryUnit::READ_FROM_L0C), l0cToFixpData}
    };
    auto roofLineDatas = GetRoofLineData(computilityDataMap);
    AddJson(MEMORY_UNIT_CUBE, cubeProperties_, roofLineDatas);
}

void RoofLineOfA5::CubePipeLine()
{
    uint64_t pmuUbToGm = SafeSub(pmuValues_[Event::WRITE_DATA_SENT],
        SafeAdd(pmuValues_[Event::DCU_REQ_STG], pmuValues_[Event::DCU_REQ_STK], "pmuUbToGm"), "pmuUbToGm", false);
    uint64_t pmuL1ToUb = SafeSub(pmuValues_[Event::AIC_EXT_RD_UB_INSTR], SafeAddAll<uint64_t>({pmuUbToGm,
        pmuValues_[Event::FIXP_WR_UB_INSTR], pmuValues_[Event::FIXP_WR_UB1_INSTR]}, "pmuL1ToUb"), "pmuL1ToUb", false);
    uint64_t l1ToUbData = GetDataNumber(pmuL1ToUb, REQ_DATA_OF_A5.at(TransportType::L1_TO_UB));
    uint64_t gmToL1Data = GetDataNumber(pmuValues_[Event::READ_DATA_RECEIVED], REQ_DATA_OF_A5.at(TransportType::GM_TO_L1));
    uint64_t pmuUbToL1 = SafeSub(pmuValues_[Event::WR_L1_INSTR], SafeAdd(pmuValues_[Event::FIXP_WR_L1_INSTR],
        pmuValues_[Event::READ_DATA_RECEIVED], "pmuUbToL1"), "pmuUbToL1", false);
    uint64_t ubToL1Data = GetDataNumber(pmuUbToL1, REQ_DATA_OF_A5.at(TransportType::UB_TO_L1));
    map<string, uint64_t> computilityDataMap = {
        {"MTE1", basicPmu_["l1ToL0aData"] + basicPmu_["l1ToL0bData"] + l1ToUbData},
        {"MTE2", gmToL1Data},
        {"MTE3", ubToL1Data},
        {"FIXP", basicPmu_["l0cToFixpData"]},
    };
    auto roofLineDatas = GetRoofLineData(computilityDataMap);
    AddJson(PIPE_LINE_CUBE, cubeProperties_, roofLineDatas);
}

void RoofLineOfA5::CubeMemoryPipe()
{
    uint64_t pmuL0cToGm = SafeSub(pmuValues_[Event::FIXP_RD_L0C_INSTR], SafeAddAll<uint64_t>({pmuValues_[Event::FIXP_WR_UB_INSTR],
        pmuValues_[Event::FIXP_WR_L1_INSTR], pmuValues_[Event::FIXP_WR_UB1_INSTR]}, "pmuL0cToGm"), "pmuL0cToGm", false);
    uint64_t l0cToGmData = GetDataNumber(pmuL0cToGm, REQ_DATA_OF_A5.at(TransportType::L0C_TO_GM));
    map<string, uint64_t> computilityDataMap = {
        {string(MemoryPipe::L0C_TO_GM), l0cToGmData},
        {string(MemoryPipe::L0C_TO_L1), basicPmu_["l0cToL1Data"]},
        {string(MemoryPipe::GM_L1_TO_L0A), basicPmu_["l1ToL0aData"]},
        {string(MemoryPipe::GM_L1_TO_L0B), basicPmu_["l1ToL0bData"]},
    };
    auto roofLineDatas = GetRoofLineData(computilityDataMap);
    AddJson(MEMORY_PIPE_CUBE, cubeProperties_, roofLineDatas);
}

void RoofLineOfA5::VecMemoryUnit()
{
    uint64_t dcuReqLdg = GetDataNumber(pmuValues_[Event::DCU_REQ_LDG], REQ_DATA_OF_A5.at(TransportType::GM_TO_DCACHE));
    uint64_t dcuReqLdk = GetDataNumber(pmuValues_[Event::DCU_REQ_LDK], REQ_DATA_OF_A5.at(TransportType::GM_TO_DCACHE));
    uint64_t dcuReqStg = GetDataNumber(pmuValues_[Event::DCU_REQ_STG], REQ_DATA_OF_A5.at(TransportType::DCACHE_TO_GM));
    uint64_t dcuReqStk = GetDataNumber(pmuValues_[Event::DCU_REQ_STK], REQ_DATA_OF_A5.at(TransportType::DCACHE_TO_GM));

    map<string, uint64_t> computilityDataMap = {
        {string(MemoryUnit::SIMT_VF),      dcuReqLdg + dcuReqLdk + dcuReqStg + dcuReqStk},
    };
    auto roofLineDatas = GetRoofLineData(computilityDataMap);
    AddJson(MEMORY_UNIT_VEC, vecSimtProperties_, roofLineDatas);
}

void RoofLineOfA5::GmAndL2cache()
{
    std::string location = "gm and l2cache";
    uint64_t coreToL2 = SafeAdd(pmuValues_[Event::AW_CLOSE_L2_HIT_CORE], pmuValues_[Event::AW_FAR_L2_HIT_CORE], location);
    uint64_t l2ToCore = SafeAdd(pmuValues_[Event::AR_CLOSE_L2_HIT_CORE], pmuValues_[Event::AR_FAR_L2_HIT_CORE], location);
    uint64_t gmToL2 = pmuValues_[Event::AR_FAR_L2_MISS_CORE];
    uint64_t l2ToGm = SafeAdd(pmuValues_[Event::AR_CLOSE_L2_VICTIM_CORE], pmuValues_[Event::AW_FAR_L2_VICTIM_CORE], location);
    std::vector<uint64_t> gmReq {gmToL2, l2ToGm};     // read miss, write evict
    std::vector<uint64_t> l2Req {l2ToCore, coreToL2}; // read hit, write hit
    std::vector<uint16_t> reqByte {REQ_DATA_OF_A5.at(TransportType::READ_MAIN_MEMORY), REQ_DATA_OF_A5.at(TransportType::WRITE_MAIN_MEMORY)};
    uint64_t gmReadAndWriteData = SafeMulAddAll(gmReq, reqByte, location);
    uint64_t l2ReadAndWriteData = SafeMulAddAll(l2Req, reqByte, location);
    l2ReadAndWriteData = SafeAdd(l2ReadAndWriteData, gmReadAndWriteData, location);
    // TB/s, for all socVersion
    const float gmBw = 1.6;
    const float l2Bw = 8.8;
    vector<RoofLineData> roofLineDatas {
        {string(GmAndL2cacheUnit::GM), gmReadAndWriteData, gmBw},
        {string(GmAndL2cacheUnit::L2_CACHE), l2ReadAndWriteData, l2Bw},
    };

    auto statistical = [](const SubCoreProperty &origin, SubCoreProperty &res) {
        res.fops = SafeAdd(res.fops, origin.fops, "fops");
        res.theoryTfops = SafeAdd(res.theoryTfops, origin.theoryTfops, "theoryTfops");
    };
    vector<SubCoreProperty> propertyVec;
    SubCoreProperty newInteger = {0, 0, "Integer"};
    for (const auto &p: cubeProperties_) {
        if (p.computilityName == "Integer") {
            statistical(p, newInteger);
        } else {
            propertyVec.push_back({p.fops, p.theoryTfops, "Cube " + p.computilityName});
        }
    }
    for (const auto &p: vecSimtProperties_) {
        if (p.computilityName == "Integer") {
            statistical(p, newInteger);
        } else {
            propertyVec.push_back({p.fops, p.theoryTfops, "SIMT " + p.computilityName});
        }
    }
    if (newInteger.fops != 0) {
        propertyVec.emplace_back(newInteger);
    }
    AddJson(GM_AND_L2CACHE, {propertyVec}, roofLineDatas);
}

vector<nlohmann::json> RoofLineOf310P::GenerateRoofLines()
{
    Init();
    CalCubeBaseData(aiCoreNum_);
    CalVecBaseData(aiCoreNum_);
    CalMemoryUnitBw(aiCoreNum_, memoryUnitBitsPerSecond_);
    MemoryUnit();
    return roofLineJson_;
}

void RoofLineOf310P::MemoryUnit()
{
    std::string location = "memory";
    uint64_t mteToL1Data = SafeMul(pmuValues_[Event::MTE_TO_L1_REQ],
        static_cast<uint64_t>(REQ_DATA_OF_310P.at(TransportType::MTE_TO_L1)), location);
    uint64_t l1ToMteData = SafeMul(pmuValues_[Event::L1_TO_MTE_REQ],
        static_cast<uint64_t>(REQ_DATA_OF_310P.at(TransportType::L1_TO_MTE)), location);
    uint64_t mteToL0aData = SafeMul(pmuValues_[Event::MTE_TO_L0A_REQ],
        static_cast<uint64_t>(REQ_DATA_OF_310P.at(TransportType::MTE_TO_L0A)), location);
    uint64_t mteToL0bData = SafeMul(pmuValues_[Event::MTE_TO_L0B_REQ],
        static_cast<uint64_t>(REQ_DATA_OF_310P.at(TransportType::MTE_TO_L0B)), location);
    uint64_t l0cToVecData = SafeMul(pmuValues_[Event::LOC_TO_VEC_REQ],
        static_cast<uint64_t>(REQ_DATA_OF_310P.at(TransportType::L0C_TO_VEC)), location);
    uint64_t ubToMteData = SafeMul(pmuValues_[Event::UB_TO_MTE_REQ],
        static_cast<uint64_t>(REQ_DATA_OF_310P.at(TransportType::UB_TO_MTE)), location);
    uint64_t mteToUbData = SafeMul(pmuValues_[Event::MTE_TO_UB_REQ],
        static_cast<uint64_t>(REQ_DATA_OF_310P.at(TransportType::MTE_TO_UB)), location);
    uint64_t ubToVecData = SafeMul(pmuValues_[Event::UB_TO_VEC_REQ],
        static_cast<uint64_t>(REQ_DATA_OF_310P.at(TransportType::UB_TO_VEC)), location);
    uint64_t vecToUbData = SafeMul(pmuValues_[Event::VEC_TO_UB_REQ],
        static_cast<uint64_t>(REQ_DATA_OF_310P.at(TransportType::VEC_TO_UB)), location);
    map<string, uint64_t> computilityDataMap = {
        {string(MemoryUnit::L1_TOTAL),        SafeAdd(mteToL1Data, l1ToMteData, location)},
        {string(MemoryUnit::WRITE_TO_L1),     mteToL1Data},
        {string(MemoryUnit::READ_FROM_L1),    l1ToMteData},
        {string(MemoryUnit::WRITE_TO_L0A),    mteToL0aData},
        {string(MemoryUnit::WRITE_TO_L0B),    mteToL0bData},
        {string(MemoryUnit::READ_FROM_L0C),   l0cToVecData},
        {string(MemoryUnit::UB_TOTAL),        SafeAddAll(std::vector<uint64_t>{ubToMteData,
                                                         mteToUbData, ubToVecData, vecToUbData}, location)},
        {string(MemoryUnit::READ_FROM_UB),    ubToMteData},
        {string(MemoryUnit::WRITE_TO_UB),     mteToUbData},
        {string(MemoryUnit::VECTOR_READ_UB),  ubToVecData},
        {string(MemoryUnit::VECTOR_WRITE_UB), vecToUbData},
    };
    auto roofLineDatas = GetRoofLineData(computilityDataMap);
    vector<string> computilities = {cubeProperty_.computilityName, vecProperty_.computilityName};
    string computilityName = Join(computilities.begin(), computilities.end(), " + ");
    SubCoreProperty property = {SafeAdd(cubeProperty_.fops, vecProperty_.fops, location),
                                SafeAdd(cubeProperty_.theoryTfops, vecProperty_.theoryTfops, location),
                                computilityName};
    AddJson(MEMORY_UNIT, {property}, roofLineDatas);
}

void RoofLineOf910B::GetTheoryBwByGmType(std::map<std::string, float> &gmBw) const
{
    GmType type = HalHelper::Instance().GetGmType();
    if (GM_PRODUCT_THEORY_BW.find(type) != GM_PRODUCT_THEORY_BW.end()) {
        gmBw = GM_PRODUCT_THEORY_BW.at(type);
    } else {
        gmBw = GM_PRODUCT_THEORY_BW.at(GmType::DEFAULT);
    }
}
 
void RoofLineOf910B::GetTheoryL2CacheByGmType(std::map<std::string, float> &l2CacheBw) const
{
    GmType type = HalHelper::Instance().GetGmType();
    if (GM_PRODUCT_THEORY_L2CACHE_BW.find(type) != GM_PRODUCT_THEORY_L2CACHE_BW.end()) {
        l2CacheBw = GM_PRODUCT_THEORY_L2CACHE_BW.at(type);
    } else {
        l2CacheBw = GM_PRODUCT_THEORY_L2CACHE_BW.at(GmType::DEFAULT);
    }
}
 
void RoofLineOf910B::GetPipeMaxBwByGmType(std::map<std::string, std::map<std::string, float>>
    &memoryPipeMaxBwRate) const
{
    GmType type = HalHelper::Instance().GetGmType();
    if (GM_PRODUCT_MAX_BW_REQ.find(type) != GM_PRODUCT_MAX_BW_REQ.end()) {
        memoryPipeMaxBwRate = GM_PRODUCT_MAX_BW_REQ.at(type);
    } else {
        memoryPipeMaxBwRate = GM_PRODUCT_MAX_BW_REQ.at(GmType::DEFAULT);
    }
}
}
