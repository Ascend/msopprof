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


#ifndef __MSOPPROF_NUMBER_OPERATION_H__
#define __MSOPPROF_NUMBER_OPERATION_H__

#include <cmath>
#include <limits>
#include <string>

#include "log.h"

namespace Utility {
template<typename T>
inline typename std::enable_if<std::is_floating_point<T>::value, bool>::type IsZero(T num)
{
    return std::abs(num) < std::numeric_limits<T>::epsilon();
}

template<typename T>
inline typename std::enable_if<std::is_integral<T>::value, bool>::type IsZero(T num)
{
    return num == 0;
}

template<typename T>
bool SafeEqual(T a, T b, T precision = 0,
               typename std::enable_if<
               !std::is_floating_point<T>::value,
               void>::type* = nullptr)
{
    (void)precision;
    return a == b;
}

// FP numbers must be judged for equality by setting the precision.
// High-precision comparisons can specify the precision additionally.
template<typename T>
bool SafeEqual(T a, T b, T precision = 0.0001,
               typename std::enable_if<
               std::is_floating_point<T>::value,
               void>::type* = nullptr)
{
    return (std::fabs(a - b) <= precision);
}

template<typename T>
T OverFlowReturnValue(T num, bool returnValue)
{
    if (!returnValue) {
        return 0;
    }
    if (std::signbit(num)) {
        return std::numeric_limits<T>::lowest();
    }
    return std::numeric_limits<T>::max();
}

template<typename T>
T SafeAdd(T num1, T num2, const std::string &location, bool returnValue = true)
{
    T maxValue = std::numeric_limits<T>::max();
    T minValue = std::numeric_limits<T>::lowest();
    if (std::is_signed<T>::value) {
        if (num1 > 0 && num2 > 0 && num1 > maxValue - num2) {
            LogDebug("Signed addition overflow at %s, num1=%ld, num2=%ld", location.c_str(), (long)num1, (long)num2);
            return OverFlowReturnValue(maxValue, returnValue);
        }
        if (num1 < 0 && num2 < 0 && num1 < minValue - num2) {
            LogDebug("Signed addition underflow at %s, num1=%ld, num2=%ld", location.c_str(), (long)num1, (long)num2);
            return OverFlowReturnValue(minValue, returnValue);
        }
    } else {
        if (num1 > maxValue - num2) {
            LogDebug("Unsigned addition overflow at %s, num1=%lu, num2=%lu", location.c_str(), (unsigned long)num1, (unsigned long)num2);
            return OverFlowReturnValue(maxValue, returnValue);
        }
    }
    return num1 + num2;
}

template<typename T>
T SafeSub(T num1, T num2, const std::string &location, bool returnValue = true)
{
    T minValue = std::numeric_limits<T>::lowest();
    T maxValue = std::numeric_limits<T>::max();
    if (std::is_signed<T>::value) {
        if (num1 >= 0 && num2 < 0 && num1 > maxValue + num2) {
            LogDebug("Signed subtraction overflow at %s, num1=%ld, num2=%ld", location.c_str(), (long)num1, (long)num2);
            return OverFlowReturnValue(maxValue, returnValue);
        }
        if (num1 < 0 && num2 > 0 && num1 < minValue + num2) {
            LogDebug("Signed subtraction underflow at %s, num1=%ld, num2=%ld", location.c_str(), (long)num1, (long)num2);
            return OverFlowReturnValue(minValue, returnValue);
        }
    } else {
        if (num1 < num2) {
            LogDebug("Unsigned subtraction overflow at %s, num1=%lu, num2=%lu", location.c_str(), (unsigned long)num1, (unsigned long)num2);
            return OverFlowReturnValue(maxValue, returnValue);
        }
    }
    return num1 - num2;
}

template<typename T>
bool HaveSameSign(T num1, T num2)
{
    return std::signbit(num1) == std::signbit(num2);
}

template<typename T>
bool CheckOverFlow(T num)
{
    if (std::is_signed<T>::value) {
        if (SafeEqual(num, std::numeric_limits<T>::max()) || SafeEqual(num, std::numeric_limits<T>::lowest())) {
            return true;
        }
    } else {
        if (SafeEqual(num, std::numeric_limits<T>::max())) {
            return true;
        }
    }
    return false;
}

template<typename T>
T SafeMul(T num1, T num2, const std::string &location, bool returnValue = true)
{
    if (SafeEqual(num1, T(0)) || SafeEqual(num2, T(0))) {
        return 0;
    }
    T minValue = std::numeric_limits<T>::lowest();
    T maxValue = std::numeric_limits<T>::max();
    T res = num1 * num2;
    if (std::is_same<T, float>::value) {
        if (!SafeEqual(res / num1, num2)) {
            LogDebug("Float multiplication overflow at %s, num1=%f, num2=%f", location.c_str(), (double)num1, (double)num2);
            if (HaveSameSign(num1, num2)) {
                return OverFlowReturnValue(maxValue, returnValue);
            }
            return OverFlowReturnValue(minValue, returnValue);
        }
    } else if (!SafeEqual(res / num1, num2)) {
        LogDebug("Integer multiplication overflow at %s, num1=%ld, num2=%ld", location.c_str(), (long)num1, (long)num2);
        if (HaveSameSign(num1, num2)) {
                return OverFlowReturnValue(maxValue, returnValue);
            }
        return OverFlowReturnValue(minValue, returnValue);
    }
    return res;
}

template<typename T>
T SafeAddAll(const std::vector<T> &nums, const std::string &location, bool returnValue = true)
{
    T sum = 0;
    for (const auto& num : nums) {
        sum = SafeAdd(sum, num, location, returnValue);
        if (CheckOverFlow(sum)) {
            return sum;
        }
    }
    return sum;
}

template<typename T>
T SafeMulAll(const std::vector<T>& nums, const std::string &location, bool returnValue = true)
{
    T res = 1;
    for (const auto& num : nums) {
        if (SafeEqual(num, T(0))) {
            return 0;
        }
        res = SafeMul(res, num, location, returnValue);
        if (CheckOverFlow(res)) {
            res = OverFlowReturnValue(res, returnValue);
            return res;
        }
    }
    return res;
}

// The numerical range of T1 must be greater than that of T2.This function is used to
// calculate the sum of the products of the corresponding elements multiply of two vectors.
template<typename T1, typename T2>
T1 SafeMulAddAll(const std::vector<T1>& nums1, const std::vector<T2>& nums2, const std::string &location,
                 bool returnValue = true)
{
    if (nums1.size() != nums2.size()) {
        LogDebug("The multiplication and addition all function receive wrong param size in %s", location.c_str());
        return std::numeric_limits<T1>::max();
    }
    T1 res = 0;
    for (size_t i = 0; i < nums1.size(); i++) {
        T1 temp = SafeMul(nums1[i], static_cast<T1>(nums2[i]), location, returnValue);
        res = SafeAdd(res, temp, location, returnValue);
        if (CheckOverFlow(res)) {
            return res;
        }
    }
    return res;
}

inline uint64_t ExtractKBits(uint64_t regValue, uint32_t startBit, uint32_t bitLength)
{
    constexpr uint32_t bitSize = 64;
    if (startBit >= bitSize || bitLength >= bitSize) {
        return 0;
    }
    return (regValue >> startBit) & ((1ULL << bitLength) - 1);
}

}

#endif // __MSOPPROF_NUMBER_OPERATION_H__
