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


#ifndef __MSOPPROF_PROFILING_HOTSPOT_FUNCTION_GENERATOR_H__
#define __MSOPPROF_PROFILING_HOTSPOT_FUNCTION_GENERATOR_H__

#include <string>
#include <utility>
#include <vector>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include "profiling/op_prof_data_parse.h"
#include "common/dbi_defs.h"
#include "l2cache/l2cache.h"
#include "common/visualize.h"
#include "common/defs.h"
#include "parse_timeline.h"
#include "tlv_parse.h"
#include "log.h"

namespace Profiling {

struct Encoding {
    uint64_t addr;
    std::string pipe;
    std::string source;
    std::string cceCode;
    uint64_t calls;
    uint32_t l2cacheHit;
    uint32_t l2cacheMiss;
    int processBytes;
    uint32_t gprCount;
    std::vector<uint64_t> pcSampling;
    std::vector<RecordStatus> gprStatus;
};

// Base class for pcSampling-related data (CodeLine and InstrInfo)
class BaseSource {
public:
    float l2cacheHitRate;
    float samplingPercentNotIssue;
    float samplingPercentAllSample;
    int processBytes;
    uint32_t gprCount;
    std::vector<uint64_t> pcSampling;

    BaseSource()
        : l2cacheHitRate(-1.0f)
        , samplingPercentNotIssue(0.0f)
        , samplingPercentAllSample(0.0f)
        , processBytes(0)
        , gprCount(0)
        , pcSampling(9, 0)
    {
    }

    virtual ~BaseSource() = default;

    // Generate stall sampling JSON structure
    nlohmann::json GenStallSampling(bool allSample) const;

protected:
    // Helper function to add common fields to JSON
    void AddCommonFieldsToJson(nlohmann::json &jsonObj) const;
};

// CodeLine represents a line of source code with profiling data
class CodeLine : public BaseSource {
public:
    int line{};
    std::vector<int> callCount;
    std::vector<std::vector<std::string>> addrRange;

    CodeLine() : BaseSource(), line(0) {}

    void ToJson(const std::string &socVersion, nlohmann::json &lineDetails);
};

// CodeFile represents a source file with multiple code lines
struct CodeFile {
    std::string file;
    std::vector<CodeLine> lines;
};

// InstrInfo represents an instruction with profiling data
class InstrInfo : public BaseSource {
public:
    std::string addr;
    std::string cceCode;
    std::string instr;
    std::string pipe;
    std::vector<int> callCount;
    std::vector<RecordStatus> gprStatus;
    InstrInfo() : BaseSource() {}

