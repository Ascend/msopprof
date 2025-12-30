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

#ifndef __MSOPPROF_PROFILING_PMU_CALCULATE_H__
#define __MSOPPROF_PROFILING_PMU_CALCULATE_H__
#include <map>
#include <functional>
#include <utility>
#include <vector>
#include <string>
#include <set>
#include <iostream>
#include <cmath>

#include "common/defs.h"
#include "ustring.h"
#include "device_defs.h"
namespace Profiling {
constexpr uint32_t TIME_CONVERSION = 1000;
constexpr uint32_t BIT_CONVERSION = 1024;
constexpr float THROUGHPUT_CONVERSION =
    static_cast<float>(TIME_CONVERSION) * TIME_CONVERSION / BIT_CONVERSION / BIT_CONVERSION;
constexpr uint32_t PERCENTAGE_CONVERSION = 100;
constexpr uint32_t CUBE_FOPS_FP16_NUM = 8192;
constexpr uint32_t CUBE_FOPS_INT8_NUM = 16384;
constexpr uint64_t BANDWIDTH_NUM = 8589934592;

struct VecFopsPmus {
    uint64_t fp32Pmu;
    uint64_t fp16Lane128Pmu;
    uint64_t fp16Lane64Pmu;
    uint64_t fp16Lane32Pmu;
    uint64_t int32Pmu;
    uint64_t int16Pmu;
    uint64_t miscPmu;
};

struct VecFopsNums {
    uint16_t fp32Num;
    uint16_t fp16Lane128Num;
    uint16_t fp16Lane64Num;
    uint16_t fp16Lane32Num;
    uint16_t int32Num;
    uint16_t int16Num;
    uint16_t miscNum;
};

struct L2CachePmus {
    uint64_t whiteHit;
    uint64_t whiteMiss;
    uint64_t whiteMissNotAlloc;
    uint64_t readR0Hit;
    uint64_t readR0Miss;
    uint64_t readR0MissNotAlloc;
    uint64_t readR1Hit;
    uint64_t readR1Miss;
    uint64_t readR1MissNotAlloc;
};

struct Mte1BwPmusWithSize {
    uint64_t l0aSize;
    uint64_t l0aPipe;
    uint64_t l0bSize;
    uint64_t l0bPipe;
    uint64_t gmToL0a;
    uint64_t gmToL0b;
    uint64_t l0aPmu;
    uint64_t l0bPmu;
    uint64_t mte1CyclePmu;
};

class PmuMap {
public:
    explicit PmuMap(std::map<uint16_t, uint64_t> &pmuMap) : pmuMap_(pmuMap) {}
    uint64_t At(const uint16_t key) const
    {
        auto it = pmuMap_.find(key);
        return (it != pmuMap_.end()) ? it->second : defaultValue_;
    }
private:
    uint64_t defaultValue_ = 0;
    std::map<uint16_t, uint64_t> pmuMap_;
};

struct CalculateParams {
    uint64_t totalCycles;
    uint64_t frequency;
    float duration;
    std::string socVersion;
    PmuMap pmuMap;
};

const std::map<Common::ChipType, VecFopsNums> VEC_FOPS_NUMS = {
    {Common::ChipType::ASCEND910B, {64, 128, 64, 32, 64, 128, 32}},
    {Common::ChipType::ASCEND310P, {64, 128, 64, 32, 16, 32, 16}},
};

struct CalDeviceInfo {
    Common::ChipType chipType;
    int64_t freq;
    int64_t aiCoreNum;
    uint64_t blockDim;
    std::string socVersion;
};
class Calculate {
public:
    Calculate(const std::map<uint16_t, uint64_t> &pmuEventValueMap, uint64_t totalCycles,
        const CalDeviceInfo &deviceInfo, float duration = 0.0f) : pmuEventValueMap_(pmuEventValueMap),
        totalCycles_(totalCycles), freq_(deviceInfo.freq), socVersion_(deviceInfo.socVersion), duration_(duration)
    {
        auto it = VEC_FOPS_NUMS.find(deviceInfo.chipType);
        if (it == VEC_FOPS_NUMS.end()) {
            vecFopsNums_ = VEC_FOPS_NUMS.at(Common::ChipType::ASCEND910B);
        } else {
            vecFopsNums_ = VEC_FOPS_NUMS.at(deviceInfo.chipType);
        }
        if (deviceInfo.chipType == Common::ChipType::ASCEND310P) {
            if ((deviceInfo.aiCoreNum != 0) && (deviceInfo.blockDim != 0)) {
                auto blockDim = static_cast<float>(deviceInfo.blockDim);
                auto aiCoreNum = static_cast<float>(deviceInfo.aiCoreNum);
                timeFactor_ = static_cast<float>(std::floor((blockDim + aiCoreNum - 1) / aiCoreNum)) / blockDim;
            }
        }
    }
    ~Calculate() = default;

