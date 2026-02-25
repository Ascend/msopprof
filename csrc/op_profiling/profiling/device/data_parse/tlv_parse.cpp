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

#include <cstring>
#include <fstream>
#include <iostream>
#include "log.h"
#include "tlv_parse.h"

using namespace Utility;
// 通用函数：数据读取
template <typename T>
bool TlvParser::ReadData(const std::vector<uint8_t>& data, size_t& offset, size_t containerEnd, T& outData) const
{
    if (offset + sizeof(T) > containerEnd) {
        Utility::LogDebug("ReadData: Data out of bounds, offset=%zu, bytes to read=%zu, remaining bytes=%zu", offset, sizeof(T), containerEnd - offset);
        return false;
    }
    if (memcpy_s(&outData, sizeof(T), data.data() + offset, sizeof(T)) != EOK) {
        Utility::LogWarn("ReadData: memcpy_s failed");
        return false;
    }
    offset += sizeof(T);
    return true;
}

// 通用函数：校验长度是否为目标类型大小的整数倍
template <typename T>
bool TlvParser::ValidateLengthMultiple(TlvLengthType length, const std::string& tagName) const
{
    if (sizeof(T) == 0) {
        Utility::LogDebug("ValidateLengthMultiple: %d The divisor cannot be zero", sizeof(T));
        return false;
    }
    if (length % sizeof(T) != 0) {
        Utility::LogDebug("ValidateLengthMultiple: %s length is not a multiple of %d bytes, length=%d", tagName.c_str(), sizeof(T), length);
        return false;
    }
    return true;
}

// 通用函数：读取TLV头部（Tag+Length）
bool TlvParser::ReadTlvHeader(const std::vector<uint8_t>& data, size_t& offset, size_t containerEnd,
                              TlvTag& outTag, TlvLengthType& outLength) const
{
    bool result = ReadData(data, offset, containerEnd, outTag);
    result = ReadData(data, offset, containerEnd, outLength);
    // 校验Value长度是否超过容器剩余空间
    if (!result || (offset + outLength > containerEnd)) {
        Utility::LogWarn("ReadTlvHeader: TLV Value length exceeds container bounds, declared length=%zu, remaining space=%zu ", outLength, containerEnd - offset);
        return false;
    }
    return true;
}

bool TlvParser::ValidatePcAddressConsistency() const
{
    const auto& instRecordList = parsedRegLiveness_.instRecordList;
    if (instRecordList.empty()) {
        Utility::LogDebug("ParseInstRecordList is empty");
        return false;
    }
    size_t regTypeCount = instRecordList.size();
    uint64_t totalPcCount = parsedRegLiveness_.instNum;
    for (size_t pcIdx = 0; pcIdx < totalPcCount; ++pcIdx) {
        uint64_t targetAddr = instRecordList[0][pcIdx].address;
        for (size_t typeIdx = 1; typeIdx < regTypeCount; ++typeIdx) {
            uint64_t currentAddr = instRecordList[typeIdx][pcIdx].address;
            if (currentAddr != targetAddr) {
                Utility::LogDebug("ParseInstRecordListTag: PC address mismatch, pcIdx=%zu, typeIdx=%zu, currentAddr=%llx, targetAddr=%llx", pcIdx, typeIdx, currentAddr, targetAddr);
                return false;
            }
        }
    }
    return true;
}

bool TlvParser::ValidateDataReadOver(std::string tlvTagName, size_t& offset, size_t containerEnd) const
{
    if (offset != containerEnd) {
        Utility::LogWarn("TLV parsing warning: %s parsing incomplete, expected offset=%zu, actual offset=%zu", tlvTagName.c_str(), containerEnd, offset);
        return false;
    }
    return true;
}

std::string TlvParser::ReadTlvString(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType strLength) const
{
    const size_t containerEnd = offset + strLength;
    if (containerEnd > data.size()) {
        Utility::LogDebug("ReadTlvString: String read out of bounds, offset=%zu, strLength= %llx, data size=%zu",
                          offset, strLength, data.size());
        return "";
    }
    std::string str(reinterpret_cast<const char*>(data.data() + offset), strLength);
    offset += strLength;
    return str;
}

