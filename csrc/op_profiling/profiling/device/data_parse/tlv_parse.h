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

#ifndef TLV_PARSER_H
#define TLV_PARSER_H

#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include "json.hpp"

// 统一TLV长度类型（修改长度类型时仅需改此处）
using TlvLengthType = uint64_t;

// 操作类型枚举（uint16_t底层类型）
enum OperationType : uint16_t {
    Space = 0,
    Read = 1,
    Write = 2,
    ReadAndWrite = 3,
    InUsed = 4,
};

// 寄存器种类枚举（uint32_t底层类型）
enum RegType : uint32_t {
    R = 0,
    S = 1,
    P = 2,
    X = 3,
};

struct RecordStatus {
    inline void ToJson(nlohmann::json &jsonData) const
    {
        jsonData["regIndex"] = this->regIndex;
        jsonData["regStatus"] = this->operation;
        jsonData["survivalTime"] = this->survivalTime;
    }
    std::string regIndex;
    OperationType operation;
    uint32_t survivalTime;
};

struct RegLength {
    uint64_t beginAddr;
    uint64_t lastIndex;
    uint64_t survivalTime;
};

// 寄存器记录结构体（强制1字节对齐，避免内存填充）
struct RecordValue {
    uint16_t regIndex;
    OperationType operation;
};

// 单条PC记录结构体
struct InstRecord {
    uint64_t address;
    std::vector<RecordValue> regRecord; // 该PC的寄存器记录（空表示无操作）
};

// 顶层解析结果结构体
struct RegLiveness {
    std::vector<RegType> regType;                     // 寄存器种类列表（如{r, s}）
    std::vector<uint64_t> regNumber;                  // 对应种类的寄存器总数
    uint64_t instNum;                                 // PC总数
    uint64_t functionNameNum;                         // 函数总数
    std::vector<std::string> functionNameList;        // 函数名称列表
    std::vector<int64_t> functionPositionStartIdx;    // 函数PC起始索引列表
    std::vector<std::vector<InstRecord>> instRecordList; // 按寄存器种类分组的PC记录（外层=种类数，内层=PC数）
};

// TLV标签枚举（定义在TLVHeader之前，确保类型可用）
enum class TlvTag : uint32_t {
    RegLivenessTag = 0,          // 顶层标签
    RegTypeTag = 1,              // 寄存器种类列表
    RegNumberTag = 2,            // 寄存器总数列表
    InstNumTag = 3,              // PC数量
    RFunctionNameNumTag = 4,     // 函数名称数量
    FunctionNameListTag = 5,     // 函数名称列表容器
    FunctionNameTag = 6,         // 单个函数名称
    FunctionPositionStartIdxTag = 7, // 函数PC起始索引列表
    InstRecordListTag = 8,       // PC记录总容器
    InstRecordListByRegTypeTag = 9, // 按寄存器种类分组的PC记录容器
    InstRecordTag = 10,           // 单条PC记录容器
    addressTag = 11,              // PC地址
    InstRecordRegTag = 12         // PC对应的寄存器记录列表
};

// TLV头部结构体（使用统一长度类型）
struct TLVHeader {
    TlvTag tag;               // 标签
    TlvLengthType length;     // Value字段长度（统一类型）
};

// TLV解析器类
class TlvParser {
public:
    TlvParser();
    // 构造函数：初始化文件路径
    explicit TlvParser(std::string tlvPath) : parsedRegLiveness_({}), tlvPath_(tlvPath) {}
    ~TlvParser() = default;

    // 禁用拷贝构造和赋值（避免浅拷贝问题）
    TlvParser(const TlvParser&) = delete;
    TlvParser& operator=(const TlvParser&) = delete;

    // 公共接口：解析TLV二进制流
    bool ParseStream();

    // 公共接口：获取解析后的完整数据
    const RegLiveness& GetParsedData() const;

    // 公共接口：统计每个PC使用的所有寄存器总数（跨种类累加）
    bool CountPCNum();

    // 公共接口：获取PC寄存器统计结果
    const std::unordered_map<uint64_t, uint64_t>& GetPcNumCount() const { return pcNumCount_; }

    // 公共接口：获取寄存器状态
    const std::unordered_map<uint64_t, std::vector<RecordStatus>>& GetGprStatus() const { return gprStatus_; }

    // 公共接口：获取寄存器存活时间
    const std::unordered_map<std::string, RegLength>& GetRegSurvivalTime() const { return regSurvivalTime_; }

private:
    // 辅助函数：处理单个寄存器记录，更新寄存器状态和存活时间
    void ProcessRegisterRecord(const std::string& regType, const RecordValue& gpr, uint64_t pcAddr, size_t pcIndex);
    
    // 辅助函数：处理单个PC记录组
    void ProcessPcGroup(const std::vector<InstRecord>& pcGroup, const std::string& regType);

    // 通用函数：读取数据
    template <typename T>
    bool ReadData(const std::vector<uint8_t>& data, size_t& offset, size_t containerEnd, T& outData) const;

    // 通用函数：校验长度是否为目标类型大小的整数倍
    template <typename T>
    bool ValidateLengthMultiple(TlvLengthType length, const std::string& tagName) const;

    // 通用函数：读取TLV头部（Tag+Length），统一校验越界
    bool ReadTlvHeader(const std::vector<uint8_t>& data, size_t& offset, size_t containerEnd,
                       TlvTag& outTag, TlvLengthType& outLength) const;

    // 校验函数：所有PC地址全局一致（同一PC索引地址相同）
    bool ValidatePcAddressConsistency() const;

    // 检验函数：所有子Tag读取完后，当前偏移量与预期是否相同
    bool ValidateDataReadOver(std::string tlvTagName, size_t& offset, size_t containerEnd) const;

    // 工具函数：读取TLV字符串（按长度直接读取）
    std::string ReadTlvString(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType strLength) const;

    // 解析函数：按TLV标签分层解析
    bool ParseRegLivenessTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length);
    bool ParseRegTypeTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length);
    bool ParseRegNumberTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length);
    bool ParseInstNumTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length);
    bool ParseFunctionNameNumTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length);
    bool ParseFunctionNameListTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length);
    bool ParseFunctionNameTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length);
    bool ParseFunctionPositionStartIdxTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length);
    bool ParseInstRecordListTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length);
    bool ParseInstRecordListByRegTypeTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length,
                                         std::vector<InstRecord>& instRecords);
    bool ParseInstRecordTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length,
                            InstRecord& instRecord);
    bool ParseAddressTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length, uint64_t& address);
    bool ParseInstRecordRegTag(const std::vector<uint8_t>& data, size_t& offset, TlvLengthType length,
                               std::vector<RecordValue>& regRecords);

    // 解析结果存储
    RegLiveness parsedRegLiveness_;

    std::string tlvPath_;
    
    // 私有成员：存储PC寄存器统计结果（key=PC地址，value=总寄存器数）
    std::unordered_map<uint64_t, uint64_t> pcNumCount_;

    std::unordered_map<uint64_t, std::vector<RecordStatus>> gprStatus_;

    std::unordered_map<std::string, RegLength> regSurvivalTime_;

    std::string GetRegType(const RegType& type);

    void UpdateRegStatus(uint64_t pcAddr, const std::string& regName, uint64_t length);
    
    // 校验标签有效性
    static constexpr bool IsValidTag(TlvTag tag)
    {
        return static_cast<uint32_t>(tag) >= 0 && static_cast<uint32_t>(tag) <= 12;
    }
};

#endif // TLV_PARSER_H