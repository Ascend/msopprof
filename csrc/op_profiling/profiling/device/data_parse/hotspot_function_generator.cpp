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


#include "hotspot_function_generator.h"
#include <json.hpp>
#include <algorithm>
#include <regex>
#include "common/visualize.h"
#include "filesystem.h"
#include "cmd_execute.h"
#include "ustring.h"
#include "instr_encoding/instr_encoding.h"
#include "pc_process.h"
#include "tlv_parse.h"
#include "ustring.h"

using namespace Encode;
using namespace Utility;
namespace Profiling {

static constexpr uint64_t INSTR_SIZE = 4;
static constexpr float PERCENTAGE = 100;
static constexpr int PC_SAMPLING_DATA_LEN = 16;
static constexpr int PC_SAMPLING_DATA_STATUS_OFFSET = 3;
static constexpr int PC_SAMPLING_STATUS_NUM = 9;
static constexpr int TOP_STALL_REASON_NUM = 8;
static constexpr char const *PC_SAMPLING_STATE_NAMES[PC_SAMPLING_STATUS_NUM] = {
    "IBuf_Empty",
    "Nop_Cycles",
    "Scoreboard_Not_Ready",
    "Register_bank_conflict",
    "Resource_conflict",
    "Warp_Level_Sync",
    "Divergence_Stack_Spill",
    "Others",
    "Active"
};
nlohmann::json BaseSource::GenStallSampling(bool allSample) const
{
    nlohmann::json samplingJson;
    samplingJson["Percent"] = allSample ? this->samplingPercentAllSample : this->samplingPercentNotIssue;
    size_t stateCnt = allSample ? PC_SAMPLING_STATUS_NUM : TOP_STALL_REASON_NUM;
    nlohmann::json dataDetail;
    for (size_t i = 0; i < stateCnt; ++i) {
        dataDetail[PC_SAMPLING_STATE_NAMES[i]] = this->pcSampling[i];
    }
    samplingJson["Details"] = dataDetail;
    return samplingJson;
}

void BaseSource::AddCommonFieldsToJson(nlohmann::json &jsonObj) const
{
    jsonObj["GPR Count"] = this->gprCount;
    if (this->processBytes != 0) {
        jsonObj["Process Bytes"] = this->processBytes;
    } else {
        jsonObj["Process Bytes"] = -1;
    }
}

void CodeLine::ToJson(const std::string &socVersion, nlohmann::json &lineDetails)
{
    ChipProductType chipType = GetProductTypeBySocVersion(socVersion);
    lineDetails["Line"] = this->line;
    lineDetails["Address Range"] = this->addrRange;
    lineDetails["Instructions Executed"] = this->callCount;
    AddCommonFieldsToJson(lineDetails);

    if (IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND950_SERIES)) {
        lineDetails["Stall Sampling(Not Issue)"] = GenStallSampling(false);
        lineDetails["Stall Sampling(All Samples)"] = GenStallSampling(true);
        return;
    }
    if (IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND910B_SERIES) ||
        IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND910_93_SERIES)) {
        lineDetails["L2Cache Hit Rate"] = this->l2cacheHitRate;
    }
}

void InstrInfo::ToJson(const std::string &socVersion, nlohmann::json &instrDetails) const
{
    ChipProductType chipType = GetProductTypeBySocVersion(socVersion);
    instrDetails["Address"] = this->addr;
    instrDetails["AscendC Inner Code"] = this->cceCode;
    instrDetails["Source"] = this->instr;
    instrDetails["Pipe"] = this->pipe;
    std::vector<nlohmann::json> statusJson;
    for (const auto& status : this->gprStatus) {
        nlohmann::json jsonData;
        status.ToJson(jsonData);
        statusJson.emplace_back(jsonData);
    }
    instrDetails["GPR Status"] = statusJson;
    instrDetails["Instructions Executed"] = this->callCount;
    AddCommonFieldsToJson(instrDetails);

    if (IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND950_SERIES)) {
        instrDetails["Stall Sampling(Not Issue)"] = GenStallSampling(false);
        instrDetails["Stall Sampling(All Samples)"] = GenStallSampling(true);
        return;
    }
    if (IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND910B_SERIES) ||
        IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND910_93_SERIES)) {
        instrDetails["L2Cache Hit Rate"] = this->l2cacheHitRate;
    }
}

bool HotSpotFunctionGenerator::ExtractData(const std::string &outputPath)
{
    std::string dumpPath = Utility::JoinPath({outputPath, "dump"});
    if (!Utility::StoullConverter(GetStartPcFromDump(dumpPath, Common::PC_START_PATH), startPc_, RADIX_16)) {
        startPc_ = 0;
        Utility::LogWarn("Failed to get start pc");
    }
    if (!Utility::StoullConverter(GetStartPcFromDump(dumpPath, "pc_start_pcsampling.txt"), startPcForPcSampling_, RADIX_16)) {
        startPcForPcSampling_ = startPc_;
        Utility::LogDebug("Failed to get start pc for pcsampling");
    }
    if (!ProcessBBCount(dumpPath)) {
        Utility::LogWarn("Failed to process BBCount");
        if (pcSamplingEnable_) {
            sourceEnable_ = false;
            return true;
        }
        return false;
    }
    return true;
}

