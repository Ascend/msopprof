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

#include "equation_utils.h"
#include "pmu_calculate.h"
#include "common/hal_helper.h"
#include "number_operation.h"

using namespace Common;

namespace Profiling {
std::string Ratio(uint64_t val1, uint64_t val2)
{
    if (val2 == 0) {
        return EMPTY_METRIC_VALUE;
    }
    return std::to_string(static_cast<float>(val1) / val2);
}

float RatioFp(uint64_t val1, uint64_t val2)
{
    if (val2 == 0) {
        return 0.0f;
    }
    return static_cast<float>(val1) / val2;
}

std::string BandWidth(float val, float duration, float timeFactor)
{
    // duration unit is us
    if (Utility::IsZero(duration)  || val < 0) {
        return EMPTY_METRIC_VALUE;
    }
    return std::to_string(val * timeFactor * TIME_CONVERSION * TIME_CONVERSION / duration);
}

float BandWidthFp(float val, float duration, float timeFactor)
{
    // duration unit is us
    if (Utility::IsZero(duration)  || val < 0) {
        return 0.0f;
    }
    return val * timeFactor * TIME_CONVERSION * TIME_CONVERSION / duration;
}

bool GetMaxBandWidthByType(const TransportType &type, const ChipProductType &chipType, float &maxBw)
{
    GmType gmType = GmType::DEFAULT;
    ChipProductType series = GetProductSeriesType(chipType);
    if (series == ChipProductType::ASCEND910B_SERIES && HalHelper::Instance().GetGmType() == GmType::CJ) {
        gmType = GmType::CJ;
    }
    std::pair<ChipProductType, GmType> search = {chipType, gmType};
    if (MAX_BW_RATE_ALL.find(search) == MAX_BW_RATE_ALL.end()) {
        Utility::LogWarn("Failed to get max bandwidth of type : %d, gm type is %d",
            static_cast<int>(type), static_cast<int>(chipType));
        return false;
    }
    auto maxBwCollection = MAX_BW_RATE_ALL.at(search);
    if (maxBwCollection.find(type) == maxBwCollection.end()) {
        Utility::LogWarn("Failed to get max bandwidth of type : %d", static_cast<int>(type));
        return false;
    }
    maxBw = maxBwCollection.at(type);
    return true;
}

uint64_t GetDataNumber(uint64_t pmu, uint64_t request)
{
    std::string location = "calculate data number";
    return Utility::SafeMul(pmu, request, location, false);
}

float GetDataNumberFp(uint64_t pmu, uint64_t request, const std::string& unit)
{
    if (unit == "KB") {
        return static_cast<float>(GetDataNumber(pmu, request)) / BIT_CONVERSION;
    }
    return static_cast<float>(GetDataNumber(pmu, request)) / (BIT_CONVERSION * BIT_CONVERSION * BIT_CONVERSION);
}

std::string BandWidthUsage(float val, float duration, TransportType type, ChipProductType chipType)
{
    if (static_cast<uint64_t>(type) >= static_cast<uint64_t>(TransportType::UNKNOWN)) {
        return EMPTY_METRIC_VALUE;
    }
    float maxBw = 0.0f;
    if (!GetMaxBandWidthByType(type, chipType, maxBw) || Utility::IsZero(duration) || Utility::IsZero(maxBw)) {
        return EMPTY_METRIC_VALUE;
    }
    float calBw = BandWidthFp(val, duration);
    // exceed maxBw, usage rate will be 100%
    if (calBw > maxBw) {
        calBw = maxBw;
    }
    return std::to_string(calBw * PERCENTAGE_CONVERSION / maxBw);
}

std::string BandWidthUsage(float val, float duration, TransportType type, const std::string &socVersion)
{
    auto chipType = GetProductSeriesTypeBySocVersion(socVersion);
    return BandWidthUsage(val, duration, type, chipType);
}

std::map<TransportType, float> GetMaxBwBySoc(const std::string &socVersion, const ChipProductType &defalutChip)
{
    // defalutChip must in MAX_BW_RATE_ALL
    auto chipType = GetProductSeriesTypeBySocVersion(socVersion);
    GmType gmType = GmType::DEFAULT;
    if (defalutChip == ChipProductType::ASCEND910B1) {
        gmType = HalHelper::Instance().GetGmType();
    }
    auto iter = MAX_BW_RATE_ALL.find({chipType, gmType});
    if (iter != MAX_BW_RATE_ALL.end()) {
        return iter->second;
    } else {
        Utility::LogDebug("Missing theoretical bandwidth for soc %s, using default value", socVersion.c_str());
        return MAX_BW_RATE_ALL.at({defalutChip, GmType::DEFAULT});
    }
}
}