    uint64_t GetPmuEventValue(const uint64_t &eventId) const;
    std::string CalRatio(const uint64_t &pmu) const;
    std::string CalSumRatio(const std::vector<uint64_t> &pmus) const;
    std::string CalCubeRatio(const uint64_t &pmu1, const uint64_t &pmu2, const uint64_t &pmu3) const;
    std::string CalVecRatio(const std::vector<uint64_t> &pmus) const;
    std::string CalPmuSelf(const uint64_t &pmu) const;
    std::string CalBandwidth(const uint64_t &pipSize, const uint64_t &scalar, const uint64_t &pmu) const;
    float CalBandwidthFp(const uint64_t &pipSize, const uint64_t &scalar, const uint64_t &pmu) const;
    float CalBwUsageReqFp(const uint64_t &pmu, const uint64_t &bytes) const;
    std::string CalPipeTime(const uint64_t &pmu) const;
    std::string CalTotalCycles() const;
    std::string CalTotalTime() const;
    float CalPmuDivFp(const uint64_t &pmu1, const uint64_t &pmu2) const;
    std::string CalPmuDiv(const uint64_t &pmu1, const uint64_t &pmu2) const;
    std::string CalReadMainMemoryBandwidth(const uint64_t &pipSize, const uint64_t &scalar, const uint64_t &pmu1,
                                           const uint64_t &pmu2) const;
    std::string CalVecFp32Ratio(const uint64_t &pmu1, const uint64_t &pmu2, const uint64_t &pmu3,
                                const uint64_t &pmu4) const;
    std::string CalVecFops(const VecFopsPmus &pmus) const;
    uint64_t CalVecFopsInt(const VecFopsPmus &pmus) const;
    std::string CalCubeFops(const uint64_t &pmu1, const uint64_t &pmu2) const;
    uint64_t CalCubeFopsInt(const uint64_t &pmu1, const uint64_t &pmu2) const;
    float CalTotalHitRateFp(const L2CachePmus &pmus) const;
    std::string CalTotalHitRate(const L2CachePmus &pmus) const;
    std::string CalWriteHitRate(const uint64_t &pmu1, const uint64_t &pmu2, const uint64_t &pmu3) const;
    std::string CalReadHitRate(const L2CachePmus &l2CachePmus) const;
    std::string CalReadMainMemoryDatas(const uint64_t &pmu1, const uint64_t &pmu2) const;
    std::string CalWriteMainMemoryDatas(const uint64_t &pmu1) const;
    std::string CalGmToL1Datas(const uint64_t &pmu1, const uint64_t &pmu2) const;
    std::string CalL1ToGmDatas(const uint64_t &pmu1, const uint64_t &pmu2, const uint64_t &pmu3) const;
    std::string CalL0cToL1Datas(const uint64_t &pmu1) const;
    std::string CalL0cToGmDatas(const uint64_t &pmu1, const uint64_t &pmu2) const;
    std::string CalGmToUbDatas(const uint64_t &pmu1) const;
    std::string CalUbToGmDatas(const uint64_t &pmu1) const;
    std::string CalGmToL1BwUsageRate(const uint64_t &pmu1, const uint64_t &pmu2) const;
    std::string CalL1ToGmBwUsageRate(const uint64_t &pmu1, const uint64_t &pmu2, const uint64_t &pmu3) const;
    std::string CalL0cToL1BwUsageRate(const uint64_t &pmu1) const;
    std::string CalL0cToGmBwUsageRate(const uint64_t &pmu1, const uint64_t &pmu2) const;
    std::string CalGmToUbBwUsageRate(const uint64_t &pmu1) const;
    std::string CalUbToGmBwUsageRate(const uint64_t &pmu1) const;
    std::string CalL2CacheHitRate(const uint64_t &pmu1, const uint64_t &pmu2, const uint64_t &pmu3) const;
    std::string CalTransportBwUsageRate(const uint64_t &pmu, const uint64_t type) const;
    std::map<std::string, std::map<TransportType, float>> GetMaxBwRateByGmType() const;
    std::string CalAicMte1ActivateBw(const Mte1BwPmusWithSize &mte1) const;
    std::string CalAicMte2ActivateBw(const uint64_t &gmToL1, const uint64_t &gmToL0a,
                                     const uint64_t &gmToL0b, const uint64_t &pmu) const;
    std::string CalAicMte3ActivateBw(const uint64_t &pipSize, const uint64_t &pmu1, const uint64_t &pmu2,
                                     const uint64_t &pmu3, const uint64_t &pmu4) const;
    std::string CalAivMteActivateBw(const uint64_t &pipSize, const uint64_t &pmu1, const uint64_t &pmu2) const;

