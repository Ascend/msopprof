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

#include "pmu_calculate.h"
#include "log.h"
#include "number_operation.h"
#include "equation_utils.h"
#include "metric_csv_header.h"

using namespace std;
using namespace Utility;
using namespace Common;
namespace Profiling {

std::map<std::string, std::vector<std::string>> DbiPipeMap {
    {"aic_mte1_active_bw(GB/s)", {GM_TO_L0A_DATA, GM_TO_L0B_DATA}},
    {"aic_mte2_active_bw(GB/s)", {GM_TO_L0A_DATA, GM_TO_L0B_DATA, GM_TO_L1_DATA}}
};

uint64_t Calculate::GetPmuEventValue(const uint64_t &eventId) const
{
    return GetPmuValue(pmuEventValueMap_, eventId);
}

string Calculate::CalRatio(const uint64_t &pmu) const
{
    uint64_t calCycles = totalCycles_;
    // only mix op and without kernel scale
    if (!IsZero(duration_) && !IsZero(timeFactor_)) {
        std::string location = "ratio";
        calCycles = static_cast<uint64_t>(SafeMul(static_cast<uint64_t>(duration_),
            static_cast<uint64_t>(freq_), location) / timeFactor_);
    }
    return CalPmuDiv(pmu, calCycles);
}

string Calculate::CalSumRatio(const vector<uint64_t> &pmus) const
{
    std::string location = "sum ratio";
    return Calculate::CalRatio(SafeAddAll(pmus, location));
}

string Calculate::CalCubeRatio(const uint64_t &pmu1, const uint64_t &pmu2, const uint64_t &pmu3) const
{
    // because pmu data not accurate, eg: use pmu(73)*pmu(10)/(pmu(73)+pmu(74)) to cal cube fp ratio
    uint64_t scaleCycle = 0;
    std::string location = "cube ratio";
    uint64_t temp = SafeAdd(pmu1, pmu2, location);
    if (temp != 0) {
        scaleCycle = static_cast<uint64_t>(SafeMul(pmu1, pmu3, location) / temp);
    }
    return CalRatio(scaleCycle);
}

string Calculate::CalVecRatio(const vector<uint64_t> &pmus) const
{
    uint64_t scaleCycle = 0;
    // 该公式至少需要 3 个pmu，pmus[0]为pmu(8), pmus[1]为pmu(1)
    if (pmus.size() >= 3) {
        std::string location = "vec ratio";
        uint64_t temp = SafeSub(SafeAddAll(pmus, location), SafeAdd(pmus[0], pmus[1], location), location);
        if (pmus[1] != 0) {
            scaleCycle = static_cast<uint64_t>(SafeMul(pmus[0], temp, location) / pmus[1]);
        }
    }
    return CalRatio(scaleCycle);
}

string Calculate::CalPmuSelf(const uint64_t &pmu) const
{
    return to_string(pmu);
}

string Calculate::CalBandwidth(const uint64_t &pipSize, const uint64_t &scalar, const uint64_t &pmu) const
{
    return to_string(Calculate::CalBandwidthFp(pipSize, scalar, pmu));
}

float Calculate::CalBandwidthFp(const uint64_t &pipSize, const uint64_t &scalar, const uint64_t &pmu) const
{
    float time = 0.0f;
    if (!IsZero(duration_)) {
        // only mix op and without kernel scale
        time = duration_;
    } else {
        if (freq_ != 0) {
            time = static_cast<float>(totalCycles_) / freq_ * timeFactor_;
        }
    }
    if (IsZero(time)) {
        return 0.0f;
    }
    vector<float> bandWithFp {static_cast<float>(pmu), static_cast<float>(pipSize), static_cast<float>(scalar),
        static_cast<float>(TIME_CONVERSION), static_cast<float>(TIME_CONVERSION)};
    std::string location = "band with Fp";
    return SafeMulAll(bandWithFp, location) / time / BANDWIDTH_NUM;
}

float Calculate::CalBwUsageReqFp(const uint64_t &pmu, const uint64_t &bytes) const
{
    float res;
    // 溢出判断
    if ((pmu > 0) && (bytes > UINT64_MAX / pmu)) {
        return static_cast<float>(pmu);
    }
    // 如果数值不溢出，表明pmu数据正常，无法导致溢出
    std::string location = "band with Usage";
    res = SafeMul(static_cast<float>(pmu), static_cast<float>(bytes), location) / BIT_CONVERSION;

    float time = 0.0f;
    if (!IsZero(duration_)) {
        // only mix op and without kernel scale
        time = duration_;
    } else if (freq_ != 0) {
        time = static_cast<float>(totalCycles_) / freq_ * timeFactor_;
    }
    if (IsZero(time)) {
        return 0.0f;
    }

    return res * THROUGHPUT_CONVERSION / time;
}

string Calculate::CalPipeTime(const uint64_t &pmu) const
{
    if (freq_ == 0 || IsZero(timeFactor_)) {
        return "0";
    }
    return to_string(static_cast<float>(pmu) / freq_ * timeFactor_);
}

string Calculate::CalTotalCycles() const
{
    return CalPmuSelf(totalCycles_);
}

string Calculate::CalTotalTime() const
{
    return CalPipeTime({totalCycles_});
}

float Calculate::CalPmuDivFp(const uint64_t &pmu1, const uint64_t &pmu2) const
{
    if (pmu2 == 0) {
        return 0;
    }
    return static_cast<float>(pmu1) / pmu2;
}

string Calculate::CalPmuDiv(const uint64_t &pmu1, const uint64_t &pmu2) const
{
    return to_string(CalPmuDivFp(pmu1, pmu2));
}

string Calculate::CalReadMainMemoryBandwidth(const uint64_t &pipSize, const uint64_t &scalar, const uint64_t &pmu1,
                                             const uint64_t &pmu2) const
{
    std::string location = "read main memory bandwidth";
    return CalBandwidth(pipSize, scalar, SafeAdd(pmu1, pmu2, location));
}

string Calculate::CalVecFp32Ratio(const uint64_t &pmu1, const uint64_t &pmu2, const uint64_t &pmu3,
                                  const uint64_t &pmu4) const
{
    std::string location = "fp32 ratio";
    if (IsZero(pmu2)) {
        return "0";
    }
    return Calculate::CalRatio(SafeMul(SafeSub(pmu3, pmu4, location), pmu1, location) / pmu2);
}

uint64_t Calculate::CalVecFopsInt(const VecFopsPmus &pmus) const
{
    vector<uint64_t> pmusValues{pmus.fp32Pmu, pmus.fp16Lane128Pmu, pmus.fp16Lane64Pmu,
        pmus.fp16Lane32Pmu, pmus.int32Pmu, pmus.int16Pmu, pmus.miscPmu};
    vector<uint16_t> vecFopsValues{vecFopsNums_.fp32Num, vecFopsNums_.fp16Lane128Num, vecFopsNums_.fp16Lane64Num,
        vecFopsNums_.fp16Lane32Num, vecFopsNums_.int32Num, vecFopsNums_.int16Num, vecFopsNums_.miscNum};
    std::string location = "vector of fops";
    return SafeMulAddAll(pmusValues, vecFopsValues, location);
}

string Calculate::CalVecFops(const VecFopsPmus &pmus) const
{
    VecFopsPmus calPmus = pmus;
    // fp32Pmu-fp16Lane128Pmu only for 910B davinci bug
    std::string location = "fp32 Pmu";
    calPmus.fp32Pmu = StartsWith(socVersion_, "Ascend910B") ?
        SafeSub(pmus.fp32Pmu, pmus.fp16Lane128Pmu, location) : pmus.fp32Pmu;
    return to_string(CalVecFopsInt(calPmus));
}

uint64_t Calculate::CalCubeFopsInt(const uint64_t &pmu1, const uint64_t &pmu2) const
{
    vector<uint64_t> pmuValues{pmu1, pmu2};
    vector<uint16_t> cubeFopsValues{CUBE_FOPS_FP16_NUM, CUBE_FOPS_INT8_NUM};
    std::string location = "cube fops";
    return SafeMulAddAll(pmuValues, cubeFopsValues, location);
}

string Calculate::CalCubeFops(const uint64_t &pmu1, const uint64_t &pmu2) const
{
    return to_string(CalCubeFopsInt(pmu1, pmu2));
}

float Calculate::CalTotalHitRateFp(const L2CachePmus &pmus) const
{
    vector<uint64_t> nums1{pmus.whiteHit, pmus.readR0Hit, pmus.readR1Hit};
    vector<uint64_t> nums2{pmus.whiteHit, pmus.readR0Hit, pmus.readR1Hit,
                           pmus.whiteMiss, pmus.readR0Miss, pmus.readR1Miss,
                           pmus.whiteMissNotAlloc, pmus.readR0MissNotAlloc, pmus.readR1MissNotAlloc};
    std::string location = "total hit rate fp";
    uint64_t values1 = SafeAddAll(nums1, location);
    uint64_t values2 = SafeAddAll(nums2, location);
    return CalPmuDivFp(PERCENTAGE_CONVERSION * values1, values2);
}

string Calculate::CalTotalHitRate(const L2CachePmus &pmus) const
{
    vector<uint64_t> nums1{pmus.whiteHit, pmus.readR0Hit, pmus.readR1Hit};
    vector<uint64_t> nums2{pmus.whiteHit, pmus.readR0Hit, pmus.readR1Hit,
                             pmus.whiteMiss, pmus.readR0Miss, pmus.readR1Miss,
                             pmus.whiteMissNotAlloc, pmus.readR0MissNotAlloc, pmus.readR1MissNotAlloc};
    std::string location = "total hit rate";
    uint64_t values1 = SafeAddAll(nums1, location);
    uint64_t values2 = SafeAddAll(nums2, location);
    return CalPmuDiv(PERCENTAGE_CONVERSION * values1, values2);
}

string Calculate::CalWriteHitRate(const uint64_t &pmu1, const uint64_t &pmu2, const uint64_t &pmu3) const
{
    L2CachePmus l2CachePmus{};
    l2CachePmus.whiteHit = pmu1;
    l2CachePmus.whiteMiss = pmu2;
    l2CachePmus.whiteMissNotAlloc = pmu3;
    return CalTotalHitRate(l2CachePmus);
}

string Calculate::CalReadHitRate(const L2CachePmus &l2CachePmus) const
{
    return CalTotalHitRate(l2CachePmus);
}

string Calculate::CalWriteMainMemoryDatas(const uint64_t &pmu1) const
{
    return CalTransportDatas(pmu1, TransportType::WRITE_MAIN_MEMORY);
}

string Calculate::CalReadMainMemoryDatas(const uint64_t &pmu1, const uint64_t &pmu2) const
{
    std::string location = "read main memory datas";
    return CalTransportDatas(SafeAdd(pmu1, pmu2, location), TransportType::READ_MAIN_MEMORY);
}

string Calculate::CalGmToL1Datas(const uint64_t &pmu1, const uint64_t &pmu2) const
{
    std::string location = "gm to L1 datas";
    return CalTransportDatas(SafeSub(pmu1, pmu2, location), TransportType::GM_TO_L1);
}

string Calculate::CalL1ToGmDatas(const uint64_t &pmu1, const uint64_t &pmu2, const uint64_t &pmu3) const
{
    uint64_t calFactor = 4;
    std::string location = "l1 to gm datas";
    uint64_t pmu = SafeMul(calFactor, pmu1, location);
    pmu = SafeAdd(pmu, pmu2, location);
    pmu = SafeSub(pmu, pmu3, location);
    if (pmu == std::numeric_limits<uint64_t>::max()) {
        return Common::EMPTY_METRIC_VALUE;
    }
    return CalTransportDatas(pmu, TransportType::L1_TO_GM);
}

string Calculate::CalL0cToL1Datas(const uint64_t &pmu1) const
{
    return CalTransportDatas(pmu1, TransportType::L0C_TO_L1);
}

string Calculate::CalL0cToGmDatas(const uint64_t &pmu1, const uint64_t &pmu2) const
{
    std::string location = "l0c to gm datas";
    return CalTransportDatas(SafeSub(pmu2, pmu1, location), TransportType::L0C_TO_GM);
}

string Calculate::CalGmToUbDatas(const uint64_t &pmu1) const
{
    return CalTransportDatas(pmu1, TransportType::GM_TO_UB);
}

string Calculate::CalUbToGmDatas(const uint64_t &pmu1) const
{
    return CalTransportDatas(pmu1, TransportType::UB_TO_GM);
}

string Calculate::CalGmToL1BwUsageRate(const uint64_t &pmu1, const uint64_t &pmu2) const
{
    const uint64_t type = static_cast<uint64_t>(TransportType::GM_TO_L1);
    std::string location = "gm to l1 bandwidth usage rate";
    return CalTransportBwUsageRate(SafeSub(pmu1, pmu2, location), type);
}

string Calculate::CalL1ToGmBwUsageRate(const uint64_t &pmu1, const uint64_t &pmu2, const uint64_t &pmu3) const
{
    uint64_t calFactor = 4;
    std::string location = "l1 to gm bandwidth usage rate";
    // calFactor is estimate, especially inaccurate when the data is too small.
    uint64_t pmu = SafeMul(calFactor, pmu1, location);
    pmu = SafeAdd(pmu, pmu2, location);
    pmu = SafeSub(pmu, pmu3, location);
    if (pmu == std::numeric_limits<uint64_t>::max()) {
        return Common::EMPTY_METRIC_VALUE;
    }
    const uint64_t type = static_cast<uint64_t>(TransportType::L1_TO_GM);
    return CalTransportBwUsageRate(pmu, type);
}

string Calculate::CalL0cToL1BwUsageRate(const uint64_t &pmu1) const
{
    const uint64_t type = static_cast<uint64_t>(TransportType::L0C_TO_L1);
    return CalTransportBwUsageRate(pmu1, type);
}

string Calculate::CalL0cToGmBwUsageRate(const uint64_t &pmu1, const uint64_t &pmu2) const
{
    const uint64_t type = static_cast<uint64_t>(TransportType::L0C_TO_GM);
    std::string location = "l0 to gm bandWidth usage ratio";
    return CalTransportBwUsageRate(SafeSub(pmu2, pmu1, location), type);
}

string Calculate::CalGmToUbBwUsageRate(const uint64_t &pmu1) const
{
    const uint64_t type = static_cast<uint64_t>(TransportType::GM_TO_UB);
    return CalTransportBwUsageRate(pmu1, type);
}

string Calculate::CalUbToGmBwUsageRate(const uint64_t &pmu1) const
{
    const uint64_t type = static_cast<uint64_t>(TransportType::UB_TO_GM);
    return CalTransportBwUsageRate(pmu1, type);
}

string Calculate::CalL2CacheHitRate(const uint64_t &pmu1, const uint64_t &pmu2, const uint64_t &pmu3) const
{
    std::string location = "l2 cache hit rate";
    uint64_t totalCache = SafeAdd(pmu2, pmu3, location);
    if (pmu1 > totalCache) {
        return EMPTY_METRIC_VALUE;
    }
    return CalPmuDiv(PERCENTAGE_CONVERSION * pmu1, totalCache);
}

string Calculate::CalTransportDatas(const uint64_t &pmu, const TransportType &type)
{
    auto it = REQ_DATA_OF_910B.find(type);
    if (it == REQ_DATA_OF_910B.end() || pmu == std::numeric_limits<uint64_t>::max()) {
        return EMPTY_METRIC_VALUE;
    }
    return to_string(static_cast<float>(pmu) * it->second / BIT_CONVERSION);
}

string Calculate::CalTransportBwUsageRate(const uint64_t &pmu, const uint64_t type) const
{
    if (pmu == std::numeric_limits<uint64_t>::max()) {
        return EMPTY_METRIC_VALUE;
    }
    if (type >= static_cast<uint64_t>(TransportType::UNKNOWN)) {
        return EMPTY_METRIC_VALUE;
    }
    TransportType transportType = static_cast<TransportType>(type);

    auto dataIt = REQ_DATA_OF_910B.find(transportType);
    if (dataIt == REQ_DATA_OF_910B.end()) {
        return EMPTY_METRIC_VALUE;
    }
    uint16_t dataOfSingleReq = dataIt->second;
    map<TransportType, float> transportBwMap = GetMaxBwBySoc(socVersion_, ChipProductType::ASCEND910B1);
    auto typeIt = transportBwMap.find(transportType);
    if (typeIt == transportBwMap.end()) {
        return EMPTY_METRIC_VALUE;
    }
    float maxBwRate = typeIt->second;

    float time = 0.0f;
    if (!IsZero(duration_)) {
        // only mix op and without kernel scale
        time = duration_;
    } else if (freq_ != 0) {
        time = static_cast<float>(totalCycles_) / freq_ * timeFactor_;
    }
    if (IsZero(time) || IsZero(maxBwRate)) {
        return EMPTY_METRIC_VALUE;
    }
    float calBwRate = static_cast<float>(pmu) * dataOfSingleReq * TIME_CONVERSION * TIME_CONVERSION /
                      BIT_CONVERSION / BIT_CONVERSION / BIT_CONVERSION / time;
    // exceed maxBwRate, usage rate will be 100%
    if (calBwRate > maxBwRate) {
        calBwRate = maxBwRate;
    }
    return to_string(calBwRate * PERCENTAGE_CONVERSION / maxBwRate);
}

std::string Calculate::CalAicMte1ActivateBw(const Mte1BwPmusWithSize &mte1) const
{
    uint64_t l0aByte = mte1.l0aPmu * mte1.l0aSize < mte1.gmToL0a ? 0 : mte1.l0aPmu * mte1.l0aSize - mte1.gmToL0a;
    uint64_t l0bByte = mte1.l0bPmu * mte1.l0bSize < mte1.gmToL0b ? 0 : mte1.l0bPmu * mte1.l0bSize - mte1.gmToL0b;
    float dataBit = static_cast<float>(l0aByte) * mte1.l0aPipe + l0bByte * mte1.l0bPipe;
    float mte1Time = freq_ <= 0 ? 0.0f : static_cast<float>(mte1.mte1CyclePmu) / freq_;
    if (IsZero(mte1Time)) {
        return "NA";
    }
    return std::to_string(static_cast<float>(dataBit) / mte1Time / BANDWIDTH_NUM * TIME_CONVERSION * TIME_CONVERSION);
}

std::string Calculate::CalAicMte2ActivateBw(const uint64_t &gmToL1, const uint64_t &gmToL0a,
    const uint64_t &gmToL0b, const uint64_t &pmu) const
{
    float mte2Time = freq_ <= 0 ? 0.0f : static_cast<float>(pmu) / freq_;
    if (IsZero(mte2Time)) {
        return "NA";
    }
    float dataSize = static_cast<float>(gmToL1) + gmToL0a + gmToL0b;
    return std::to_string(dataSize / mte2Time / BIT_CONVERSION / BIT_CONVERSION / BIT_CONVERSION
        * TIME_CONVERSION * TIME_CONVERSION);
}

std::string Calculate::CalAicMte3ActivateBw(const uint64_t &pipSize, const uint64_t &pmu1, const uint64_t &pmu2,
    const uint64_t &pmu3, const uint64_t &pmu4) const
{
    uint64_t calFactor = 4;

    float mte3Time = freq_ <= 0 ? 0.0f : static_cast<float>(pmu4) / freq_;
    if (calFactor * pmu1 + pmu2 < pmu3 || IsZero(mte3Time)) {
        return "NA";
    }
    float dataSize = static_cast<float>((calFactor * pmu1 + pmu2 - pmu3) * pipSize);
    return std::to_string(dataSize / mte3Time / BIT_CONVERSION / BIT_CONVERSION / BIT_CONVERSION
        * TIME_CONVERSION * TIME_CONVERSION);
}

std::string Calculate::CalAivMteActivateBw(const uint64_t &pipSize, const uint64_t &pmu1, const uint64_t &pmu2) const
{
    float aivMte2Time = freq_ <= 0 ? 0.0f : static_cast<float>(pmu2) / freq_;
    if (freq_ <= 0 || IsZero(aivMte2Time)) {
        return "NA";
    }
    float dataSize = static_cast<float>(pmu1 * pipSize);
    return std::to_string(dataSize / aivMte2Time / BIT_CONVERSION / BIT_CONVERSION / BIT_CONVERSION
        * TIME_CONVERSION * TIME_CONVERSION);
}

uint64_t GetPmuValue(const map<uint16_t, uint64_t> &pmuEventValueMap, const uint64_t &eventId)
{
    auto it = pmuEventValueMap.find(eventId);
    if (it == pmuEventValueMap.end()) {
        return EMPTY_PMU_VALUE;
    }
    return it->second;
}

std::vector<uint64_t > GetDbiReq(const string &metric, std::map<std::string, uint64_t> &dbiMap)
{
    vector<uint64_t> requests = {};
    for (const std::string &pipe: DbiPipeMap[metric]) {
        requests.emplace_back(dbiMap[pipe]);
    }
    return requests;
}

map<string, string> CalMetricItems(const Calculate &cal, const set<string> &metricItems,
    const map<string, CalInfo> &formula, std::map<std::string, uint64_t> &dbiMap)
{
    map<string, string> metricDatas;
    for (const string &item: metricItems) {
        auto it = formula.find(item);
        if (it == formula.end()) {
            metricDatas[item] = Common::EMPTY_METRIC_VALUE;
            LogDebug("Can not cal metric item [%s] for formula not exists.", item.c_str());
            continue;
        }
        CalInfo calInfo = it->second;
        vector<uint64_t> pmuEvents = calInfo.pmuEvents;
        vector<uint64_t> params = calInfo.params;
        if (std::find(pipeDbiFor910B_.begin(), pipeDbiFor910B_.end(), item) != pipeDbiFor910B_.end()) {
            auto pipeRequest = GetDbiReq(item, dbiMap);
            params.insert(params.end(), pipeRequest.begin(), pipeRequest.end());
        }
        for (const uint64_t &event: pmuEvents) {
            // if any event does not exist, the metric value will be NA
            uint64_t pmuValue = cal.GetPmuEventValue(event);
            if (pmuValue == Common::EMPTY_PMU_VALUE) {
                metricDatas[item] = Common::EMPTY_METRIC_VALUE;
                LogDebug("Can not cal metric item [%s] for event value is empty.", item.c_str());
                break;
            }
            params.emplace_back(pmuValue);
        }
        if (metricDatas[item] != Common::EMPTY_METRIC_VALUE && Functions.count(calInfo.funcType) != 0) {
            metricDatas[item] = Functions.at(calInfo.funcType)(cal, params);
        }
    }
    return metricDatas;
}

map<string, string> CalMetricItems(const CalculateParams &params, const set<std::string> &metricItems,
    const std::map<std::string, std::function<std::string(const CalculateParams &params)>> &formula)
{
    map<string, string> metricDatas;
    for (const string &item: metricItems) {
        auto it = formula.find(item);
        if (it == formula.end()) {
            metricDatas[item] = Common::EMPTY_METRIC_VALUE;
            LogDebug("Can not cal metric item [%s] for formula not exists.", item.c_str());
            continue;
        }
        auto equation = it->second;
        metricDatas[item] = equation(params);
    }
    return metricDatas;
}

}