bool HotSpotFunctionGenerator::CalculateData(const std::string &outputPath,
                                             const std::vector<Common::MemRecord> &memoryRecords,
                                             const std::shared_ptr<L2Cache> &l2CachePtr,
                                             std::map<std::string, std::vector<Encoding>> &line2Encodings)
{
    ChipProductType chipType = GetProductTypeBySocVersion(socVersion_);
    std::string dumpPath = Utility::JoinPath({outputPath, Common::DUMP});
    std::string kernelPath = Utility::JoinPath({dumpPath, Common::AICORE_KERNEL_NAME});
    if (!ProcessEncoding(kernelPath, l2CachePtr)) {
        Utility::LogDebug("Failed to process encoding");
        return false;
    }
    if (pcSamplingEnable_) {
        UpdatePcSampling(dumpPath);
    }
    if (IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND950_SERIES) && !GenTlvdata(kernelPath, dumpPath)) {
        Utility::LogWarn("Failed to gen register data");
    }
    if (memdetailEnable_) {
        UpdateProcessBytes(memoryRecords);
    }
    if (!GenLine2Encodings(kernelPath, line2Encodings)) {
        Utility::LogDebug("Failed to get Line2Encodings");
        return false;
    }
    return true;
}

bool HotSpotFunctionGenerator::IsObjKernel(const std::string &kernelName)
{
    if (kernelName_.empty()) {
        return true;
    }
    for (const auto& i : kernelName_) {
        if (i == kernelName) {
            return true;
        }
    }
    return false;
}

bool HotSpotFunctionGenerator::IsObjKernelPcAddr(uint64_t pc)
{
    if (kernelName_.empty()) {
        return true;
    }
    for (const auto& i : kernelName_) {
        if (pc >= beginAddr_[i] && pc <= endAddr_[i]) {
            return true;
        }
    }
    return false;
}

bool HotSpotFunctionGenerator::VisualizeData(const std::string &outputPath,
                                             std::map<std::string, std::vector<Encoding>> &line2Encodings)
{
    // 计算所有指令的总 pcSampling 计数
    totalPcSamplingNotIssue_ = 0;
    totalPcSamplingAllSample_ = 0;
    for (const auto &item : encodings_) {
        AccumulatePcSampling(item.second.pcSampling, totalPcSamplingNotIssue_, totalPcSamplingAllSample_);
    }

    std::vector<CodeFile> codeFiles;
    if (!GenCodeFiles(outputPath, line2Encodings, codeFiles)) {
        Utility::LogDebug("Failed to gen CodeFiles");
        return false;
    }

    std::vector<InstrInfo> instrInfos;
    if (!GenInstrInfos(instrInfos)) {
        Utility::LogDebug("Failed to gen InstrInfos");
        return false;
    }
    GenVisualizeData(outputPath, codeFiles, instrInfos);
    return true;
}

bool HotSpotFunctionGenerator::Process(const std::string &outputPath,
                                       const std::vector<Common::MemRecord> &memoryRecords,
                                       const std::shared_ptr<L2Cache> &l2CachePtr)
{
    std::map<std::string, std::vector<Encoding>> line2Encodings;
    if (!ExtractData(outputPath)) {
        LogWarn("Hot spot function extract data failed");
        return false;
    }
    if (!CalculateData(outputPath, memoryRecords, l2CachePtr, line2Encodings)) {
        LogWarn("Hot spot function calculate data failed");
        return false;
    }
    if ((sourceEnable_ || pcSamplingEnable_) && !VisualizeData(outputPath, line2Encodings)) {
        LogWarn("Hot spot function visualize data failed");
        return false;
    }
    if (pcSamplingEnable_) {
        WriteTopStallReasonFigure(outputPath);
    }
    return true;
}

bool HotSpotFunctionGenerator::ProcessBBCount(const std::string &dumpPath)
{
    ChipProductType chipType = GetProductTypeBySocVersion(socVersion_);
    if (!IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND910B_SERIES) &&
        !IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND910_93_SERIES) &&
        !IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND950_SERIES)) {
        return true;
    }
    if (!sourceEnable_) {
        return true;
    }
    std::vector<std::string> filenames;
    if (!Utility::ListDir(dumpPath, std::back_inserter(filenames))) {
        return false;
    }
    std::smatch matches;
    const std::regex extraPattern("kernel([0-9]+)\\.extra\\.([0-9]+)");
    const std::regex bbbmapPattern("kernel([0-9]+)Stub\\.o\\.bbbmap\\.([0-9]+)");
    std::string extraPath;
    std::string bbbmapPath;
    for (const auto &filename : filenames) {
        if (std::regex_match(filename, extraPattern)) {
            extraPath = Utility::JoinPath({dumpPath, "/", filename});
        }
        if (std::regex_match(filename, bbbmapPattern)) {
            bbbmapPath = Utility::JoinPath({dumpPath, "/", filename});
        }
    }
    if (!UpdateBBBMap(bbbmapPath)) {
        return false;
    }
    if (!UpdateExtra(extraPath)) {
        return false;
    }
    std::string fdataPath = Utility::JoinPath({dumpPath, "/fdata"});
    if (!GenFdata(bbbmapPath, extraPath, fdataPath)) {
        Utility::LogDebug("Failed to gen fdata");
        return false;
    }
    return true;
}