bool TlvParser::ParseStream()
{
    // 清空历史解析结果和统计结果
    parsedRegLiveness_ = RegLiveness{};

    // 读取落盘文件内容
    std::ifstream tlvFile(tlvPath_, std::ios::binary | std::ios::ate);
    if (!tlvFile.is_open()) {
        Utility::LogWarn("Failed to open lrm file: %s ", tlvPath_);
        return false;
    }
    std::streamsize fileSize = tlvFile.tellg();
    tlvFile.seekg(0, std::ios::beg);
    std::vector<uint8_t> tlvData(fileSize);
    if (!tlvFile.read(reinterpret_cast<char *>(tlvData.data()), fileSize)) {
        Utility::LogWarn("Failed to read lrm file: %s ", tlvPath_);
        return false;
    }
    tlvFile.close();
    if (tlvData.empty()) {
        Utility::LogWarn("inputData is null: %s ", tlvPath_);
        return false;
    }
    size_t offset = 0;
    const size_t dataSize = tlvData.size();

    // 读取顶层TLV头部
    TlvTag topTag;
    TlvLengthType topLength;
    if (!ReadTlvHeader(tlvData, offset, dataSize, topTag, topLength)) {
        Utility::LogWarn("ParseStream: Top-level TLV header data is insufficient");
        return false;
    }

    // 校验顶层标签有效性
    if (topTag != TlvTag::RegLivenessTag) {
        Utility::LogDebug("ParseStream: Invalid top-level tag:%04x", static_cast<uint32_t>(topTag));
        return false;
    }
    // 解析顶层标签对应的Value
    if (!ParseRegLivenessTag(tlvData, offset, topLength) || !ValidateDataReadOver("RegLivenessTag", offset, dataSize)) {
        return false;
    }
    return true;
}

bool TlvParser::ParseRegLivenessTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length)
{
    const size_t containerEnd = offset + length;

    while (offset < containerEnd) {
        TlvTag subTag;
        TlvLengthType subLength;
        if (!ReadTlvHeader(data, offset, containerEnd, subTag, subLength)) {
            Utility::LogDebug("ParseRegLivenessTag: SubTag header data is insufficient, offset=%zu", offset);
            return false;
        }
        bool result = true;
        // 按子Tag类型分发解析
        switch (subTag) {
            case TlvTag::RegTypeTag:
                result = ParseRegTypeTag(data, offset, subLength);
                break;
            case TlvTag::RegNumberTag:
                result = ParseRegNumberTag(data, offset, subLength);
                break;
            case TlvTag::InstNumTag:
                result = ParseInstNumTag(data, offset, subLength);
                break;
            case TlvTag::RFunctionNameNumTag:
                result = ParseFunctionNameNumTag(data, offset, subLength);
                break;
            case TlvTag::FunctionNameListTag:
                result = ParseFunctionNameListTag(data, offset, subLength);
                break;
            case TlvTag::FunctionPositionStartIdxTag:
                result = ParseFunctionPositionStartIdxTag(data, offset, subLength);
                break;
            case TlvTag::InstRecordListTag:
                result = ParseInstRecordListTag(data, offset, subLength);
                break;
            default:
                Utility::LogWarn("TLV parsing warning: Unknown subTag, value%04x, offset=%zu", static_cast<uint32_t>(subTag), offset);
                return false;
        }
        if (!result) {
            return false;
        }
    }
    return true;
}

bool TlvParser::ParseRegTypeTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length)
{
    const size_t containerEnd = offset + length;
    auto& regTypeList = parsedRegLiveness_.regType;
    regTypeList.clear();
    if (!ValidateLengthMultiple<RegType>(length, "RegTypeTag")) {
        return false;
    }
    const size_t itemCount = length / sizeof(RegType);
    for (size_t i = 0; i < itemCount; ++i) {
        RegType regType;
        if (!ReadData(data, offset, containerEnd, regType)) {
            return false;
        }
        regTypeList.push_back(regType);
    }

    // 校验是否读取完全
    if (!ValidateDataReadOver("RegTypeTag", offset, containerEnd)) {
        return false;
    }
    return true;
}

