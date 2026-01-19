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

#ifndef __MSOPPROF_PROFILING_METRIC_DATA_HANDLER_H__
#define __MSOPPROF_PROFILING_METRIC_DATA_HANDLER_H__
#include <map>
#include <vector>

#include "data_format.h"
#include "common/prof_args.h"
#include "profiling/device/data_visualize/data_visualize_const.h"
#include "data_bean.h"
#include "metric_csv_header.h"
#include "pmu_formulator.h"
#include "common/dbi_defs.h"
#include "l2cache/l2cache.h"
#include "profiling/op_prof_task.h"
#include "common/dbi_defs.h"

namespace Profiling {
struct ParserConfig {
    std::string outputPath;
    std::set<std::string> aicCalMetricItems;
    std::set<std::string> aivCalMetricItems;
    bool kernelScale;
};

using BlockIDPairType = std::pair<uint16_t, std::string>;
struct BlockProfBinData {
    PmuEventValueMapType pmuEventValueMap;
    uint64_t totalCycles;
    std::string blockType;
    std::pair<uint64_t, uint64_t> systemTime; // last replay start/end systemTime
};
using BlockPmuMapType = std::map<BlockIDPairType, BlockProfBinData>;

struct ProfBinInfo {
    std::vector<uint16_t> aicEvents;
    std::vector<uint16_t> aivEvents;
    std::set<std::string> relatedMetrics;
    inline std::string GetAllRelatedMetrics() const
    {
        return Utility::Join(relatedMetrics.begin(), relatedMetrics.end(), ",");
    }
};

struct DefaultDeviceInfo {
    int64_t aicoreFreq;
    int64_t taskSchedulerFreq;
    int64_t aiCoreNum;
};

class DataHandler {
public:
    virtual ~DataHandler() = default;
    bool ParseDeviceData(const ParserConfig &config, const std::map<std::string, ProfBinInfo> &profBinMap,
        const Common::ProfMetricsAbilityConfig &metrics, const std::string &timeStamp);
    void MergeTotalPmuData(SplitBlockPmuData &blockData);
    virtual void ParseProfBin(const std::vector<char> &binData, const size_t &fileSize, const ProfBinInfo &binInfo) {}
    virtual void ParseDurationBin(const std::string &outputPath, const std::vector<char> &binData,
                                  const size_t &fileSize, uint64_t &startTime, uint64_t &endTime) {}
    virtual void ReadAndParseL2CacheBin(const std::string &outputPath, const Common::ProfMetricsAbilityConfig &metrics) {}
    std::vector<Common::MemRecord> ParseMemoryChartBin(const std::string &memoryChartBin) const;
    bool ParseL2CacheBin(const std::string &filePath, const std::vector<uint16_t> &pmuVec, SplitBlockPmuData &blockData) const;
    virtual std::string GetOpType() // 在主函数里起到Set的作用
    {
        std::string str;
        return str;
    }
    virtual std::vector<uint64_t> GetVisualizeEvents(bool isComputeLoad, const std::string &blockType)
    {
        return {};
    }

    std::map<std::string, std::vector<std::string>> metricHeader = {};
    std::map<std::string, std::function<std::string(const CalculateParams &params)>> formulaOfA5_;
    std::map<std::string, CalInfo> formula = {};
    std::vector<std::string> opBasicHeader = {};
    // 最后一次重放采集的BlockId与对应物理CoreId，因为每次重放的物理coreId不相同，BlockId相同，所以只记录最后一次重放的coreId数据
    std::vector<std::pair<uint16_t, uint16_t>> blockIdCoreIdPairVec_;

    std::map<uint16_t, std::vector<float>> GetBlockDetailMap() { return blockDetailMap_; }
    std::vector<Visualize::ComputeLoadBlockDetail> GetComputeLoadBlockDetailVec() {return computeLoadBlockDetailVec_; }
    std::vector<Visualize::MemMapDetail> GetMemMapDetail() {return memMapDetail_; }
    std::vector<std::pair<uint16_t, uint16_t>> GetBlockIdCoreIdPairVec() const { return blockIdCoreIdPairVec_; }
    Visualize::AcsqTimeMapType GetAcsqTimeMap() { return acsqTimeMap_; }
    inline uint64_t GetMinMc2TimeCyc() const { return minMc2TimeCyc_; }
    inline uint64_t GetMinLcclTimeCyc() const { return minLcclTimeCyc_; }
    BlockPmuMapType GetTotalPmuData() { return totalPmuData_; }
    const std::shared_ptr<L2Cache> &GetL2CacheObj() { return l2Cache_; }

