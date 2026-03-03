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

#ifndef MSOPT_ROOFLINE_H
#define MSOPT_ROOFLINE_H
#include <utility>
#include <vector>
#include <string>
#include <map>
#include "json.hpp"

#include "basic_op_and_pmu.h"
#include "pmu_calculator.h"
#include "data_visualize_const.h"
#include "profiling/device/data_parse/pmu_calculate.h"

namespace Visualize {
struct RoofLineData {
    std::string bandwidthName;
    // actual data volume, the unit is B
    uint64_t dataVolume;
    // theory bandwidth, the unit is TB/s
    float bandwidth;
};

struct SubCoreProperty {
    uint64_t fops;
    float theoryTfops;
    // eg: "Cube_FP(60%),Cube_INT(40%)"
    std::string computilityName;
};

struct AnalysisPoint {
    float usageRatio;
    std::string boundType;
    AnalysisPoint(float ratio, std::string type) : usageRatio(ratio), boundType(std::move(type)) {}
};

class RoofLine {
public:
    RoofLine(const int64_t aicoreFreq, const int64_t aiCoreNum, std::shared_ptr<OpBasicInfo> &opBasicInfoObj,
             std::shared_ptr<BasicPmu> &basicPmuObj, std::unique_ptr<Visualize::PmuCalculator> &pmuCalculatorObj)
        : opBasicInfoObj_(opBasicInfoObj),
          basicPmuObj_(basicPmuObj),
          pmuCalculatorObj_(pmuCalculatorObj),
          cal_(std::map<uint16_t, uint64_t>(), 0, Profiling::CalDeviceInfo{})
    {
        freq_ = aicoreFreq;
        aiCoreNum_ = aiCoreNum;
    };
    void Init();
    virtual void CalCubeBaseData(int64_t cubeNum);
    virtual void CalVecBaseData(int64_t vectorNum);
    void AddJson(const std::string &title, const std::vector<SubCoreProperty> &propertyVec,
                 const std::vector<RoofLineData> &roofLineDatas);
    std::string RoofLineAnalysis(float bandwidth, float theoryTfops, float horizontalPoint, float verticalPoint) const;
    void SetPipeLineRatio(const std::string& pipeName, float ratio);
    std::string GenerateAdvice();
    void CalMemoryUnitBw(int64_t coreNum, const std::map<std::string, uint16_t> &memoryUnitBitsPerSecond);
    std::vector<RoofLineData> GetRoofLineData(std::map<std::string, uint64_t> &computilityDataMap);
    virtual std::vector<nlohmann::json> GenerateRoofLines() { return {}; };
    void RoofLineToJson();
    void ClearRoofLineJson();