void HotSpotFunctionGenerator::UpdateProcessBytes(const std::vector<Common::MemRecord> &memoryRecords)
{
    std::unordered_map<uint64_t, uint64_t> mergedByPcMemRecordsMap;
    for (auto oneMemRecord : memoryRecords) {
        if (oneMemRecord.src == Common::MemType::GM) {
            mergedByPcMemRecordsMap[oneMemRecord.pc] += oneMemRecord.srcMemSize;
        } else if (oneMemRecord.dst == Common::MemType::GM) {
            mergedByPcMemRecordsMap[oneMemRecord.pc] += oneMemRecord.dstMemSize;
        }
    }
    for (auto &item : encodings_) {
        Encoding &oneEncoding = item.second;
        auto it = mergedByPcMemRecordsMap.find(oneEncoding.addr);
        if (it != mergedByPcMemRecordsMap.end()) {
            oneEncoding.processBytes = static_cast<int>(it->second);
        }
    }
}

void HotSpotFunctionGenerator::UpdatePcSampling(const std::string &dumpPath)
{
    std::vector<std::string> filenames;
    topStallReasonByCore_.fill(0);
    if (!Utility::ListDir(dumpPath, std::back_inserter(filenames))) {
        return;
    }
    bool hasPcSampling = false;
    for (auto &file : filenames) {
        if (StartsWith(file, "pcSampling.bin")) {
            std::string filePath = JoinPath({dumpPath, file});
            ReadPcSamplingData(filePath);
            hasPcSampling = true;
        }
    }
    if (!hasPcSampling) {
        pcSamplingEnable_ = false;
    }
}

void HotSpotFunctionGenerator::ReadPcSamplingData(const std::string &filePath)
{
    std::unordered_set<uint64_t> missingPcSet;
    if (!IsReadable(filePath)) {
        LogWarn("File %s is not exists or not readable.", filePath.c_str());
        return;
    }
    size_t fileSize = GetFileSize(filePath);
    std::vector<char> totalBin(fileSize);
    size_t headSize = sizeof(Visualize::InstrProfHeadInfo);
    size_t structSize = headSize + Visualize::DATA_BUFFER_SIZE;
    if (!ReadBinFileByMultiStruct(filePath, fileSize, structSize, totalBin)) {
        LogWarn("File %s failed to be read.", filePath.c_str());
        return;
    }
    // 第一个for循环取数据，每组数据包含一个数据头以及2M的数据段
    for (size_t i = 0; i < fileSize; i += structSize) {
        Visualize::InstrProfHeadInfo instrProfHeadInfo;
        if (memcpy_s(&instrProfHeadInfo, headSize, &totalBin[i], headSize) != EOK) {
            LogDebug("Instr profiling data memory get error");
            continue;
        }
        std::vector<char> splitData{&totalBin[i] + headSize, &totalBin[i] + structSize};
        // 第二个for循环分割数据段，每个数据段中的一组数据为16Byte
        for (size_t j = 0; j < instrProfHeadInfo.validLen && j < splitData.size(); j += PC_SAMPLING_DATA_LEN) {
            std::vector<uint8_t> pcSamplingData(PC_SAMPLING_DATA_LEN, 0);
            if (memcpy_s(pcSamplingData.data(), PC_SAMPLING_DATA_LEN, &splitData[j], PC_SAMPLING_DATA_LEN) != EOK) {
                LogDebug("pc sampling data memory get error");
                continue;
            }
            uint64_t pc = ParsePcSamplingData(pcSamplingData);
            AccumulatePcSamplingRecord(pc, pcSamplingData, missingPcSet);
        }
    }
}

uint64_t HotSpotFunctionGenerator::ParsePcSamplingData(const std::vector<uint8_t> &pcSamplingData) const
{
    uint64_t pc = 0;
    for (int t = 0; t < PC_SAMPLING_DATA_STATUS_OFFSET; ++t) {
        pc |= (static_cast<uint64_t>(pcSamplingData[t]) << (t * 8));
    }
    // pc sampling 数据24位标识26:3的地址，所以*8进行偏移，减去start pc 以及插桩pc offset
    // 0x7FFFFFFUL为27位掩码，防止pc越界导致错误数据,表示范围到[26:0]
    pc = pc * 8 - (startPcForPcSampling_ & 0x7FFFFFFUL) - pcOffset_;
    return pc;
}

void HotSpotFunctionGenerator::AccumulatePcSamplingRecord(uint64_t pc, const std::vector<uint8_t> &pcSamplingData,
                                                          std::unordered_set<uint64_t> &missingPcSet)
{
    auto encodingItem = encodings_.find(pc);
    if (encodingItem == encodings_.end()) {
        if (missingPcSet.insert(pc).second) {
            LogDebug("PC-Sampling pc [%lx] is not in encodings", pc);
        }
        return;
    }
    if (encodingItem->second.pcSampling.size() != PC_SAMPLING_STATUS_NUM) {
        encodingItem->second.pcSampling.resize(PC_SAMPLING_STATUS_NUM);
    }
    for (int i = 0; i < PC_SAMPLING_STATUS_NUM; ++i) {
        const uint64_t value = static_cast<uint64_t>(pcSamplingData[i + PC_SAMPLING_DATA_STATUS_OFFSET]);
        encodingItem->second.pcSampling[i] += value;
        if (i < static_cast<int>(topStallReasonByCore_.size())) {
            topStallReasonByCore_[i] += value;
        }
    }
}

