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

#ifndef __MSOPPROF_PROFILING_EQUATION_UTILS_H__
#define __MSOPPROF_PROFILING_EQUATION_UTILS_H__
#include <map>
#include <string>

#include "common/defs.h"
#include "device_defs.h"
 
namespace Profiling {

std::string Ratio(uint64_t val1, uint64_t val2);
float RatioFp(uint64_t val1, uint64_t val2);
std::string BandWidth(float val, float duration, float timeFactor = 1.0);
float BandWidthFp(float val, float duration, float timeFactor = 1.0);
bool GetMaxBandWidthByType(const TransportType &type, const ChipProductType &chipType, float &maxBw);
uint64_t GetDataNumber(uint64_t pmu, uint64_t request);
float GetDataNumberFp(uint64_t pmu, uint64_t request, const std::string& unit = "GB");
std::string BandWidthUsage(float val, float duration, TransportType type, ChipProductType chipType);
std::string BandWidthUsage(float val, float duration, TransportType type, const std::string &socVersion);
std::map<TransportType, float> GetMaxBwBySoc(const std::string &socVersion, const ChipProductType &defalutChip);
}
 
#endif // __MSOPPROF_PROFILING_EQUATION_UTILS_H__