bool TlvParser::ParseRegNumberTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length)
{
    const size_t containerEnd = offset + length;
    auto& regNumberList = parsedRegLiveness_.regNumber;
    regNumberList.clear();
    if (!ValidateLengthMultiple<uint64_t>(length, "RegNumberTag")) {
        return false;
    }
    const size_t itemCount = length / sizeof(uint64_t);
    for (size_t i = 0; i < itemCount; ++i) {
        uint64_t regNum;
        if (!ReadData(data, offset, containerEnd, regNum)) {
            return false;
        }
        regNumberList.push_back(regNum);
    }

    // 校验：寄存器种类数与总数列表数一致
    if (regNumberList.size() != parsedRegLiveness_.regType.size()) {
        Utility::LogDebug("ParseRegNumberTag: Number of total registers mismatch with types, total registers=%zu, types=%zu", regNumberList.size(), parsedRegLiveness_.regType.size());
        return false;
    }
    if (!ValidateDataReadOver("RegNumberTag", offset, containerEnd)) {
        return false;
    }
    return true;
}

bool TlvParser::ParseInstNumTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length)
{
    const size_t containerEnd = offset + length;
    if (length != sizeof(uint64_t)) {
        Utility::LogDebug("ParseInstNumTag: Invalid length, expected=%zu bytes, actual=%zu bytes", sizeof(uint64_t), length);
        return false;
    }
    if (!ReadData(data, offset, containerEnd, parsedRegLiveness_.instNum)) {
        return false;
    }
    if (!ValidateDataReadOver("InstNumTag", offset, containerEnd)) {
        return false;
    }
    return true;
}

bool TlvParser::ParseFunctionNameNumTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length)
{
    const size_t containerEnd = offset + length;
    if (length != sizeof(uint64_t)) {
        Utility::LogDebug("ParseFunctionNameNumTag: Invalid length, expected=%zu bytes, actual=%zu bytes", sizeof(uint64_t), length);
        return false;
    }
    if (!ReadData(data, offset, containerEnd, parsedRegLiveness_.functionNameNum)) {
        return false;
    }
    if (!ValidateDataReadOver("FunctionNameNumTag", offset, containerEnd)) {
        return false;
    }
    return true;
}

bool TlvParser::ParseFunctionNameListTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length)
{
    const size_t containerEnd = offset + length;
    auto& funcNameList = parsedRegLiveness_.functionNameList;
    funcNameList.clear();
    const uint64_t expectedCount = parsedRegLiveness_.functionNameNum;
    while (offset < containerEnd) {
        TlvTag subTag;
        TlvLengthType subLength;
        if (!ReadTlvHeader(data, offset, containerEnd, subTag, subLength)) {
            Utility::LogDebug("ParseFunctionNameListTag: SubTag header data is insufficient, offset=%zu", offset);
            return false;
        }
        if (subTag != TlvTag::FunctionNameTag) {
            Utility::LogDebug("ParseFunctionNameListTag: Invalid subTag=%04x, offset=%zu", static_cast<uint32_t>(subTag), offset);
            return false;
        }
        if (!ParseFunctionNameTag(data, offset, subLength)) {
            return false;
        }
    }

    // 校验函数名数量与预期一致
    if (funcNameList.size() != expectedCount) {
        Utility::LogDebug("ParseFunctionNameListTag: Function name count mismatch, actual=%zu, expected=%zu", funcNameList.size(), expectedCount);
        return false;
    }
    if (!ValidateDataReadOver("FunctionNameListTag", offset, containerEnd)) {
        return false;
    }
    return true;
}