nlohmann::json HotSpotFunctionGenerator::GenTopStallReasonFigureJson() const
{
    nlohmann::json topStallReasonData;
    for (size_t i = 0; i < TOP_STALL_REASON_NUM; ++i) {
        topStallReasonData[PC_SAMPLING_STATE_NAMES[i]] = topStallReasonByCore_[i];
    }
    nlohmann::json figure;
    figure["top_stall_reason_table"] = topStallReasonData;
    return figure;
}

void HotSpotFunctionGenerator::WriteTopStallReasonFigure(const std::string &outputPath) const
{
    const bool hasTopStallReason = std::any_of(topStallReasonByCore_.begin(), topStallReasonByCore_.end(),
                                               [](uint64_t value) { return value != 0; });
    if (!hasTopStallReason) {
        return;
    }
    Utility::Visualize::WriteBin<Utility::VisualizeType::TOP_STALL_REASON>(outputPath,
                                                                           GenTopStallReasonFigureJson());
}

bool HotSpotFunctionGenerator::ProcessEncoding(const std::string &kernelPath,
                                               const std::shared_ptr<L2Cache> &l2CachePtr)
{
    std::vector<EncodingInfo> instrEncodingVec;
    ChipProductType chipType = GetProductTypeBySocVersion(socVersion_);
    if (!InstrEncoding::GetInstance().Init(chipType)) {
        return false;
    }
    if (!InstrEncoding::GetInstance().GenerateEncoding(kernelPath, instrEncodingVec)) {
        Utility::LogDebug("Failed to generate encoding");
        return false;
    }

    for (size_t i = 0; i < instrEncodingVec.size(); ++i) {
        uint64_t pc = instrEncodingVec[i].pc;
        if (!IsObjKernelPcAddr(pc)) {
            continue;
        }
        Encoding newEncoding = {pc, instrEncodingVec[i].pipe, instrEncodingVec[i].name, "", 0, 0,
                                0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0}};
        if ((IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND910B_SERIES) ||
            IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND910_93_SERIES)) &&
            l2CachePtr != nullptr) {
            auto cacheMetrics = l2CachePtr->GetPcBasedCacheData();
            if (cacheMetrics.find(pc) != cacheMetrics.end()) {
                newEncoding.l2cacheHit = cacheMetrics[pc].hit;
                newEncoding.l2cacheMiss = cacheMetrics[pc].miss;
            }
        }
        encodings_[pc] = newEncoding;
    }
    return true;
}

bool HotSpotFunctionGenerator::CheckBBBMapPermission(const std::string &bbbmapPath) const
{
    if (!Utility::IsReadable(bbbmapPath)) {
        Utility::LogDebug("bbbmap is not readable");
        return false;
    }
    if (!Utility::IsWritable(bbbmapPath)) {
        Utility::LogDebug("bbbmap is not writable");
        return false;
    }
    return true;
}

size_t HotSpotFunctionGenerator::MatchBbbId(const std::regex &blockIdPattern, const uint64_t startAddr,
                                            size_t id, std::vector<std::string> &bbbmap, const std::string &kernelName)
{
    std::smatch matches;
    while (id < bbbmap.size() && std::regex_search(bbbmap[id], matches, blockIdPattern)) {
        uint64_t beginAddr;
        uint64_t endAddr;
        std::string beginAddrStr = matches[2];
        std::string endAddrStr = matches[3];
        if (!Utility::StoullConverter(beginAddrStr, beginAddr, RADIX_16) ||
            !Utility::StoullConverter(endAddrStr, endAddr, RADIX_16)) {
            id++;
            continue;
        }
        auto endPos = bbbmap[id].find(" end");
        bbbmap[id].erase(endPos);
        id++;
        if (IsObjKernel(kernelName)) {
            beginAddr += startAddr;
            endAddr += startAddr;
            bbCalls_[beginAddr] = {endAddr, 0};
            beginAddr_[kernelName] = std::min(beginAddr_[kernelName], beginAddr);
            endAddr_[kernelName] = std::max(endAddr_[kernelName], endAddr);
        }
    }
    return id;
}

bool HotSpotFunctionGenerator::UpdateBBBMap(const std::string &bbbmapPath)
{
    if (!CheckBBBMapPermission(bbbmapPath)) {
        return false;
    }

    std::vector<std::string> bbbmap(0);
    Utility::ReadFileSaveStringVec(bbbmapPath, bbbmap);

    std::smatch matches;
    const std::regex funcPattern("function: (\\w+)");
    const std::regex addrPattern("start addr: 0x([0-9a-fA-F]+)");
    const std::regex blockIdPattern("BBB id:([0-9]+) Offset: 0x([0-9a-fA-F]+) end: 0x([0-9a-fA-F]+)");
    for (size_t j = 0, id = 0; j < bbbmap.size(); j = id + 1) {
        id = j;
        if (!regex_search(bbbmap[id], matches, funcPattern)) {
            continue;
        }
        std::string kernelName = matches[1];
        size_t parseBlockId = id + 2;
        if (parseBlockId >= bbbmap.size() || !regex_search(bbbmap[id + 1], matches, addrPattern)) {
            continue;
        }
        uint64_t startAddr;
        std::string startAddrStr = matches[1];
        if (!Utility::StoullConverter(startAddrStr, startAddr, RADIX_16)) {
            continue;
        }
        kernelStartAddr_[kernelName] = startAddr;
        if (!regex_search(bbbmap[parseBlockId], matches, blockIdPattern)) {
            continue;
        }
        id = MatchBbbId(blockIdPattern, startAddr, parseBlockId, bbbmap, kernelName);
    }
    Utility::WriteWithoutAppend(bbbmapPath, bbbmap);
    return true;
}

