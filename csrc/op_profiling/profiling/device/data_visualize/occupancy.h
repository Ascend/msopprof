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


#ifndef OCCUPANCY_H
#define OCCUPANCY_H

#include "filesystem.h"
#include "basic_op_and_pmu.h"
#include "pmu_calculator.h"
#include "common/defs.h"
#include "number_operation.h"

namespace Visualize {
// pmu for A2/A3
constexpr uint64_t THROUGHPUT_DATA_GRANULARITY = 128;  // 读写数据量单位，一个req代表 128 Byte
constexpr uint64_t EVENT_WRITE_CACHE_HIT = 1280;
constexpr uint64_t EVENT_WRITE_CACHE_MISS = 1282;
constexpr uint64_t EVENT_WRITE_CACHE_MISS_NO_ALLOCATE = 1283;
constexpr uint64_t EVENT_READ_R0_CACHE_HIT = 1284;
constexpr uint64_t EVENT_READ_R0_CACHE_MISS = 1286;
constexpr uint64_t EVENT_READ_R0_CACHE_MISS_NO_ALLOCATE = 1287;
constexpr uint64_t EVENT_READ_R1_CACHE_HIT = 1288;
constexpr uint64_t EVENT_READ_R1_CACHE_MISS = 1290;
constexpr uint64_t EVENT_READ_R1_CACHE_MISS_NO_ALLOCATE = 1291;
constexpr uint64_t EVENT_GM_MAIN_WRITE = 1292;
constexpr uint64_t EVENT_GM_MAIN_READ_R0 = 1293;
constexpr uint64_t EVENT_GM_MAIN_READ_R1 = 1294;
// pmu for A5
constexpr uint64_t EVENT_GM_MAIN_READ_A5 = 1058;
constexpr uint64_t EVENT_GM_MAIN_WRITE_A5 = 1059;
constexpr uint64_t EVENT_READ_CLOSE_HIT = 1060;
constexpr uint64_t EVENT_READ_CLOSE_MISS = 1061;
constexpr uint64_t EVENT_READ_CLOSE_VICTIM = 1062;
constexpr uint64_t EVENT_READ_FAR_HIT = 1063;
constexpr uint64_t EVENT_READ_FAR_MISS = 1064;
constexpr uint64_t EVENT_READ_FAR_VICTIM = 1065;
constexpr uint64_t EVENT_WRITE_CLOSE_HIT = 1066;
constexpr uint64_t EVENT_WRITE_CLOSE_MISS = 1067;
constexpr uint64_t EVENT_WRITE_CLOSE_VICTIM = 1068;
constexpr uint64_t EVENT_WRITE_FAR_HIT = 1069;
constexpr uint64_t EVENT_WRITE_FAR_MISS = 1070;
constexpr uint64_t EVENT_WRITE_FAR_VICTIM = 1071;
constexpr uint64_t PMU_IDC_AIV_VEC_INSTR_SIMT_VF_BUSY_O = 1284;
constexpr uint64_t WSU_PMU_EXU_INS_ISSUE = 1466;
constexpr uint64_t WSU_PMU_DVG_INS_ISSUE = 1467;
constexpr uint64_t WSU_PMU_DCU_INS_ISSUE = 1468;

constexpr float OCCUPANCY_BALANCE_THRESHOLD = 0.9;

enum class OccupancyDataType {
    OCPY_CYCLES = 0,
    OCPY_THROUGHPUT,
    OCPY_CACHE_HIT_RATE,
    OCPY_SIMT_INSTR,
    OCPY_MAX,
};

struct SimtComputeLoadMetrics {
    uint64_t cycles;
    uint64_t instrNum;
};

struct OccupancyMetrics {
    uint64_t cycles;
    uint64_t throughput;
    float cacheHitRate;
    bool hasSimtMetrics;
    SimtComputeLoadMetrics simtMetrics;
};

class Occupancy {
public:
    Occupancy(std::shared_ptr<OpBasicInfo> &opBasicInfoObj, std::shared_ptr<BasicPmu> &basicPmuObj)
        : opBasicInfoObj_(opBasicInfoObj), basicPmuObj_(basicPmuObj)
        {
            SetBlockIDCoreIDPairVec();
            // 例如 0vector1 表示0核上的aiv1
            pattern_ = std::regex(("([0-9]{1,2})([a-z]{4,6})([0-1]{1})"));
        }
    bool GetOccupancyMap(nlohmann::json &occupancyMapJson);
    void ClearOccupancyJson();
protected:
    virtual OccupancyMetrics GetSubBlockData(const std::map<uint64_t, uint64_t> &eventMap, uint64_t totalCycles) { return OccupancyMetrics{}; };
    // 存放每个block的三种指标
    std::map<std::string, OccupancyMetrics> metricsMap_;
    std::shared_ptr<OpBasicInfo> &opBasicInfoObj_;
    std::shared_ptr<BasicPmu> &basicPmuObj_;
private:
    void SetBlockIDCoreIDPairVec() { blockIdCoreIdPairVec_ = std::move(basicPmuObj_->GetBlockIdCoreIdPairVec()); };
    std::vector<nlohmann::json> GetOccupancyJson();
    std::vector<nlohmann::json> GetOccupancyBlockJson(MemMapDetail &memMapDetail);
    void MergeSameCorePmu(std::vector<MemMapDetail> &pmuMapDetails);
    std::map<uint64_t, uint64_t> MergeEventMap(std::map<uint64_t, uint64_t> &eventMap1,
                                               std::map<uint64_t, uint64_t> &eventMap2) const;
    void AnalyzeOccupy(Common::PairVector<std::string, double> &Ocpy, OccupancyDataType OcpyType,
                       std::vector<std::string> &adviceList) const;
    bool NormalizeOccupy(Common::PairVector<std::string, double> &Ocpy, OccupancyDataType OcpyType) const;
    std::string GetAdviceFromOcpyData(const Common::PairVector<Common::RefOf<Common::PairVector<std::string, double>>, OccupancyDataType> &ocpyData) const;
    std::string GetAdvice() const;

    std::string opType_;
    // 融合的pmu event，纯vector或纯cube算子，把相同物理核的pmu相加，blockID使用最小的
    std::vector<MemMapDetail> fusionPmuMapDetails_;
    Common::PairVector<uint16_t, uint16_t> blockIdCoreIdPairVec_;
    std::regex pattern_;
    static const std::unordered_map<OccupancyDataType, double> ReportThresholds;
};

class Occupancy910B : public Occupancy {
public:
    Occupancy910B(std::shared_ptr<OpBasicInfo> &opBasicInfoObj, std::shared_ptr<BasicPmu> &basicPmuObj)
        : Occupancy(opBasicInfoObj, basicPmuObj) {}
private:
    OccupancyMetrics GetSubBlockData(const std::map<uint64_t, uint64_t> &eventMap, uint64_t totalCycles) override;
    bool CheckCacheHitEventMap(const std::map<uint64_t, uint64_t> &pmuEvents) const;
};

class OccupancyA5 : public Occupancy {
public:
    OccupancyA5(std::shared_ptr<OpBasicInfo> &opBasicInfoObj, std::shared_ptr<BasicPmu> &basicPmuObj)
        : Occupancy(opBasicInfoObj, basicPmuObj) {}
private:
    OccupancyMetrics GetSubBlockData(const std::map<uint64_t, uint64_t> &eventMap, uint64_t totalCycles) override;
};
}
#endif // DATA_VISUALIZE_CONST_H