bool TlvParser::ParseFunctionNameTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length)
{
    const size_t containerEnd = offset + length;
    if (length == 0) {
        Utility::LogDebug("ParseFunctionNameTag: Function name string length is 0");
        return false;
    }
    std::string funcName = ReadTlvString(data, offset, length);
    parsedRegLiveness_.functionNameList.push_back(std::move(funcName));
    if (!ValidateDataReadOver("FunctionNameTag", offset, containerEnd)) {
        return false;
    }
    return true;
}

bool TlvParser::ParseFunctionPositionStartIdxTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length)
{
    const size_t containerEnd = offset + length;
    auto& funcPosList = parsedRegLiveness_.functionPositionStartIdx;
    funcPosList.clear();
    if (!ValidateLengthMultiple<int64_t>(length, "FunctionPositionStartIdxTag")) {
        return false;
    }
    const size_t itemCount = length / sizeof(int64_t);
    for (size_t i = 0; i < itemCount; ++i) {
        int64_t idx;
        if (!ReadData(data, offset, containerEnd, idx)) {
            return false;
        }
        funcPosList.push_back(idx);
    }

    // 校验：函数起始索引数与函数总数一致
    if (funcPosList.size() != parsedRegLiveness_.functionNameNum) {
        Utility::LogDebug("ParseFunctionPositionStartIdxTag: Start index count mismatch with function count, index count=%zu, function count=%llu", funcPosList.size(), parsedRegLiveness_.functionNameNum);
        return false;
    }
    if (!ValidateDataReadOver("FunctionPositionStartIdxTag", offset, containerEnd)) {
        return false;
    }
    return true;
}

bool TlvParser::ParseInstRecordListTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length)
{
    const size_t containerEnd = offset + length;
    std::vector<std::vector<InstRecord>>& instRecordList = parsedRegLiveness_.instRecordList;
    instRecordList.clear();

    while (offset < containerEnd) {
        TlvTag subTag;
        TlvLengthType subLength;
        if (!ReadTlvHeader(data, offset, containerEnd, subTag, subLength)) {
            Utility::LogDebug("ParseInstRecordListTag: SubTag header data is insufficient, offset=%zu", offset);
            return false;
        }
        if (subTag != TlvTag::InstRecordListByRegTypeTag) {
            Utility::LogDebug("ParseInstRecordListTag: SubTag must be InstRecordListByRegTypeTag, actual=%04x", static_cast<uint32_t>(subTag));
            return false;
        }
        std::vector<InstRecord> instRecordsByType;
        if (!ParseInstRecordListByRegTypeTag(data, offset, subLength, instRecordsByType)) {
            return false;
        }

        // 校验当前分组的PC记录数是否与总PC数一致
        if (instRecordsByType.size() != parsedRegLiveness_.instNum) {
            Utility::LogDebug("ParseInstRecordListTag: PC record count is not equal instNum, pcRecordCount=%zu, totalPcCount=%llu", instRecordsByType.size(), parsedRegLiveness_.instNum);
            return false;
        }
        instRecordList.emplace_back(std::move(instRecordsByType));
    }

    // 校验分组数是否与寄存器种类数一致
    size_t regTypeCount = parsedRegLiveness_.regType.size();
    if (instRecordList.size() != regTypeCount) {
        Utility::LogDebug("ParseInstRecordListTag: Number of groups does not match number of register types, groupCount=%zu, regTypeCount=%zu", instRecordList.size(), regTypeCount);
        return false;
    }

    // 检查每组pc地址是否相同
    if (!ValidatePcAddressConsistency()) {
        return false;
    }
    if (!ValidateDataReadOver("InstRecordListTag", offset, containerEnd)) {
        return false;
    }
    return true;
}