bool HotSpotFunctionGenerator::UpdateExtra(const std::string &extraPath) const
{
    if (!Utility::IsReadable(extraPath)) {
        Utility::LogDebug("extra is not readable");
        return false;
    }
    std::string data;
    Utility::ReadFileSaveString(extraPath, data);

    if (!Utility::IsWritable(extraPath)) {
        Utility::LogDebug("extra is not writable");
        return false;
    }

    const auto *dataPtr = reinterpret_cast<const uint32_t*>(data.data());
    std::ofstream ofs(extraPath, std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) {
        Utility::LogDebug("Can not open [%s]", extraPath.c_str());
        return false;
    }
    for (size_t id = 0; id < data.size() / sizeof(uint32_t); id++) {
        ofs << static_cast<uint32_t>(dataPtr[id]) << "\n";
    }
    ofs.close();
    chmod(extraPath.c_str(), Utility::SAVE_DATA_FILE_AUTHORITY);
    return true;
}

bool HotSpotFunctionGenerator::GenFdata(const std::string &bbmapPath, const std::string &extraPath,
                                        const std::string &fdataPath)
{
    std::vector<std::string> cmd = {
        "bisheng-tune", "--action=analyze-profile",
        "--bbbmap=" + bbmapPath, extraPath, "-o=" + fdataPath
    };
    std::string output;
    if (!Utility::CmdExecute(cmd, {}, output)) {
        Utility::LogDebug("bisheng-tune failed");
        return false;
    }
    // 设置fdata的权限为 640
    if (chmod(fdataPath.c_str(), 0640) != 0) {
        Utility::LogDebug("Failed to set permissions for %s", fdataPath.c_str());
        return false;
    }
    if (!Utility::IsReadable(fdataPath)) {
        Utility::LogDebug("fdata is not readable");
        return false;
    }
    std::vector<std::string> fdata(0);
    Utility::ReadFileSaveStringVec(fdataPath, fdata);
    if (!GenBBCalls(fdata)) {
        Utility::LogDebug("Failed to gen BBCalls");
        return false;
    }
    return true;
}

bool HotSpotFunctionGenerator::GenTlvdata(const std::string &kernelPath, const std::string &tlvdataPath)
{
    // bisheng-tune  path/to/object  --lrm=count --lrm-output=path/to/out/xx.bin
    std::string lrmPath = Utility::JoinPath({tlvdataPath, Common::TLV_DATA});
    std::vector<std::string> cmd = {
        "bisheng-tune", kernelPath,
        "--lrm=count", "--lrm-output=" + lrmPath
    };
    std::string output;
    if (!Utility::CmdExecute(cmd, {}, output)) {
        Utility::LogDebug("bisheng-tune failed");
        return false;
    }
     // 设置 lrm.bin 的权限为 640
    if (chmod(lrmPath.c_str(), 0640) != 0) {
        Utility::LogDebug("Failed to set permissions for %s", lrmPath.c_str());
        return false;
    }
    TlvParser parser(lrmPath);
    if (!parser.ParseStream()) {
        Utility::LogDebug("Failed to parse TLvdata");
        return false;
    }
    if (!parser.CountPCNum() || parser.GetPcNumCount().empty()) {
        Utility::LogDebug("Failed to count gpr num");
        return false;
    }
    // 将统计好的相关数据写入到encoding中
    // 判断标志位，如果判断出为非simt算子，则不显示寄存器列
    bool sign = false;
    for (auto &item : parser.GetPcNumCount()) {
        uint64_t pc = item.first;
        if (encodings_.find(pc) == encodings_.end()) {
            Utility::LogDebug("PC %llu not found in encodings", pc);
            continue;
        }
        encodings_[pc].gprCount = item.second;
        const auto& gprStatusMap = parser.GetGprStatus();
        auto it = gprStatusMap.find(pc);
        if (it != gprStatusMap.end()) {
            encodings_[pc].gprStatus = it->second;
        }
        sign = item.second > 0 ? true : sign;
    }
    if (!sign) {
        Utility::LogInfo("This operator is a non-SIMT operator and does not require generating register information");
        skipKeys_.insert("GPR Count");
    }
    return true;
}

bool HotSpotFunctionGenerator::GenBBCalls(const std::vector<std::string> &fData)
{
    std::smatch matches;
    const std::regex fdataPattern("\\b(\\d+)\\s+(\\w+)\\s+([0-9a-fA-F]+)\\s+(\\d+)\\b");
    for (size_t j = 0, id = 0; j < fData.size(); j = id + 1) {
        id = j;
        if (!regex_search(fData[id], matches, fdataPattern)) {
            continue;
        }
        std::string kernelName = matches[2];
        if (!IsObjKernel(kernelName)) {
            continue;
        }
        uint64_t startAddr{0};
        if (kernelStartAddr_.find(kernelName) == kernelStartAddr_.end()) {
            Utility::LogWarn("Failed to get kernel start addr");
        } else {
            startAddr = kernelStartAddr_[kernelName];
        }
        while (id < fData.size() && regex_search(fData[id], matches, fdataPattern)) {
            std::string curKernelName = matches[2];
            if (curKernelName != kernelName) {
                id = id > 0 ? id - 1 : id;
                break;
            }
            uint64_t blockBeginAddr;
            std::string blockBeginAddrStr = matches[3];
            if (!Utility::StoullConverter(blockBeginAddrStr, blockBeginAddr, RADIX_16)) {
                continue;
            }
            blockBeginAddr += startAddr;
            uint64_t calls;
            std::string callStr = matches[4];
            if (!Utility::StoullConverter(callStr, calls)) {
                continue;
            }
            bbCalls_[blockBeginAddr].second += calls;
            id++;
        }
    }
    return true;
}