    void ToJson(const std::string &socVersion, nlohmann::json &instrDetails) const;
};

struct FileDtype {
    int addrRange = static_cast<int>(Utility::VisualizeBinDType::DO_NOT_DISPLAY);
    int instructionExecuted = static_cast<int>(Utility::VisualizeBinDType::INT);
    int line = static_cast<int>(Utility::VisualizeBinDType::INT);
    int l2cacheHitRate = static_cast<int>(Utility::VisualizeBinDType::PERCENTAGE);
    int processBytes = static_cast<int>(Utility::VisualizeBinDType::INT);
    int gprCount = static_cast<int>(Utility::VisualizeBinDType::INT);
    int pcSampling = static_cast<int>(Utility::VisualizeBinDType::CUSTOM_PERCENTAGE);
};

struct InstructionsDtype {
    int address = static_cast<int>(Utility::VisualizeBinDType::STRING);
    int ascendCInnerCode = static_cast<int>(Utility::VisualizeBinDType::STRING);
    int instructionExecuted = static_cast<int>(Utility::VisualizeBinDType::INT);
    int pipe = static_cast<int>(Utility::VisualizeBinDType::STRING);
    int source = static_cast<int>(Utility::VisualizeBinDType::STRING);
    int l2cacheHitRate = static_cast<int>(Utility::VisualizeBinDType::PERCENTAGE);
    int processBytes = static_cast<int>(Utility::VisualizeBinDType::INT);
    int pcSampling = static_cast<int>(Utility::VisualizeBinDType::CUSTOM_PERCENTAGE);
    int gprCount = static_cast<int>(Utility::VisualizeBinDType::INT);
    int gprStatus = static_cast<int>(Utility::VisualizeBinDType::JSON_STR);
};

struct HotspotFuncArgs {
    std::string socVersion;
    std::string kernelName;
    uint64_t pcOffset;
    bool sourceEnable;
    bool pcSamplingEnable;
    bool memdetailEnable;
};

/*
 * @brief 此类用于生成上板代码热点图
 */
class HotSpotFunctionGenerator {
public:
    /*
    * @brief：构造函数
    *
    * @param：芯片类型
    */
    explicit HotSpotFunctionGenerator(const HotspotFuncArgs &args)
        : pcOffset_(args.pcOffset), socVersion_(std::move(args.socVersion)), sourceEnable_(args.sourceEnable),
        pcSamplingEnable_(args.pcSamplingEnable), memdetailEnable_(args.memdetailEnable)
    {
        if (args.kernelName.empty()) {
            beginAddr_[""] = 0;
            endAddr_[""] = UINT64_MAX;
            Utility::LogDebug("Init Hotspot function, kernel name is null");
        } else {
            Utility::LogDebug("Init Hotspot function, kernel name is %s", args.kernelName.c_str());
            size_t pos;
            kernelName_.emplace_back(args.kernelName);
            beginAddr_[args.kernelName] = UINT64_MAX;
            endAddr_[args.kernelName] = 0;
            std::string otherTypeName = args.kernelName;
            if ((pos = args.kernelName.find(Common::MIX_AIC_TAIL)) != std::string::npos) {
                otherTypeName.replace(pos, std::string(Common::MIX_AIC_TAIL).length(), Common::MIX_AIV_TAIL);
                kernelName_.emplace_back(otherTypeName);
            } else if ((pos = args.kernelName.find(Common::MIX_AIV_TAIL)) != std::string::npos) {
                otherTypeName.replace(pos, std::string(Common::MIX_AIV_TAIL).length(), Common::MIX_AIC_TAIL);
                kernelName_.emplace_back(otherTypeName);
            }
            if (otherTypeName != args.kernelName) {
                beginAddr_[otherTypeName] = UINT64_MAX;
                endAddr_[otherTypeName] = 0;
            }
            if (args.kernelName.find(Common::MIX_AIC_TAIL) ==std::string::npos && args.kernelName.find(Common::MIX_AIV_TAIL)) {
                auto aicName = args.kernelName + Common::MIX_AIC_TAIL;
                auto aivName = args.kernelName + Common::MIX_AIV_TAIL;
                kernelName_.emplace_back(aicName);
                kernelName_.emplace_back(aivName);
                beginAddr_[aicName] = UINT64_MAX;
                endAddr_[aicName] = 0;
                beginAddr_[aivName] = UINT64_MAX;
                endAddr_[aivName] = 0;
            }
        }
    }
    /*
    * @brief：全流程处理函数，唯一对外接口
    *
    * @param：outputPath 数据处理的路径
    * @return：是否正确生成上板代码热点图
    * @output：visualize_data.bin文件中加入fileApi.json和InstrApi.json内容
    */
    bool Process(const std::string &outputPath, const std::vector<Common::MemRecord> &memoryRecords,
                 const std::shared_ptr<L2Cache> &l2CachePtr);
private:
    /*
    * @brief：BBcount相关产物的处理函数，其中包含更新bbbmap(UpdateBBBMap)、extra(UpdateExtra)
    *         以及生成fdata(GenFdata)，并得到每个bb块的调用次数(GenBBCalls)
    *
    * @param：dumpPath dump数据处理的路径
    * @return：是否正确处理
    * @output：产物为成员变量bbCalls_和kernelStartAddr_
    */
    bool ProcessBBCount(const std::string &dumpPath);

    /*
    * @brief：处理encoding函数，调用InstrEncoding的接口得到地址对应的指令pipe和source
    *
    *
    * @param：kernelPath为aicore_binary.o的路径
    * @return：是否正确处理
    * @output：产物为成员变量encodings_
    */
    bool ProcessEncoding(const std::string &kernelPath, const std::shared_ptr<L2Cache> &l2CachePtr);

    /*
    * @brief：调用Symbolizer生成得到offset2Lines，并结合bbCalls得到每个encoding指令的调用次数和每个文件每行的调用次数
    *
    * @param：kernelPath为aicore_binary.o存储的路径
    * @return：是否正确处理
    * @output：产物为encodings_中填补calls字段，生成fileLineCalls_成员变量（文件行号对应的调用次数），
    *          line2Encodings（文件行号对应的encoding指令）
    */
    bool GenLine2Encodings(const std::string &kernelPath, std::map<std::string, std::vector<Encoding>> &line2Encodings);