bool TlvParser::ParseInstRecordListByRegTypeTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length,
                                                std::vector<InstRecord>& instRecords)
{
    const size_t containerEnd = offset + length;
    instRecords.clear();
    const uint64_t totalPcCount = parsedRegLiveness_.instNum;
    while (offset < containerEnd) {
        TlvTag subTag;
        TlvLengthType subLength;
        if (!ReadTlvHeader(data, offset, containerEnd, subTag, subLength)) {
            Utility::LogDebug("ParseInstRecordListByRegTypeTag: SubTag header data is insufficient, offset=%zu", offset);
            return false;
        }
        if (subTag != TlvTag::InstRecordTag) {
            Utility::LogDebug("ParseInstRecordListByRegTypeTag: SubTag must be InstRecordListByRegTypeTag, actual=%04x", static_cast<uint32_t>(subTag));
            return false;
        }
        InstRecord instRecord;
        if (!ParseInstRecordTag(data, offset, subLength, instRecord)) {
            return false;
        }
        instRecords.push_back(std::move(instRecord));
    }

    // 校验：当前分组的PC记录数与总PC数一致
    if (instRecords.size() != totalPcCount) {
        Utility::LogDebug("ParseInstRecordListByRegTypeTag: PC record count mismatch, actual=%zu, total PC count=%zu", instRecords.size(), totalPcCount);
        return false;
    }
    if (!ValidateDataReadOver("InstRecordListByRegTypeTag", offset, containerEnd)) {
        return false;
    }
    return true;
}

bool TlvParser::ParseInstRecordTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length,
                                   InstRecord& instRecord)
{
    const size_t containerEnd = offset + length;
    instRecord = InstRecord{}; // 初始化空记录
    while (offset < containerEnd) {
        TlvTag subTag;
        TlvLengthType subLength;
        if (!ReadTlvHeader(data, offset, containerEnd, subTag, subLength)) {
            Utility::LogDebug("ParseInstRecordTag: SubTag header data is insufficient, offset=%zu", offset);
            return false;
        }
        bool result = true;
        switch (subTag) {
            case TlvTag::addressTag:
                result = ParseAddressTag(data, offset, subLength, instRecord.address);
                break;
            case TlvTag::InstRecordRegTag:
                result = ParseInstRecordRegTag(data, offset, subLength, instRecord.regRecord);
                break;
            default:
                Utility::LogDebug("ParseInstRecordTag: Invalid subTag=%04x, offset=%zu", static_cast<uint32_t>(subTag), offset);
                return false;
        }
        if (!result) {
            return false;
        }
    }
    if (!ValidateDataReadOver("InstRecordTag", offset, containerEnd)) {
        return false;
    }
    return true;
}

bool TlvParser::ParseAddressTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length, uint64_t& address)
{
    const size_t containerEnd = offset + length;

    if (length != sizeof(uint64_t)) {
        Utility::LogDebug("ParseAddressTag: Invalid length, expected=%zu bytes, actual=%zu bytes", sizeof(uint64_t), length);
        return false;
    }
    if (!ReadData(data, offset, containerEnd, address)) {
        return false;
    }
    if (!ValidateDataReadOver("addressTag", offset, containerEnd)) {
        return false;
    }
    return true;
}

// ParseInstRecordRegTag 函数，处理空记录和 4 字节 RecordValue
bool TlvParser::ParseInstRecordRegTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length,
                                      std::vector<RecordValue>& regRecords)
{
    const size_t containerEnd = offset + length;
    regRecords.clear();

    // Length=0 直接返回空记录，不读取任何数据
    if (length == 0) {
        return true;
    }

    // 校验 Length 是 4 字节（RecordValue 大小）的整数倍
    if (!ValidateLengthMultiple<RecordValue>(length, "InstRecordRegTag")) {
        return false;
    }
    const size_t recordCount = length / sizeof(RecordValue);
    for (size_t i = 0; i < recordCount; ++i) {
        // 读取 4 字节的 RecordValue（RegIndex=2字节 + Operation=2字节）
        RecordValue record;
        // 手动读取，确保字节序和大小正确
        if (!ReadData(data, offset, containerEnd, record.regIndex)) {
            return false;
        }
        uint16_t operation;
        if (!ReadData(data, offset, containerEnd, operation)) {
            return false;
        }
        record.operation = static_cast<OperationType>(operation);
        regRecords.push_back(record);
    }

    // 校验是否读取完整
    if (!ValidateDataReadOver("InstRecordRegTag", offset, containerEnd)) {
        return false;
    }
    return true;
}