bool HotSpotFunctionGenerator::GenLine2Encodings(const std::string &kernelPath,
                                                 std::map<std::string, std::vector<Encoding>> &line2Encodings)
{
    if (!Utility::IsReadable(kernelPath)) {
        Utility::LogDebug("aicore_binary.o is not readable");
        return false;
    }

    std::vector<std::string> addrVec(0);
    ChipProductType chipType = GetProductTypeBySocVersion(socVersion_);
    if (IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND910_93_SERIES) ||
        IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND910B_SERIES)) {
        for (const auto &item : bbCalls_) {
            uint64_t begin = item.first;
            uint64_t end = item.second.first;
            for (uint64_t addr = begin; addr <= end; addr += INSTR_SIZE) {
                addrVec.emplace_back(std::to_string(addr));
            }
        }
    } else if (IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND950_SERIES)) {
        for (const auto &item : encodings_) {
            addrVec.emplace_back(std::to_string(item.second.addr));
        }
    }
    Utility::LogDebug("Addr vec size is: %lu, bb calls size: %lu", addrVec.size(), bbCalls_.size());

    std::unordered_map<uint64_t, std::vector<std::string>> addr2Lines{};
    if (!GenAddr2Lines(kernelPath, addrVec, addr2Lines)) {
        return false;
    }

    for (const auto &item: encodings_) {
        // get code according to pc
        for (const auto &line: addr2Lines[item.first]) {
            line2Encodings[line].emplace_back(item.second);
        }
    }
    return true;
}

bool HotSpotFunctionGenerator::GenCodeLine(const std::string &line, const std::vector<Encoding> &encodings,
                                           CodeLine &codeLine) const
{
    if (!Utility::StoiConverter(line, codeLine.line)) {
        Utility::LogDebug("Failed to convert line [%s] str to int", line.c_str());
        return false;
    }
    uint32_t l2cacheHit = 0;
    uint32_t l2cacheMiss = 0;
    std::set<uint64_t> addrSet;
    for (const auto &encoding : encodings) {
        if (addrSet.insert(encoding.addr + startPc_).second) {
            l2cacheHit += encoding.l2cacheHit;
            l2cacheMiss += encoding.l2cacheMiss;
            codeLine.processBytes += encoding.processBytes;
            codeLine.gprCount = codeLine.gprCount > encoding.gprCount ? codeLine.gprCount : encoding.gprCount;
            for (size_t i = 0; i < encoding.pcSampling.size(); ++i) {
                codeLine.pcSampling[i] += encoding.pcSampling[i];
            }
        }
    }
    codeLine.l2cacheHitRate = -1;
    if (l2cacheHit + l2cacheMiss != 0) {
        codeLine.l2cacheHitRate = PERCENTAGE * static_cast<float>(l2cacheHit) /
                static_cast<float>(l2cacheHit + l2cacheMiss);
    }

    // 计算 CodeLine 的 pcSampling 百分比
    uint64_t linePcSamplingNotIssue = 0;
    uint64_t linePcSamplingAllSample = 0;
    AccumulatePcSampling(codeLine.pcSampling, linePcSamplingNotIssue, linePcSamplingAllSample);

    codeLine.samplingPercentNotIssue = 0.0f;
    codeLine.samplingPercentAllSample = 0.0f;
    if (totalPcSamplingNotIssue_ > 0) {
        codeLine.samplingPercentNotIssue = static_cast<float>(linePcSamplingNotIssue) /
                static_cast<float>(totalPcSamplingNotIssue_);
    }
    if (totalPcSamplingAllSample_ > 0) {
        codeLine.samplingPercentAllSample = static_cast<float>(linePcSamplingAllSample) /
                static_cast<float>(totalPcSamplingAllSample_);
    }

    std::vector<std::vector<std::string>> addrRange = MergeAddrRange(addrSet);
    if (addrRange.empty()) {
        Utility::LogDebug("Failed to MergeAddrRange");
        return false;
    }
    codeLine.addrRange = addrRange;
    return true;
}