    inline std::string GetOpName() const { return opName_; }
    inline std::string GetSoc() const { return soc_; }
    inline std::string GetBlockDim() const { return basicBlockDim_; }
    inline std::string GetMixBlockDim() const { return mixBlockDim_; }
    inline std::string GetDeviceId() { return deviceId_; }
    inline std::string GetPid() { return pid_; }
    inline std::string GetCurFreq() { return basicCurFreq_; }
    inline std::string GetRatedFreq() { return ratedFreq_; }
    inline bool GetMC2Flag() const { return isMC2_; }
    inline bool GetLcclFlag() const { return isLccl_; }
    inline float GetDuration() const { return duration_; }
    inline void SetOpName(const std::string& name) { opName_ = name; }
    inline void SetSoc(const std::string& soc) { soc_ = soc; }
    inline void ClearOpType() { opType_ = ' '; }
    inline void SetBlockDim(const std::string& blockDim) { basicBlockDim_ = blockDim; }
    inline void SetMixBlockDim(const std::string& mixBlockDim) { mixBlockDim_ = mixBlockDim; }
    inline void SetDeviceId(const std::string& deviceId) { deviceId_ = deviceId; }
    inline void SetPid(const std::string& pid) { pid_ = pid; }
    inline void SetCurFreq(const std::string& curFreq) { basicCurFreq_ = curFreq; }
    inline void SetRatedFreq(const std::string& ratedFreq) { ratedFreq_ = ratedFreq; }
    inline void SetDuration(float duration) { duration_ = duration; }
    inline  std::map<uint64_t, std::map<std::string, uint64_t>> GetDbiMap() { return dbiMap_; }
    inline int64_t GetL2CacheEvict() const { return l2CacheEvict_; }

    int64_t aicoreFreq_;
    int64_t taskSchedulerFreq_;
    int64_t aiCoreNum_;
    Common::ChipType chipType_;
    int64_t l2CacheEvict_ = -1;
    uint64_t pcOffsetByPcSampling_ = 0;
protected:
    virtual void ParseMemoryChartData(const std::string &outputPath, const Common::ProfMetricsAbilityConfig &metrics,
                                      std::vector<Common::MemRecord> &memoryRecords,
                                      const std::string &kernelName = "") {}
    virtual void Statics(std::vector<Common::MemRecord> &memoryRecords,
                         std::map<uint64_t, std::map<std::string, uint64_t>> &dataSize) {}
    void InitPlatformInfo();
    DefaultDeviceInfo defaultDeviceInfo_;
    // For MC2 timeline, record stars times
    Visualize::AcsqTimeMapType acsqTimeMap_;
    uint64_t minMc2TimeCyc_ = UINT64_MAX;
    uint64_t minLcclTimeCyc_ = UINT64_MAX;
    bool isMC2_ = false;
    bool isLccl_ = false;
    std::vector<Visualize::MemMapDetail> memMapDetail_;
    std::shared_ptr<L2Cache> l2Cache_;

private:
    bool SaveOpBasicInfo(const std::string &outputPath);
    void HandleDuration(const std::string &outputPath);
    bool ParseBasicInfoFile(const std::string &filePath);
    bool HandlePmuData(const std::string &outputPath, const std::map<std::string, ProfBinInfo> &profBinMap,
                       Common::ProfMetricsAbilityConfig metrics);
    bool CalMetrics(const ParserConfig &config, const Common::ProfMetricsAbilityConfig &metrics,
                    std::map<uint64_t, std::map<std::string, uint64_t>> &dbiMap);
    bool SaveMetrics(const std::string &outputPath, const Common::ProfMetricsAbilityConfig &metrics,
                     std::vector<std::map<std::string, std::string>> &totalCalValues);
    void SetOpVisualizeData();
    void AddComputeBlockDetailToVisualize(const BlockIDPairType& blockIdPairType, const BlockProfBinData &blockProfBinData,
        const Visualize::TypeOperandRecord &recordMap, std::vector<Visualize::ComputeLoadBlockDetail> &blockDetailMap);
    void AddMemMapDetailToVisualize(const BlockIDPairType& blockIdPairType,
        const BlockProfBinData &blockProfBinData, std::vector<Visualize::MemMapDetail> &memMapDetail);
    bool LoadOpBasicInfoTxtFile(const std::string &filePath);
    void ParseOperandRecords(const std::string &recordBinPath);
    Visualize::TypeOperandRecord GetOperandRecordMap(uint16_t blockId, const std::string &subBlockId, const std::string &opType);
    void ParsePcSamplingRecords(const std::string &recordBinPath);

    std::string opName_;
    std::string soc_;
    // std::string opType_;
    std::string basicBlockDim_;
    std::string mixBlockDim_;   // mix_block_dim for mix op
    std::string deviceId_;
    std::string pid_;
    std::string basicCurFreq_;
    std::string ratedFreq_;
    // float duration_;
    std::map<uint16_t, std::vector<float>> blockDetailMap_;
    std::vector<Visualize::ComputeLoadBlockDetail> computeLoadBlockDetailVec_{};

