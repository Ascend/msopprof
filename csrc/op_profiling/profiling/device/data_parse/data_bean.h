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

#ifndef __MSOPPROF_PROFILING_DATA_BEAN_H__
#define __MSOPPROF_PROFILING_DATA_BEAN_H__
#include <string>
#include <map>
#include <vector>

#include "common/defs.h"


namespace Profiling {
using PmuEventValueMapType = std::map<uint16_t, uint64_t>;

constexpr uint16_t FFTS_LENGTH = 128;
constexpr uint16_t ACSQ_LENGTH = 64;
constexpr uint16_t ACSQ_LENGTH_A5 = 32;
constexpr uint16_t AICORE_LENGTH = 128;
constexpr uint16_t L2_CACHE_LENGTH = 72;
constexpr uint16_t HWTS_LENGTH = 64;
constexpr uint16_t FUNC_TYPE_BLOCK_PMU = 0x29;

enum class SqeType : uint16_t {
    AIC_TYPE = 0,
    AIV_TYPE,
    MIX_TYPE,
};

struct SplitBlockPmuData {
    PmuEventValueMapType pmuEventValueMap;
    std::string blockType;
    uint64_t totalCycles;
    uint16_t blockId;
    std::string subBlockId;
    uint16_t coreId;
    uint64_t startSystemTime;
    uint64_t endSystemTime;
};

class FftsBlockBean {
public:
    explicit FftsBlockBean(const ChipProductType &chip, const std::vector<char> &bin);

    SplitBlockPmuData GetBlockData(const std::vector<std::uint16_t> &aicEvents,
                                   const std::vector<std::uint16_t> &aivEvents) const;
    uint16_t GetStreamId() const { return fftsBlockData_.streamId; };
    uint16_t GetTaskId() const { return fftsBlockData_.taskId; };
    uint16_t GetFuncType() const { return fftsBlockData_.funcType; };

private:
    // format: 4short 1longlong 4short 1int 2short 12longlong, total 128B
    struct FftsConstruct {
        uint16_t shortNums1[4];
        uint64_t longlongNums1[1];
        uint16_t shortNums2[4];
        uint32_t intNums[1];
        uint16_t shortNums3[2];
        uint64_t longlongNums2[12];
    } fftsConstruct_{};

    // format: 4short 1longlong 4short 2short 1int 12longlong, total 128B，详细见表格-上板数据列表A5/stars
    struct FftsConstructA5 {
        uint16_t shortNums1[4];
        uint64_t longlongNums1[1];
        uint16_t shortNums2[4];
        uint16_t shortNums3[2];
        uint32_t intNums[1];
        uint64_t longlongNums2[12];
    } fftsConstructA5_{};

    struct FftsBlockData {
        uint16_t coreId;
        uint16_t blockId;
        uint16_t subBlockId;
        uint16_t coreType;
        uint16_t streamId;
        uint16_t taskId;
        uint16_t funcType;
        uint64_t totalCycles;
        std::vector<uint64_t> pmuValues;
        uint64_t startSystemTime;
        uint64_t endSystemTime;
    } fftsBlockData_;

    void InitA5(const std::vector<char> &bin);
    void InitA2(const std::vector<char> &bin);
    uint64_t GetTotalCycles() const { return fftsBlockData_.totalCycles; };
    std::vector<uint64_t> GetPmuValues() const { return fftsBlockData_.pmuValues; };
    uint16_t GetBlockId() const { return fftsBlockData_.blockId; };
    uint16_t GetSubBlockId() const { return fftsBlockData_.subBlockId; };
    uint16_t GetCoreType() const { return fftsBlockData_.coreType; };
    uint16_t GetCoreId() const { return fftsBlockData_.coreId; };
    uint64_t GetStartSystemTime() const { return fftsBlockData_.startSystemTime; };
    uint64_t GetEndSystemTime() const { return fftsBlockData_.endSystemTime; };
};

class AcsqBean {
public:
    AcsqBean(const ChipProductType &chip, const std::vector<char> &bin);

    uint16_t GetTaskType() const;

    Common::TimeType GetTimeType() const;

    uint16_t GetStreamId() const;

    uint16_t GetTaskId() const;

    uint64_t GetSystemTime() const;

private:
    // format: 4short 1longlong 2short 11int, total 64B
    struct AcsqConstruct {
        uint16_t shortNums1[4];
        uint64_t longlongNums[1];
        uint16_t shortNums2[2];
        uint32_t intNums[11];
    } acsqConstruct_{};

    // format: 4short 1longlong 2short 3int, total 32B
    struct AcsqA5Construct {
        uint16_t shortNums1[4];
        uint64_t longlongNums[1];
        uint16_t shortNums2[2];
        uint32_t intNums[3];
    } acsqA5Construct_{};

    struct AcsqData {
        uint16_t taskType;
        uint16_t funcType;
        uint16_t taskId;
        uint16_t streamId;
        uint64_t systemTime;
    } acsqData_{};
};

class AiCoreBean {
public:
    explicit AiCoreBean(const std::vector<char> &bin);

    SplitBlockPmuData GetAiCoreData(const std::vector<uint16_t> &events) const;
private:
    // format: 2char 3short 2int 10longlong 8int, total 128B
    struct AiCoreConstruct {
        unsigned char charNums[2];
        uint16_t shortNums[3];
        uint32_t intNums1[2];
        uint64_t longlongNums[10];
        uint32_t intNums2[8];
    } aiCoreConstruct_{};

    struct AiCoreData {
        uint64_t totalCycles;
        std::vector<uint64_t> pmuValues;
    } aiCoreData_;

    uint64_t GetTotalCycles() const;

    std::vector<uint64_t> GetPmuValues() const;
};

class L2CacheBean {
public:
    explicit L2CacheBean(const std::vector<char> &bin);

    SplitBlockPmuData GetL2CacheData(const std::vector<uint16_t> &events) const;
private:
    // format: 4short 8longlong, total 72B
    struct L2CacheConstruct {
        uint16_t shortNums[4];
        uint64_t longlongNums[8];
    } l2CacheConstruct_{};

    struct L2CacheData {
        std::vector<uint64_t> pmuValues;
    } l2CacheData_;

    std::vector<uint64_t> GetPmuValues() const;
};

class HwtsBean {
public:
    explicit HwtsBean(const std::vector<char> &bin);

    Common::TimeType GetTimeType() const;

    uint64_t GetSystemTime() const;

private:
    // format: 2char 3short 1longlong 12int, total 64B
    struct HwtsConstruct {
        unsigned char charNums[2];
        uint16_t shortNums[3];
        uint64_t longlongNums[1];
        uint32_t intNums2[12];
    } hwtsConstruct_{};

    struct HwtsData {
        uint16_t funcType;
        uint64_t systemTime;
    } hwtsData_{};
};
}
#endif // __MSOPPROF_PROFILING_DATA_BEAN_H__