bool HotSpotFunctionGenerator::GenCodeFiles(const std::string &outputPath,
                                            const std::map<std::string, std::vector<Encoding>> &line2Encodings,
                                            std::vector<CodeFile> &codeFiles)
{
    std::map<std::string, std::map<std::string, CodeLine>> file2CodeLines;
    for (const auto &line2Encoding : line2Encodings) {
        std::vector<std::string> fileLines;
        SplitString(line2Encoding.first, ':', fileLines);
        std::string fileName = fileLines.front();
        const std::string &line = fileLines.back();

        static constexpr size_t INPUT_CPP_FILE_MAX_SIZE = 10 * 1024 * 1024;
        auto fileContent = CheckAndReadFile(fileName, "cpp", INPUT_CPP_FILE_MAX_SIZE, "", Utility::LogLv::DEBUG);
        if (fileContent.empty()) {
            Utility::LogDebug("codeFile: %s, CheckInputFileValid failed, skip this file", fileName.c_str());
            continue;
        }
        Utility::Visualize::WriteBin<Utility::VisualizeType::CODE>(outputPath, fileContent, fileName);
        if (file2CodeLines[fileName].find(line) == file2CodeLines[fileName].end()) {
            file2CodeLines[fileName][line].pcSampling = {0, 0, 0, 0, 0, 0, 0, 0, 0};
        }
        if (!GenCodeLine(line, line2Encoding.second, file2CodeLines[fileName][line])) {
            return false;
        }
    }

    for (const auto &file2CodeLine: file2CodeLines) {
        CodeFile codeFile;
        codeFile.file = file2CodeLine.first;
        for (const auto &codeLines: file2CodeLine.second) {
            codeFile.lines.emplace_back(codeLines.second);
            codeFile.lines.back().callCount.emplace_back(fileLineCalls_[codeFile.file][codeLines.second.line]);
        }
        codeFiles.emplace_back(codeFile);
    }
    return true;
}

bool HotSpotFunctionGenerator::GenAddr2Lines(const std::string &kernelPath, const std::vector<std::string>& addrVec,
                                             std::unordered_map<uint64_t, std::vector<std::string>> &addr2Lines)
{
    SymbolizerParser symbolPar;
    if (!symbolPar.Parse(kernelPath, addrVec)) {
        Utility::LogDebug("Failed to Symbolizer parse");
        return false;
    }
    std::unordered_map<uint64_t, std::set<std::string>> pcWithCode;
    uint64_t pcNotInbbbMap {UINT64_MAX};
    for (const auto& offset2Line : symbolPar.offset2Lines_) {
        if (offset2Line.file.find("ascend-toolkit") != std::string::npos) {
            continue;
        }
        uint64_t addr;
        if (!Utility::StoullConverter(offset2Line.offset, addr, RADIX_16)) {
            Utility::LogDebug("Failed to covert addr string to uint64, addr is %s", offset2Line.offset.c_str());
            return false;
        }
        if (encodings_.find(addr) == encodings_.end()) {
            Utility::LogDebug("Failed to get instr calls, addr: %llu", addr);
            return false;
        }
        std::string line = offset2Line.file + ":" + std::to_string(offset2Line.line);
        if (addr2Lines.count(addr) == 0) {
            encodings_[addr].cceCode = line;
        }
        if (pcWithCode[addr].find(line) != pcWithCode[addr].end()) {
            continue;
        }
        addr2Lines[addr].emplace_back(line);
        pcWithCode[addr].insert(line);
        if (!sourceEnable_) {
            continue;
        }
        auto bigger = bbCalls_.upper_bound(addr);
        uint64_t call = 0;
        if ((bigger == bbCalls_.begin()) || (addr > std::prev(bbCalls_.end())->second.first + INSTR_SIZE)) {
            if (addr < pcNotInbbbMap) {
                pcNotInbbbMap = addr;
            }
        } else {
            call = std::prev(bigger)->second.second;
        }
        encodings_[addr].calls = call;
        fileLineCalls_[offset2Line.file][offset2Line.line] += call;
    }
    if (pcNotInbbbMap != UINT64_MAX) {
        Utility::LogDebug("Failed to find upper bound, addr: %x", pcNotInbbbMap);
    }
    return true;
}

void HotSpotFunctionGenerator::AccumulatePcSampling(const std::vector<uint64_t> &pcSampling,
                                                    uint64_t &notIssue,
                                                    uint64_t &allSample) const
{
    for (size_t i = 0; i < pcSampling.size() && i < PC_SAMPLING_STATUS_NUM; ++i) {
        allSample += pcSampling[i];
        if (i < PC_SAMPLING_STATUS_NUM - 1) {
            notIssue += pcSampling[i];
        }
    }
}

bool HotSpotFunctionGenerator::GenInstrInfos(std::vector<InstrInfo> &instrInfos)
{
    if (encodings_.empty()) {
        Utility::LogDebug("Encoding is empty, can not gen InstrInfo");
        return false;
    }

    for (const auto &item : encodings_) {
        const Encoding &instr = item.second;
        // 计算当前指令的 pcSampling 总和
        uint64_t instrPcSamplingSumNotIssue = 0;
        uint64_t instrPcSamplingSumAllSample = 0;
        AccumulatePcSampling(instr.pcSampling, instrPcSamplingSumNotIssue, instrPcSamplingSumAllSample);

        // 计算百分比
        float samplingNotIssuePercent = 0.0f;
        float samplingAllSamplePercent = 0.0f;
        if (totalPcSamplingNotIssue_ > 0) {
            samplingNotIssuePercent = static_cast<float>(instrPcSamplingSumNotIssue) /
                    static_cast<float>(totalPcSamplingNotIssue_);
        }
        if (totalPcSamplingAllSample_ > 0) {
            samplingAllSamplePercent = static_cast<float>(instrPcSamplingSumAllSample) /
                    static_cast<float>(totalPcSamplingAllSample_);
        }

        InstrInfo info;
        info.addr = Utility::NumToHexString(instr.addr + startPc_, ADDR_SIZE);
        info.cceCode = instr.cceCode;
        info.instr = instr.source;
        info.pipe = instr.pipe;
        info.callCount = {static_cast<int>(instr.calls)};
        info.samplingPercentNotIssue = samplingNotIssuePercent;
        info.samplingPercentAllSample = samplingAllSamplePercent;
        info.processBytes = instr.processBytes;
        info.gprCount = instr.gprCount;
        info.gprStatus = instr.gprStatus;
        info.pcSampling = instr.pcSampling;
        if (instr.l2cacheHit + instr.l2cacheMiss != 0) {
            info.l2cacheHitRate = PERCENTAGE * static_cast<float>(instr.l2cacheHit) /
                    static_cast<float>(instr.l2cacheHit + instr.l2cacheMiss);
        }
        instrInfos.emplace_back(info);
    }
    std::sort(instrInfos.begin(), instrInfos.end(), [](const InstrInfo &a, const InstrInfo &b) { return a.addr < b.addr; });
    return true;
}

