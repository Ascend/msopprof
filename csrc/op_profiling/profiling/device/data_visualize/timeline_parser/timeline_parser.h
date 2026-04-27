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

#ifndef TIME_PARSER_H
#define TIME_PARSER_H

#include <fstream>
#include <utility>
#include <vector>
#include <cstdint>
#include "json.hpp"

#include "profiling/device/data_visualize/basic_op_and_pmu.h"
#include "profiling/device/data_visualize/data_visualize_const.h"
#include "profapi/profapi.h"
#include "profiling/simulator/data_parse/sim_defs.h"
#include "common/hal_helper.h"
namespace Visualize {
using json = nlohmann::json;

constexpr int64_t FREQ = 50000; // kHz
constexpr uint16_t TIME_CONVERSION = 1000; // time in visualize.bin will be us, cyc/FREQ unit is ms
constexpr uint16_t SORT_BIAS = 10000;
constexpr uint16_t A2_VEC_START = 25;
constexpr uint16_t A5_VEC_START_RANGE_ONE = 18;
constexpr uint16_t A5_VEC_END_RANGE_ONE = 53;
constexpr uint16_t A5_VEC_START_RANGE_TWO = 72;
constexpr uint16_t A5_VEC_END_RANGE_TWO = 107;
constexpr uint32_t TIME_STAMP_START = 0xffff;
constexpr char const *AIC_BLOCK = "AIC BLOCK";
constexpr char const *AIV_BLOCK = "AIV BLOCK";


struct JsonEvent {
    inline void ToJson(nlohmann::json &jsonData) const
    {
        jsonData["name"] = this->name;
        jsonData["cname"] = this->cName;
        jsonData["ph"] = this->ph;
        jsonData["pid"] = this->pid;
        jsonData["tid"] = this->tid;
        jsonData["ts"] = this->ts;
        jsonData["dur"] = this->dur;
        jsonData["args"] = this->args;
    }

    std::string name;
    std::string cName;
    std::string ph;
    std::string pid;
    std::string tid;
    float ts;
    float dur;
    std::map<std::string, std::string> args;
};

class TimelineParser  {
public:
     TimelineParser(uint64_t minTimeCyc, std::shared_ptr<OpBasicInfo> &opBasicInfoObj, std::shared_ptr<BasicPmu> &basicPmuObj)
        : minSysCyc_(minTimeCyc), opBasicInfoObj_(opBasicInfoObj), basicPmuObj_(basicPmuObj)
        {
            for (const auto &pair: basicPmuObj_->GetTotalPmuData()) {
                blockSystemTimes_[pair.first.first].emplace_back(pair.second.systemTime);
                minSysCyc_ = std::min(pair.second.systemTime.first, minSysCyc_);
            }
            std::string socversion = opBasicInfoObj_->GetSoc();
            chipType_ = GetProductSeriesType(socversion);
            if (!Common::HalHelper::Instance().GetTaskSchedulerFreq(aicpuFreq_)) {
                Utility::LogWarn("Get task scheduler frequency failed. Use default value instead");
                aicpuFreq_ = FREQ;
            }
        }
    virtual bool TimelineToJson(const std::string &outputPath) = 0;
    void AddAicoreDuration(uint64_t startTime);
    void ProcessAicoreBlockDur();
    nlohmann::json GetTimelineJson() { return timelineJson_; }
    ChipProductType GetChipTypeSeries() { return chipType_; }
    float GetRunTime(uint64_t freq, int64_t cycles);

    template<typename T>
    bool GetTimeStamp(std::vector<T> &timestamps)
    {
        std::string binPath = Utility::JoinPath({outputPath_, "dump/aic_timestamp.bin"});
        if (!Utility::IsExist(binPath)) {
            return false;
        }
        size_t fileSize =  Utility::GetFileSize(binPath);
        size_t structSize = sizeof(T);
        std::vector<char> binData;
        if (fileSize < structSize || !Utility::ReadBinFileByMultiStruct(binPath, fileSize, structSize, binData)) {
            return false;
        }
        for (size_t i = 0; i < fileSize; i = i + structSize) {
            T info {};
            if (memcpy_s(&info, structSize, &binData[i], structSize) != EOK) {
                continue;
            }
            if (info.syscyc < minSysCyc_) {
                continue;
            }
            timestamps.push_back(info);
        }
        if (timestamps.size() == 0) {
            return false;
        }
        return true;
    }

    Profiling::Pc2CodeMap pc2code_;
    uint64_t minSysCyc_ = UINT64_MAX;
    // key {pid：类型aic/aiv, tid: block}, value:对应的aicore运行时间始末记录
    std::map<std::pair<std::string, uint32_t>, std::pair<uint64_t, uint64_t>> blockDuration_;
    std::string outputPath_;
    nlohmann::json timelineJson_;
    int64_t aicpuFreq_ = 0;
private :
    void SortTimelineByIds(const std::set<uint16_t> &aicDotBlockIds, const std::set<uint16_t> &aivDotBlockIds);
    BlockSystemTimeType blockSystemTimes_;
    ChipProductType chipType_;
    std::shared_ptr<OpBasicInfo> &opBasicInfoObj_;
    std::shared_ptr<BasicPmu> &basicPmuObj_;

};
}

#endif // TIME_PARSER_H