    std::map<std::string, std::string> pipeMap = {{"Pipe Cube", "Cube"}, {"Pipe Vector Core0", "Vector"},
                                                  {"Pipe Vector Core1", "Vector1"}};
    Common::ChipType chipType_{};
    std::string opType_;
    int64_t freq_ = 0;
    int64_t aiCoreNum_ = 0;
    std::map<Event, uint64_t> pmuValues_ {};
    std::vector<Event> cubeCoreEvents_ {};
    std::vector<Event> vecCoreEvents_ {};
    SubCoreProperty cubeProperty_ {};
    SubCoreProperty vecProperty_ {};
    std::vector<nlohmann::json> roofLineJson_;
    std::map<std::string, float> maxBwRates_{};
    nlohmann::json visualRoofLineJson_;
protected:
    std::shared_ptr<OpBasicInfo> &opBasicInfoObj_;
    std::shared_ptr<BasicPmu> &basicPmuObj_;
    std::unique_ptr<Visualize::PmuCalculator> &pmuCalculatorObj_;
    Profiling::Calculate cal_;
private:
    static AnalysisPoint analysisPoint_;
    std::map<std::string, float> pipeLineRatio_;
};

class RoofLineOf910B : public RoofLine {
public:
    RoofLineOf910B(const int64_t aicoreFreq, const int64_t aiCoreNum, std::shared_ptr<OpBasicInfo> &opBasicInfoObj,
                   std::shared_ptr<BasicPmu> &basicPmuObj, std::unique_ptr<Visualize::PmuCalculator> &pmuCalculatorObj)
        : RoofLine(aicoreFreq, aiCoreNum, opBasicInfoObj, basicPmuObj, pmuCalculatorObj)
    {
        chipType_ = Common::ChipType::ASCEND910B;
        cubeCoreEvents_ = cubeEvents_;
        vecCoreEvents_ = vecEvents_;
        l2CacheEvict_ = basicPmuObj->GetL2cacheEvict();
    }
    std::vector<nlohmann::json> GenerateRoofLines() override;
private:
    void CubeMemoryUnit();
    void VecMemoryUnit();
    void CubePipeLine();
    void VectorPipeLine();
    void CubeMemoryPipe();
    void VectorMemoryPipe();
    void GmAndL2cache();
    void InsertPipeLineMaxBw(int64_t coreNum);
    void InsertMemPipeMaxBw(int64_t coreNum);
    void GetTheoryBwByGmType(std::map<std::string, float> &gmBw) const;
    void GetTheoryL2CacheByGmType(std::map<std::string, float> &l2CacheBw) const;

    const std::vector<Event> cubeEvents_ = {
        // fops
        Event::CUBE_FP_EXECUTED,
        Event::CUBE_INT_EXECUTED,
        Event::L0B_READ_REQ,
        // memory unit
        Event::MTE_TO_L1_REQ,
        Event::L1_TO_MTE_REQ,
        Event::FIXP_TO_L1_REQ,
        Event::MTE_TO_L0A_REQ,
        Event::MTE_TO_L0B_REQ,
        Event::L0C_TO_FIXP_REQ,
        // pipe line
        Event::MAIN_MEM_REQ,
        // gm/l2cache
        Event::WRITE_HIT,
        Event::WRITE_MISS,
        Event::READ_HIT_R0,
        Event::READ_MISS_R0,
        Event::READ_HIT_R1,
        Event::READ_MISS_R1,
        Event::WRITE_DATA,
        Event::READ_DATA_R0,
        Event::READ_DATA_R1,
    };
    const std::vector<Event> vecEvents_ = {
        // fops
        Event::VEC_FP32_EXECUTED,
        Event::VEC_FP16_EXECUTED_128,
        Event::VEC_FP16_EXECUTED_64,
        Event::VEC_FP16_EXECUTED_32,
        Event::VEC_S32_EXECUTED,
        Event::VEC_S16_EXECUTED,
        Event::VEC_MISC_EXECUTED,
        // memory unit
        Event::UB_TO_MTE_REQ,
        Event::MTE_TO_UB_REQ,
        Event::UB_TO_VEC_REQ,
        Event::VEC_TO_UB_REQ,
        // gm/l2cache
        Event::WRITE_HIT,
        Event::WRITE_MISS,
        Event::READ_HIT_R0,
        Event::READ_MISS_R0,
        Event::READ_HIT_R1,
        Event::READ_MISS_R1,
        Event::WRITE_DATA,
        Event::READ_DATA_R0,
        Event::READ_DATA_R1
    };
    // for memory unit theory bandwidth
    const std::map<std::string, uint16_t> memoryUnitBitsPerSecondCube_ = {
        {std::string(MemoryUnit::L1_TOTAL),      512},
        {std::string(MemoryUnit::WRITE_TO_L1),   256},
        {std::string(MemoryUnit::READ_FROM_L1),  256},
        {std::string(MemoryUnit::WRITE_TO_L0A),  256},
        {std::string(MemoryUnit::WRITE_TO_L0B),  128},
        {std::string(MemoryUnit::READ_FROM_L0C), 128}
    };
    // for memory unit theory bandwidth
    const std::map<std::string, uint16_t> memoryUnitBitsPerSecondVec_ = {
        {std::string(MemoryUnit::UB_TOTAL),        512},
        {std::string(MemoryUnit::READ_FROM_UB),    128},
        {std::string(MemoryUnit::WRITE_TO_UB),     128},
        {std::string(MemoryUnit::VECTOR_READ_UB),  512},
        {std::string(MemoryUnit::VECTOR_WRITE_UB), 512},
    };
    // for pipeline and compute pipe theory bandwidth
    std::map<std::string, uint64_t> basicPmu_ = {};
    int64_t cubeNum_ = 0;
    int64_t vecNum_ = 0;
    int64_t l2CacheEvict_ = -1;
};

class RoofLineOfA5 : public RoofLine {
public:
    RoofLineOfA5(const int64_t aicoreFreq, const int64_t aiCoreNum, std::shared_ptr<OpBasicInfo> &opBasicInfoObj,
        std::shared_ptr<BasicPmu> &basicPmuObj, std::unique_ptr<Visualize::PmuCalculator> &pmuCalculatorObj)
        : RoofLine(aicoreFreq, aiCoreNum, opBasicInfoObj, basicPmuObj, pmuCalculatorObj)
    {
        chipType_ = Common::ChipType::ASCEND950;
        cubeCoreEvents_ = cubeEvents_;
        vecCoreEvents_ = vecEvents_;
    }
    std::vector<nlohmann::json> GenerateRoofLines() override;
private:
    void CalCubeBaseData(int64_t cubeNum) override;
    void CalVecBaseData(int64_t vectorNum) override;
    void CubeMemoryUnit();
    void CubePipeLine();
    void CubeMemoryPipe();
    void InsertMaxBw();
    void VecMemoryUnit();
    void GmAndL2cache();