    /*
    * @brief：生成vector<CodeFile>，用作最后转换成fileApi.json，主要负责计算每行代码对应的pc地址范围addrRange以及
    *         结合fileLineCalls_得到每行代码的调用次数callCount
    *
    * @param：outputPath为visualize_data.bin存储的路径（存储源码路径），line2Encodings为文件行号对应的encoding数组
    * @return：是否正确处理
    * @output：产物为codeFiles vector，对应可视化数据中的fileApi.json,以及在visualize_data.bin写入源码路径
    */
    bool GenCodeFiles(const std::string &outputPath, const std::map<std::string, std::vector<Encoding>> &line2Encodings,
                      std::vector<CodeFile> &codeFiles);

    /*
    * @brief：基于encodings得到vector<InstrInfo>，用作后续输出为InstrApi.json
    *
    * @return：是否正确处理
    * @output：产物为vector<InstrInfo>，对应可视化数据中的InstrApi.json
    */
    bool GenInstrInfos(std::vector<InstrInfo> &instrInfos);

    /*
    * @brief：将vector<InstrInfo>和vector<CodeFile>转换成json格式，然后写入visualize_data.bin
    *
    * @param：outputPath为visualize_data.bin存储的路径，vector<InstrInfo>和vector<CodeFile>
    * @output：产物为visualize_data.bin中写入InstrApi.json和FileApi.json
    */
    void GenVisualizeData(const std::string &outputPath, const std::vector<CodeFile>& codeFiles,
                          const std::vector<InstrInfo>& instrInfos) const;

    bool CheckBBBMapPermission(const std::string &bbbmapPath) const;
    size_t MatchBbbId(const std::regex &blockIdPattern, const uint64_t startAddr,
                      size_t id, std::vector<std::string> &bbbmap, const std::string &kernelName);
    bool UpdateBBBMap(const std::string &bbbmapPath);
    bool UpdateExtra(const std::string &extraPath) const;
    bool GenFdata(const std::string &bbmapPath, const std::string &extraPath, const std::string &fdataPath);
    bool GenTlvdata(const std::string &kernelPath, const std::string &tlvdataPath);
    bool GenBBCalls(const std::vector<std::string> &fData);
    bool GenAddr2Lines(const std::string &kernelPath, const std::vector<std::string>& addrVec,
                       std::unordered_map<uint64_t, std::vector<std::string>> &addr2Lines);
    void SetFileDtype(nlohmann::json &apiJson) const;
    void SetInstrDtype(nlohmann::json &apiJson) const;
    void UpdateProcessBytes(const std::vector<Common::MemRecord> &memoryRecords);
    bool GenCodeLine(const std::string &line, const std::vector<Encoding> &encodings, CodeLine &codeLine) const;
    void UpdatePcSampling(const std::string &dumpPath);
    void ReadPcSamplingData(const std::string &filePath);
    void ParsePcSamplingData(const std::vector<uint8_t> &pcSamplingData,
                             std::map<uint64_t, std::vector<uint64_t>> &orderedPcStateMap) const;
    bool ExtractData(const std::string &outputPath);
    bool CalculateData(const std::string &outputPath, const std::vector<Common::MemRecord> &memoryRecords,
                       const std::shared_ptr<L2Cache> &l2CachePtr,
                       std::map<std::string, std::vector<Encoding>> &line2Encodings);
    bool VisualizeData(const std::string &outputPath, std::map<std::string, std::vector<Encoding>> &line2Encodings);
    bool IsObjKernel(const std::string &kernelName);
    bool IsObjKernelPcAddr(uint64_t pc);
    void AccumulatePcSampling(const std::vector<uint64_t> &pcSampling, uint64_t &notIssue, uint64_t &allSample) const;

    std::map<uint64_t, std::pair<uint64_t, uint64_t>> bbCalls_; // bbCalls_[beginAddr] = {endAddr, calls}
    std::unordered_map<std::string, uint64_t> kernelStartAddr_; // {kernelName, start_addr}
    std::unordered_map<std::string, std::map<uint64_t, uint64_t>> fileLineCalls_;   // {fileName, {lineNo, calls}}
    std::unordered_map<uint64_t, Encoding> encodings_;
    uint64_t startPc_ = 0;
    uint64_t startPcForPcSampling_ = 0;
    uint64_t pcOffset_ = 0;
    uint64_t samplingWithNotIssueSum_;
    uint64_t samplingWithAllSum_;
    uint64_t totalPcSamplingNotIssue_ = 0;
    uint64_t totalPcSamplingAllSample_ = 0;
    std::string socVersion_;
    bool sourceEnable_ {false};
    bool pcSamplingEnable_ {false};
    bool memdetailEnable_ {false};
    std::vector<std::string> kernelName_;
    std::map<std::string, uint64_t> beginAddr_;
    std::map<std::string, uint64_t> endAddr_;
    std::unordered_set<std::string> skipKeys_;
};

}

#endif