std::string TlvParser::GetRegType(const RegType& type)
{
    static const std::unordered_map<RegType, std::string> typeMap = {
        {RegType::P, "P"},
        {RegType::X, "X"},
        {RegType::R, "R"},
        {RegType::S, "S"}
    };
    auto it = typeMap.find(type);
    if (it != typeMap.end()) {
        return it->second;
    }
    Utility::LogDebug("GetRegType: Unknown register type, %zu", type);
    return "Unknown";
}

void TlvParser::UpdateRegStatus(uint64_t pcAddr, const std::string& regName, uint64_t length)
{
    if (gprStatus_.find(pcAddr) == gprStatus_.end()) {
        return ;
    }
    for (auto& reg : gprStatus_[pcAddr]) {
        if (reg.regIndex.compare(regName) == 0) {
            reg.survivalTime = length;
        }
    }
}

// 辅助函数：处理单个寄存器记录，更新寄存器状态和存活时间
void TlvParser::ProcessRegisterRecord(const std::string& regType, const RecordValue& gpr, uint64_t pcAddr, size_t pcIndex)
{
    std::string regName = regType + std::to_string(gpr.regIndex);
    
    // 更新寄存器状态
    RecordStatus status = {regName, gpr.operation, 0};
    gprStatus_[pcAddr].emplace_back(status);
    
    // 更新寄存器存活时间
    auto it = regSurvivalTime_.find(regName);
    if (it == regSurvivalTime_.end()) {
        // 首次出现该寄存器
        regSurvivalTime_[regName] = {pcAddr, pcIndex, 1};
    } else {
        RegLength& length = it->second;
        if (length.lastIndex + 1 == pcIndex) {
            // 连续出现，延长存活时间
            length.lastIndex = pcIndex;
            length.survivalTime += 1;
        } else {
            // 不连续，更新之前的存活时间并重新开始计数
            UpdateRegStatus(length.beginAddr, regName, length.survivalTime);
            length = {pcAddr, pcIndex, 1};
        }
    }
}

// 辅助函数：处理单个PC记录组
void TlvParser::ProcessPcGroup(const std::vector<InstRecord>& pcGroup, const std::string& regType)
{
    regSurvivalTime_.clear();
    
    for (size_t pcIndex = 0; pcIndex < pcGroup.size(); ++pcIndex) {
        const auto& pcRecord = pcGroup[pcIndex];
        uint64_t pcAddr = pcRecord.address;
        
        // 更新PC寄存器计数
        pcNumCount_[pcAddr] += pcRecord.regRecord.size();
        
        // 处理该PC下的所有寄存器记录
        for (const auto& gpr : pcRecord.regRecord) {
            ProcessRegisterRecord(regType, gpr, pcAddr, pcIndex);
        }
    }
    
    // 处理该寄存器类型下所有寄存器的最终存活时间
    for (const auto& [regName, length] : regSurvivalTime_) {
        UpdateRegStatus(length.beginAddr, regName, length.survivalTime);
    }
}

// PC寄存器统计函数实现
bool TlvParser::CountPCNum()
{
    // 幂等性设计：每次调用先清空统计结果
    pcNumCount_.clear();
    gprStatus_.clear();
    
    // 校验解析结果有效性
    const auto& instRecordList = parsedRegLiveness_.instRecordList;
    if (instRecordList.empty()) {
        Utility::LogDebug("CountPCNum: No valid data parsed, please call ParseStream() first");
        return false;
    }
    
    // 遍历所有寄存器种类分组
    for (size_t groupIndex = 0; groupIndex < instRecordList.size(); ++groupIndex) {
        const auto& pcGroup = instRecordList[groupIndex];
        std::string regType = GetRegType(parsedRegLiveness_.regType[groupIndex]);
        ProcessPcGroup(pcGroup, regType);
    }
    
    return true;
}

// 获取解析后的RegLiveness结构体
const RegLiveness& TlvParser::GetParsedData() const
{
    return parsedRegLiveness_;
}