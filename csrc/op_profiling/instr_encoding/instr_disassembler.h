/* -------------------------------------------------------------------------
 *  This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#ifndef MSOPT_INSTR_DISASSEMBLER_H
#define MSOPT_INSTR_DISASSEMBLER_H

#include <cstdint>
#include <array>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>

#include "instr_encoding.h"

namespace Encode {

/*
 * @brief 使用CANN自带的llvm-objdump对算子.o反汇编，得到 指令编码 -> 指令+细节 的映射。
 *
 * 只有CANN自带的反汇编系统才能正确反汇编算子.o，因此本类会优先查找CANN包下的
 * bisheng_compiler llvm-objdump。key为四段32位指令编码，按位宽分别建立索引，value为反汇编出的指令+细节。
 */
class InstrDisassembler {
public:
    InstrDisassembler() = default;
    ~InstrDisassembler();

    /*
     * @brief 反汇编算子.o并生成 binary -> 指令+细节 的映射
     *
     * 每个实例只针对一个算子调用一次。
     * @param kernelPath 算子.o的路径
     * @return 是否成功生成映射
     */
    bool Init(const std::string &kernelPath);

    /*
     * @brief 按指定指令位宽查找反汇编指令+细节，并记录未直接命中的PC
     *
     * @param binary 从低位到高位保存的四段32位编码
     * @param type 匹配位宽，32/64位查询忽略未参与匹配的高位字段
     * @param pc 指令地址，用于汇总未直接命中的位置
     * 同位宽未命中时，首次构建更宽指令的低位索引并复用；候选不唯一时返回空串。
     * @return 反汇编出的指令+细节；未命中返回空串
     */
    std::string GetDisassembly(const InstrBinary &binary, EncodingType type, uint64_t pc) const;
    // Print and drain the deduplicated PCs after all lookup workers have finished.
    void LogUnmatchedPcs() const;

    // Parse log binary once; split SIMT tokens are ordered low64 then high64.
    static InstrBinary ParseBinary(const std::string &rawBinary);
    // Zero unused words to keep equality and hashing consistent.
    static InstrBinary FromEncoding(uint64_t low64, uint64_t high64, EncodingType type);

    bool IsValid() const
    {
        for (const auto &index : binary2Disasm_) {
            if (!index.empty()) {
                return true;
            }
        }
        return false;
    }

    /*
     * @brief 归一化日志中的binary字段：去掉0x前缀，去掉空格。
     *        SIMT指令日志中128位binary会拆成两段(如 "0x06060000000e011d 0000000100000001")，
     *        真实binary需要反序拼接为 "000000010000000106060000000e011d"。
     *
     * @param rawBinary 日志中的原始binary字符串
     * @return 归一化后的binary字符串(统一小写)
     */
    static std::string NormalizeBinary(const std::string &rawBinary);

private:
    static std::string GetObjdumpPath();
    static InstrBinary TruncateBinary(const InstrBinary &binary, EncodingType type);
    void BuildTruncatedIndex() const;
    std::string FindDisassembly(const InstrBinary &binary, EncodingType type, bool &directMatch) const;
    static bool ParseObjdumpLine(const std::string &line, InstrBinary &binary, std::string &disasm);

    // Separate widths prevent zero-extended encodings from aliasing.
    using BinaryMap = std::unordered_map<InstrBinary, std::string, InstrBinaryHash>;
    std::array<BinaryMap, 3> binary2Disasm_;
    // References into the immutable full-width maps; nullptr marks an ambiguous prefix.
    using TruncatedMap = std::unordered_map<InstrBinary, const std::string *, InstrBinaryHash>;
    mutable std::array<TruncatedMap, 2> truncated2Disasm_;
    mutable std::mutex truncatedIndexMutex_;
    mutable bool truncatedIndexBuilt_ = false;
    mutable std::mutex unmatchedPcsMutex_;
    mutable std::set<uint64_t> unmatchedPcs_;
};

}

#endif // MSOPT_INSTR_DISASSEMBLER_H