    uint64_t blockDim_{};
    std::string socVersion_;
    std::map<std::string, std::string> opInfoMap_;
    BlockPmuMapType totalPmuData_;
    std::string profilingFileTimeStamp_ {"19700101"};
    int64_t curFreq_;
    float duration_ = 0.0f;
    std::string opType_;
    std::map<uint64_t, std::map<std::string, uint64_t>> dbiMap_;
    std::vector<Visualize::TypeOperandRecord> operandRecords_;
};

class DataHandlerOf910B : public DataHandler {
public:
    DataHandlerOf910B()
    {
        metricHeader = MetricHeaderFor910B;
        formula = FormulaFor910B;
        opBasicHeader = OpBasicHeaderFor910B;
        defaultDeviceInfo_ = {DEFAULT_AICORE_FREQ, DEFAULT_TSCPU_FREQ, DEFAULT_AICORE_NUM};
        chipType_ = Common::ChipType::ASCEND910B;
        InitPlatformInfo();
    }

    void ParseProfBin(const std::vector<char> &binData, const size_t &fileSize, const ProfBinInfo &binInfo) override;
    void ParseDurationBin(const std::string &outputPath, const std::vector<char> &binData, const size_t &fileSize,
                          uint64_t &startTime, uint64_t &endTime) override;
    std::string GetOpType() override;
    std::vector<uint64_t> GetVisualizeEvents(bool isComputeLoad, const std::string &blockType) override;
    void ReadAndParseL2CacheBin(const std::string &outputPath, const Common::ProfMetricsAbilityConfig &metrics) override;
private:
    void ParseMemoryChartData(const std::string &outputPath, const Common::ProfMetricsAbilityConfig &metrics,
                              std::vector<Common::MemRecord> &memoryRecords,
                              const std::string &kernelName = "") override;
    void Statics(std::vector<Common::MemRecord> &memoryRecords,
                 std::map<uint64_t, std::map<std::string, uint64_t>> &dataSize) override;
    static constexpr int64_t DEFAULT_AICORE_FREQ = 1850;
    static constexpr int64_t DEFAULT_TSCPU_FREQ = 50000;
    static constexpr int64_t DEFAULT_AICORE_NUM = 24;
    bool isAic_ = false;
    bool isAiv_ = false;
};

class DataHandlerOf310P : public DataHandler {
public:
    DataHandlerOf310P()
    {
        metricHeader = MetricHeaderFor310P;
        formula = FormulaFor310P;
        opBasicHeader = OpBasicHeaderFor310P;
        defaultDeviceInfo_ = {DEFAULT_AICORE_FREQ, DEFAULT_TSCPU_FREQ, DEFAULT_AICORE_NUM};
        chipType_ = Common::ChipType::ASCEND310P;
        InitPlatformInfo();
    }

    void ParseProfBin(const std::vector<char> &binData, const size_t &fileSize, const ProfBinInfo &binInfo) override;
    void ParseDurationBin(const std::string &outputPath, const std::vector<char> &binData, const size_t &fileSize,
                          uint64_t &startTime, uint64_t &endTime) override;
    void ReadAndParseL2CacheBin(const std::string &outputPath, const Common::ProfMetricsAbilityConfig &metrics) override;
    std::string GetOpType() override;
    std::vector<uint64_t> GetVisualizeEvents(bool isComputeLoad, const std::string &blockType) override;
private:
    static constexpr int64_t DEFAULT_AICORE_FREQ = 1060;
    static constexpr int64_t DEFAULT_TSCPU_FREQ = 38400;
    static constexpr int64_t DEFAULT_AICORE_NUM = 10;
};

class DataHandlerOf91095 : public DataHandler {
public:
    DataHandlerOf91095()
    {
        metricHeader = MetricHeaderForA5;
        opBasicHeader = OpBasicHeaderForA5;
        formulaOfA5_ = FormulaForA5;
        defaultDeviceInfo_ = {DEFAULT_AICORE_FREQ, DEFAULT_TSCPU_FREQ, DEFAULT_AICORE_NUM};
        chipType_ = Common::ChipType::ASCEND910_95;
        InitPlatformInfo();
    }

    void ParseProfBin(const std::vector<char> &binData, const size_t &fileSize, const ProfBinInfo &binInfo) override;
    void ParseDurationBin(const std::string &outputPath, const std::vector<char> &binData, const size_t &fileSize,
                          uint64_t &startTime, uint64_t &endTime) override;
    std::string GetOpType() override;
    std::vector<uint64_t> GetVisualizeEvents(bool isComputeLoad, const std::string &blockType) override;
private:
    void ParseMemoryChartData(const std::string &outputPath, const Common::ProfMetricsAbilityConfig &metrics,
                              std::vector<Common::MemRecord> &memoryRecords,
                              const std::string &kernelName = "") override;
    void Statics(std::vector<Common::MemRecord> &memoryRecords,
                 std::map<uint64_t, std::map<std::string, uint64_t>> &dataSize) override;
    static constexpr int64_t DEFAULT_AICORE_FREQ = 1650;
    // DEFAULT_TSCPU_FREQ needs to be obtained from the driver and updated after physical goods are available.
    static constexpr int64_t DEFAULT_TSCPU_FREQ = 1000000;
    static constexpr int64_t DEFAULT_AICORE_NUM = 32;
    bool isAic_ = false;
    bool isAiv_ = false;
};
}
#endif // __MSOPPROF_PROFILING_METRIC_DATA_HANDLER_H__