    std::map<uint16_t, uint64_t> pmuEventValueMap_;
    uint64_t totalCycles_;
    int64_t freq_;
    std::string socVersion_;
    VecFopsNums vecFopsNums_{};
    float timeFactor_ = 1.0;
    float duration_;

private:
    static std::string CalTransportDatas(const uint64_t &pmu, const TransportType &type) ;
};

enum class FuncType : uint16_t {
    RATIO = 0,
    SUM_RATIO,
    CUBE_RATIO,
    VEC_RATIO,
    PMU_SELF,
    BANDWIDTH,
    PIPE_TIME,
    TOTAL_CYCLES,
    TOTAL_TIME,
    PMU_DIV,
    READ_MAIN_MEMORY_BANDWIDTH,
    VEC_FP32_RATIO,
    VEC_FOPS,
    CUBE_FOPS,
    VEC_FOPS_INT,
    CUBE_FOPS_INT,
    TOTAL_HIT_RATE,
    WRITE_HIT_RATE,
    READ_HIT_RATE,
    WRITE_MAIN_MEMORY_DATAS,
    READ_MAIN_MEMORY_DATAS,
    GM_TO_L1_DATAS,
    L1_TO_GM_DATAS,
    L0C_TO_L1_DATAS,
    L0C_TO_GM_DATAS,
    GM_TO_UB_DATAS,
    UB_TO_GM_DATAS,
    GM_TO_L1_BW_USAGE_RATE,
    L1_TO_GM_BW_USAGE_RATE,
    L0C_TO_L1_BW_USAGE_RATE,
    L0C_TO_GM_BW_USAGE_RATE,
    GM_TO_UB_BW_USAGE_RATE,
    UB_TO_GM_BW_USAGE_RATE,
    L2_CACHE_HIT_RATE,
    BW_USAGE_REQ_FP,
    BANDWIDTH_FP,
    TOTAL_HIT_RATE_FP,
    TRANSPORT_BW_USAGE_RATE,
    AIC_MTE1_ACTIVATE_BANDWIDTH,
    AIC_MTE2_ACTIVATE_BANDWIDTH,
    AIC_MTE3_ACTIVATE_BANDWIDTH,
    AIV_MTE_ACTIVATE_BANDWIDTH,
    UNKNOWN
};

struct CalInfo {
    std::vector<uint64_t> pmuEvents;
    std::vector<uint64_t> params;
    FuncType funcType;
};

using FuncInfo = std::function<std::string(const Calculate &, const std::vector<uint64_t> &)>;
using FuncInfoFp = std::function<float(const Calculate &, const std::vector<uint64_t> &)>;
using FuncInfoInt = std::function<uint64_t(const Calculate &, const std::vector<uint64_t> &)>;
const std::map<FuncType, FuncInfoFp> FunctionsFp = {
    {FuncType::BW_USAGE_REQ_FP, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalBwUsageReqFp(args[0], args[1]);
    }},
    {FuncType::BANDWIDTH_FP, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalBandwidthFp(args[0], args[1], args[2]);
    }},
    {FuncType::TOTAL_HIT_RATE_FP, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalTotalHitRateFp({args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]});
    }},
};
const std::map<FuncType, FuncInfoInt> FunctionsInt = {
    {FuncType::VEC_FOPS_INT, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalVecFopsInt({args[0], args[1], args[2], args[3], args[4], args[5], args[6]});
    }},
    {FuncType::CUBE_FOPS_INT, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalCubeFopsInt(args[0], args[1]);
    }},
};
const std::map<FuncType, FuncInfo> Functions = {
    {FuncType::RATIO, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalRatio(args[0]);
    }},
    {FuncType::SUM_RATIO, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalSumRatio(args);
    }},
    {FuncType::CUBE_RATIO, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalCubeRatio(args[0], args[1], args[2]);
    }},
    {FuncType::VEC_RATIO, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalVecRatio(args);
    }},
    {FuncType::PMU_SELF, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalPmuSelf(args[0]);
    }},
    {FuncType::BANDWIDTH, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalBandwidth(args[0], args[1], args[2]);
    }},
    {FuncType::PIPE_TIME, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalPipeTime(args[0]);
    }},
    {FuncType::TOTAL_CYCLES, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        (void)args;
        return cal.CalTotalCycles();
    }},
    {FuncType::TOTAL_TIME, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        (void)args;
        return cal.CalTotalTime();
    }},
    {FuncType::PMU_DIV, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalPmuDiv(args[0], args[1]);
    }},
    {FuncType::READ_MAIN_MEMORY_BANDWIDTH, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalReadMainMemoryBandwidth(args[0], args[1], args[2], args[3]);
    }},
    {FuncType::VEC_FP32_RATIO, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalVecFp32Ratio(args[0], args[1], args[2], args[3]);
    }},
    {FuncType::VEC_FOPS, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalVecFops({args[0], args[1], args[2], args[3], args[4], args[5], args[6]});
    }},
    {FuncType::CUBE_FOPS, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalCubeFops(args[0], args[1]);
    }},
    {FuncType::TOTAL_HIT_RATE, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalTotalHitRate({args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]});
    }},
    {FuncType::WRITE_HIT_RATE, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalWriteHitRate(args[0], args[1], args[2]);
    }},
    {FuncType::READ_HIT_RATE, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalReadHitRate({0, 0, 0, args[0], args[1], args[2], args[3], args[4], args[5]});
    }},
    {FuncType::WRITE_MAIN_MEMORY_DATAS, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalWriteMainMemoryDatas(args[0]);
    }},
    {FuncType::READ_MAIN_MEMORY_DATAS, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalReadMainMemoryDatas(args[0], args[1]);
    }},
    {FuncType::GM_TO_L1_DATAS, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalGmToL1Datas(args[0], args[1]);
    }},
    {FuncType::L1_TO_GM_DATAS, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalL1ToGmDatas(args[0], args[1], args[2]);
    }},
    {FuncType::L0C_TO_L1_DATAS, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalL0cToL1Datas(args[0]);
    }},
    {FuncType::L0C_TO_GM_DATAS, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalL0cToGmDatas(args[0], args[1]);
    }},
    {FuncType::GM_TO_UB_DATAS, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalGmToUbDatas(args[0]);
    }},
    {FuncType::UB_TO_GM_DATAS, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalUbToGmDatas(args[0]);
    }},
    {FuncType::GM_TO_L1_BW_USAGE_RATE, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalGmToL1BwUsageRate(args[0], args[1]);
    }},
    {FuncType::L1_TO_GM_BW_USAGE_RATE, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalL1ToGmBwUsageRate(args[0], args[1], args[2]);
    }},
    {FuncType::L0C_TO_L1_BW_USAGE_RATE, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalL0cToL1BwUsageRate(args[0]);
    }},
    {FuncType::L0C_TO_GM_BW_USAGE_RATE, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalL0cToGmBwUsageRate(args[0], args[1]);
    }},
    {FuncType::GM_TO_UB_BW_USAGE_RATE, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalGmToUbBwUsageRate(args[0]);
    }},
    {FuncType::UB_TO_GM_BW_USAGE_RATE, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalUbToGmBwUsageRate(args[0]);
    }},
    {FuncType::L2_CACHE_HIT_RATE, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalL2CacheHitRate(args[0], args[1], args[2]);
    }},
    {FuncType::TRANSPORT_BW_USAGE_RATE, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalTransportBwUsageRate(args[0], args[1]);
    }},
    {FuncType::AIC_MTE1_ACTIVATE_BANDWIDTH, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalAicMte1ActivateBw({args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]});
    }},
    {FuncType::AIC_MTE2_ACTIVATE_BANDWIDTH, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalAicMte2ActivateBw(args[0], args[1], args[2], args[3]);
    }},
    {FuncType::AIC_MTE3_ACTIVATE_BANDWIDTH, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalAicMte3ActivateBw(args[0], args[1], args[2], args[3], args[4]);
    }},
    {FuncType::AIV_MTE_ACTIVATE_BANDWIDTH, [](const Calculate &cal, const std::vector<uint64_t> &args) {
        return cal.CalAivMteActivateBw(args[0], args[1], args[2]);
    }}
};

uint64_t GetPmuValue(const std::map<uint16_t, uint64_t> &pmuEventValueMap, const uint64_t &eventId);

std::map<std::string, std::string> CalMetricItems(const Calculate &cal, const std::set<std::string> &metricItems,
    const std::map<std::string, CalInfo> &formula, std::map<std::string, uint64_t> &dbiMap);

std::map<std::string, std::string> CalMetricItems(
    const CalculateParams &params, const std::set<std::string> &metricItems,
    const std::map<std::string, std::function<std::string(const CalculateParams &params)>> &formula);
}
#endif // __MSOPPROF_PROFILING_PMU_CALCULATE_H__