void HotSpotFunctionGenerator::SetFileDtype(nlohmann::json &apiJson) const
{
    FileDtype fileDtype;
    nlohmann::json fileJson;
    fileJson["Address Range"] = fileDtype.addrRange;
    fileJson["Line"] = fileDtype.line;
    if (pcSamplingEnable_) {
        fileJson["Stall Sampling(All Samples)"] = fileDtype.pcSampling;
        fileJson["Stall Sampling(Not Issue)"] = fileDtype.pcSampling;
    }
    ChipProductType chipType = GetProductTypeBySocVersion(socVersion_);
    if (IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND910B_SERIES) ||
        IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND910_93_SERIES)) {
        fileJson["L2Cache Hit Rate"] = fileDtype.l2cacheHitRate;
    }
    if (memdetailEnable_) {
        fileJson["Process Bytes"] = fileDtype.processBytes;
    }
    fileJson["Instructions Executed"] = fileDtype.instructionExecuted;
    if (skipKeys_.find("GPR Count") == skipKeys_.end()) {
        fileJson["GPR Count"] = fileDtype.gprCount;
    }
    nlohmann::json fileDtypeJson;
    fileDtypeJson["Lines"] = fileJson;
    apiJson["Files Dtype"] = fileDtypeJson;
}

void HotSpotFunctionGenerator::SetInstrDtype(nlohmann::json &apiJson) const
{
    InstructionsDtype instructionsDtype;
    nlohmann::json instrJson;
    instrJson["Address"] = instructionsDtype.address;
    instrJson["AscendC Inner Code"] = instructionsDtype.ascendCInnerCode;
    if (pcSamplingEnable_) {
        instrJson["Stall Sampling(All Samples)"] = instructionsDtype.pcSampling;
        instrJson["Stall Sampling(Not Issue)"] = instructionsDtype.pcSampling;
    }
    ChipProductType chipType = GetProductTypeBySocVersion(socVersion_);
    if (IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND910B_SERIES) ||
        IsChipSeriesTypeValid(chipType, ChipProductType::ASCEND910_93_SERIES)) {
        instrJson["L2Cache Hit Rate"] = instructionsDtype.l2cacheHitRate;
    }
    if (memdetailEnable_) {
        instrJson["Process Bytes"] = instructionsDtype.processBytes;
    }
    if (skipKeys_.find("GPR Count") == skipKeys_.end()) {
        instrJson["GPR Count"] = instructionsDtype.gprCount;
        instrJson["GPR Status"] = instructionsDtype.gprStatus;
    }
    instrJson["Instructions Executed"] = instructionsDtype.instructionExecuted;
    instrJson["Pipe"] = instructionsDtype.pipe;
    instrJson["Source"] = instructionsDtype.source;
    nlohmann::json instrDTypeJson;
    instrDTypeJson["Instructions"] = instrJson;
    apiJson["Instructions Dtype"] = instrDTypeJson;
}

void HotSpotFunctionGenerator::GenVisualizeData(const std::string &outputPath, const std::vector<CodeFile>& codeFiles,
                                                const std::vector<InstrInfo>& instrInfos) const
{
    nlohmann::json apiFileJson; // for bin file writing
    apiFileJson["Cores"] = {"ALL"};
    SetFileDtype(apiFileJson);
    std::vector<nlohmann::json> fileJsonList;
    for (const CodeFile &file: codeFiles) {
        nlohmann::json fileJson;
        fileJson["Source"] = file.file;
        std::vector<nlohmann::json> linesJson;
        for (CodeLine line: file.lines) {
            nlohmann::json lineDetails;
            line.ToJson(socVersion_, lineDetails);
            linesJson.emplace_back(lineDetails);
        }
        fileJson["Lines"] = linesJson;
        fileJsonList.emplace_back(fileJson);
    }
    apiFileJson["Files"] = fileJsonList;
    Utility::Visualize::WriteBin<Utility::VisualizeType::FILE_API>(outputPath, apiFileJson);

    nlohmann::json apiInstrJson; // for bin file writing
    apiInstrJson["Cores"] = {"ALL"};
    SetInstrDtype(apiInstrJson);
    std::vector<nlohmann::json> instrJsonList;
    for (const InstrInfo& instrInfo: instrInfos) {
        if (instrInfo.pipe.empty()) continue;
        nlohmann::json instrDetails;
        instrInfo.ToJson(socVersion_, instrDetails);
        instrJsonList.emplace_back(instrDetails);
    }
    apiInstrJson["Instructions"] = instrJsonList;
    Utility::Visualize::WriteBin<Utility::VisualizeType::INSTR_API>(outputPath, apiInstrJson);
}

}
