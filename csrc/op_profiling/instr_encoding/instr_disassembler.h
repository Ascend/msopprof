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
#include <string>
#include <unordered_map>

#include "instr_encoding.h"

namespace Encode {

/*
 * @brief 使用CANN自带的llvm-objdump对算子.o反汇编，得到 指令编码 -> 指令+细节 的映射。
 *
 * 只有CANN自带的反汇编系统才能正确反汇编算子.o，因此本类会优先查找CANN包下的
 * bisheng_compiler llvm-objdump。key为归一化后的指令binary，value为反汇编出的指令+细节。
 */
class InstrDisassembler {
public:
    InstrDisassembler() = default;
    ~InstrDisassembler() = default;

    /*
     * @brief 反汇编算子.o并生成 binary -> 指令+细节 的映射
     *
     * @param kernelPath 算子.o的路径
     * @return 是否成功生成映射
     */
    bool Init(const std::string &kernelPath);
    void Clear();

    /*
     * @brief 按归一化后的binary查找反汇编指令+细节
     *
     * @param binary 归一化后的指令编码，如 "073a7fa0" 或 "ffffffff000000010e0e0010088e005d"
     * @return 反汇编出的指令+细节；未命中返回空串
     */
    std::string GetDisassembly(const std::string &binary) const;

    bool IsValid() const
    {
        return !binary2Disasm_.empty();
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

    /*
     * @brief 把指令编码解析出的encoding值转换成反汇编map使用的binary字符串。
     *
     * 上板路径复用InstrEncoding解出的encoding(Enc128)，转换为与objdump输出一致、
     * 可作为binary2Disasm_ key的归一化字符串。
     *
     * @param low64 低64位编码
     * @param high64 高64位编码
     * @param bitType 指令位宽
     * @return 归一化后的binary字符串；非法位宽返回空串
     */
    static std::string EncodingToBinary(uint64_t low64, uint64_t high64, EncodingType bitType);

private:
    static std::string GetObjdumpPath();
    static bool ParseObjdumpLine(const std::string &line, std::string &binary, std::string &disasm);

    std::unordered_map<std::string, std::string> binary2Disasm_;
};

}

#endif // MSOPT_INSTR_DISASSEMBLER_H