    const std::vector<Event> cubeEvents_ = {
        Event::READ_DATA_RECEIVED,
        Event::WRITE_DATA_SENT,
        Event::WR_L0A_INSTR,
        Event::WR_L0B_INSTR,
        Event::RD_L1_INSTR,
        Event::WR_L1_INSTR,
        Event::FIXP_WR_UB_INSTR,
        Event::FIXP_WR_L1_INSTR,
        Event::FIXP_WR_UB1_INSTR,

        // gm/l2cache
        Event::AR_CLOSE_L2_HIT_CORE,
        Event::AR_CLOSE_L2_MISS_CORE,
        Event::AR_CLOSE_L2_VICTIM_CORE,
        Event::AR_FAR_L2_HIT_CORE,
        Event::AR_FAR_L2_MISS_CORE,
        Event::AR_FAR_L2_VICTIM_CORE,
        Event::AW_CLOSE_L2_HIT_CORE,
        Event::AW_CLOSE_L2_MISS_CORE,
        Event::AW_CLOSE_L2_VICTIM_CORE,
        Event::AW_FAR_L2_HIT_CORE,
        Event::AW_FAR_L2_MISS_CORE,
        Event::AW_FAR_L2_VICTIM_CORE,
    };
    const std::vector<Event> vecEvents_ = {
        // gm/l2cache
        Event::AR_CLOSE_L2_HIT_CORE,
        Event::AR_CLOSE_L2_MISS_CORE,
        Event::AR_CLOSE_L2_VICTIM_CORE,
        Event::AR_FAR_L2_HIT_CORE,
        Event::AR_FAR_L2_MISS_CORE,
        Event::AR_FAR_L2_VICTIM_CORE,
        Event::AW_CLOSE_L2_HIT_CORE,
        Event::AW_CLOSE_L2_MISS_CORE,
        Event::AW_CLOSE_L2_VICTIM_CORE,
        Event::AW_FAR_L2_HIT_CORE,
        Event::AW_FAR_L2_MISS_CORE,
        Event::AW_FAR_L2_VICTIM_CORE,
        Event::DCU_MISS_LDG,
        Event::DCU_MISS_LDK,
        Event::DCU_REQ_STG,
        Event::DCU_REQ_STK,
    };
    // for memory unit theory bandwidth
    const std::map<std::string, uint16_t> memoryUnitBitsPerSecondCube_ = {
        {std::string(MemoryUnit::L1_TOTAL),      512},
        {std::string(MemoryUnit::WRITE_TO_L1),   256},
        {std::string(MemoryUnit::READ_FROM_L1),  256},
        {std::string(MemoryUnit::READ_FROM_L0C), 512}
    };
    // for memory unit theory bandwidth
    const std::map<std::string, uint16_t> memoryUnitBitsPerSecondVec_ = {
        {std::string(MemoryUnit::SIMT_VF), 128},
    };
    // for pipeline and compute pipe theory bandwidth
    std::map<std::string, uint64_t> basicPmu_ = {};
    int64_t cubeNum_ = 0;
    int64_t vecNum_ = 0;
    std::vector<SubCoreProperty> cubeProperties_ {};
    std::vector<SubCoreProperty> vecSimtProperties_ {};
};

class RoofLineOf310P : public RoofLine {
public:
    RoofLineOf310P(const int64_t aicoreFreq, const int64_t aiCoreNum, std::shared_ptr<OpBasicInfo> &opBasicInfoObj,
                   std::shared_ptr<BasicPmu> &basicPmuObj, std::unique_ptr<Visualize::PmuCalculator> &pmuCalculatorObj)
        : RoofLine(aicoreFreq, aiCoreNum, opBasicInfoObj, basicPmuObj, pmuCalculatorObj)
    {
        chipType_ = Common::ChipType::ASCEND310P;
        cubeCoreEvents_ = cubeEvents_;
    }
    std::vector<nlohmann::json> GenerateRoofLines() override;
private:
    void MemoryUnit();
    static constexpr char const *MEMORY_UNIT = "Memory Unit";
    const std::vector<Event> cubeEvents_ = {
        // fops
        Event::CUBE_FP_EXECUTED,
        Event::CUBE_INT_EXECUTED,
        Event::VEC_FP32_EXECUTED,
        Event::VEC_FP16_EXECUTED_128,
        Event::VEC_FP16_EXECUTED_64,
        Event::VEC_FP16_EXECUTED_32,
        Event::VEC_S32_EXECUTED,
        Event::VEC_S16_EXECUTED,
        Event::VEC_MISC_EXECUTED,
        Event::L0B_READ_REQ,
        // memory unit
        Event::MTE_TO_L1_REQ,
        Event::L1_TO_MTE_REQ,
        Event::MTE_TO_L0A_REQ,
        Event::MTE_TO_L0B_REQ,
        Event::LOC_TO_VEC_REQ,
        Event::UB_TO_MTE_REQ,
        Event::MTE_TO_UB_REQ,
        Event::UB_TO_VEC_REQ,
        Event::VEC_TO_UB_REQ,
    };
    const std::map<std::string, uint16_t> memoryUnitBitsPerSecond_ = {
        {std::string(MemoryUnit::L1_TOTAL),        768},
        {std::string(MemoryUnit::WRITE_TO_L1),     256},
        {std::string(MemoryUnit::READ_FROM_L1),    128},
        {std::string(MemoryUnit::WRITE_TO_L0A),    256},
        {std::string(MemoryUnit::WRITE_TO_L0B),    128},
        {std::string(MemoryUnit::READ_FROM_L0C),   512},
        {std::string(MemoryUnit::UB_TOTAL),        256},
        {std::string(MemoryUnit::READ_FROM_UB),    128},
        {std::string(MemoryUnit::WRITE_TO_UB),     128},
        {std::string(MemoryUnit::VECTOR_READ_UB),  512},
        {std::string(MemoryUnit::VECTOR_WRITE_UB), 512},
    };
};
}
#endif // MSOPT_ROOFLINE_H
