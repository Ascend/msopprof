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


#ifndef MSOPT_INSTR_ENCODING__H
#define MSOPT_INSTR_ENCODING__H
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <algorithm>
#include <vector>
#include "common/defs.h"
#include "encodingA2/DMAInstrTableA2.h"
#include "encodingA2/FCInstrTableA2.h"
#include "encodingA2/FCInstrTable2A2.h"
#include "encodingA2/SIMDInstrTableA2.h"
#include "encodingA2/ScalarInstrTableA2.h"
#include "encodingA2/OtherInstrTableA2.h"
#include "encodingA2/CubeInstrTableA2.h"
#include "encodingA5/DMAInstrTableA5.h"
#include "encodingA5/FCInstrTableA5.h"
#include "encodingA5/SIMDInstrTableA5.h"
#include "encodingA5/ScalarInstrTableA5.h"
#include "encodingA5/VectorInstrTableA5.h"
#include "encodingA5/CubeInstrTableA5.h"
#include "memory"

namespace Encode {
constexpr int PIPE_ID_MASK = 0x00003c00;

struct InstrTable {
    std::string pipe;
    std::unordered_map<uint32_t, std::string> table;
};

struct InstrTable64 {
    std::string pipe;
    std::unordered_map<uint64_t, std::string> table;
};

struct InstrTable128 {
    std::string pipe;
    std::unordered_map<Common::Enc128, std::string, Common::Enc128Hash> table;
};

enum class EncodingType {
    BIT32 = 0,
    BIT64,
    BIT128,
    INVALID,
};


struct EncodingInfo {
    Common::Enc128 enc;
    uint64_t pc;
    EncodingType bitType;  // 指令encoding位数
    std::string pipe;
    std::string name;
};

/**
    * @brief mask到具体指令映射表的映射。
    *
    * 通过遍历该映射表，对指令进行mask操作，之后从vector中匹配正确的指令信息,
    *
    */
const std::unordered_map<uint32_t, std::vector<InstrTable>> MASK2GROUPS_A2 = {
    {0xffc0000f, {{Common::SU_PIPE, INSTR0_TABLE0_A2}}},
    {0xffc0007f, {{Common::SU_PIPE, INSTR1_TABLE0_A2}, {Common::SIMD_PIPE, INSTR1_TABLE1_A2}}},
    {0xffc00000, {{Common::SU_PIPE, INSTR2_TABLE0_A2}, {Common::SU_PIPE, INSTR2_TABLE1_A2},
                  {Common::SU_PIPE, INSTR2_TABLE2_A2}, {Common::SU_PIPE, INSTR2_TABLE3_A2},
                  {Common::MTE_PIPE, INSTR2_TABLE4_A2}, {Common::MTE_PIPE, INSTR2_TABLE5_A2},
                  {Common::MTE_PIPE, INSTR2_TABLE6_A2}, {Common::SIMD_PIPE, INSTR2_TABLE7_A2}}},
    {0xffc00070, {{Common::SU_PIPE, INSTR3_TABLE0_A2}}},
    {0xffc00f80, {{Common::SU_PIPE, INSTR4_TABLE0_A2}}},
    {0xff000f00, {{Common::SU_PIPE, INSTR5_TABLE0_A2}}},
    {0xff000f9f, {{Common::SU_PIPE, INSTR6_TABLE0_A2}}},
    {0xffc00fc0, {{Common::SU_PIPE, INSTR7_TABLE0_A2}}},
    {0xff000f80, {{Common::SU_PIPE, INSTR8_TABLE0_A2}}},
    {0xff000000, {{Common::SIMD_PIPE, INSTR9_TABLE0_A2}, {Common::SIMD_PIPE, INSTR9_TABLE1_A2},
                  {Common::FIXP_PIPE, INSTR9_TABLE2_A2}}},
    {0xff010000, {{Common::SU_PIPE, INSTR10_TABLE0_A2}}},
    {0xffe00000, {{Common::FC_PIPE, INSTR11_TABLE0_A2}}},
    {0xfffc0000, {{Common::FC_PIPE, INSTR12_TABLE0_A2}, {Common::FC_PIPE, INSTR12_TABLE1_A2}}},
    {0xffe1f000, {{Common::FC_PIPE, INSTR13_TABLE0_A2}}},
    {0xfffe0000, {{Common::FC_PIPE, INSTR14_TABLE0_A2}}},
    {0xffe07f80, {{Common::FC_PIPE, INSTR15_TABLE0_A2}}},
    {0xffe03c00, {{Common::FC_PIPE, INSTR16_TABLE0_A2}}},
    {0xfff47fe0, {{Common::FC_PIPE, INSTR17_TABLE0_A2}}},
    {0xfff00000, {{Common::FC_PIPE, INSTR18_TABLE0_A2}, {Common::FC_PIPE, INSTR18_TABLE1_A2}}},
    {0xffc00013, {{Common::MTE_PIPE, INSTR19_TABLE0_A2}}},
    {0xffc00005, {{Common::MTE_PIPE, INSTR20_TABLE0_A2}}},
    {0xfe1c0000, {{Common::FC_PIPE, INSTR21_TABLE0_A2}}},
    {0xffc00001, {{Common::SU_PIPE, INSTR22_TABLE0_A2}, {Common::MTE_PIPE, INSTR22_TABLE1_A2}}},
    {0xff800002, {{Common::MTE_PIPE, INSTR23_TABLE0_A2}, {Common::MTE_PIPE, INSTR23_TABLE1_A2}}},
    {0xff400000, {{Common::MTE_PIPE, INSTR24_TABLE0_A2}}},
    {0xff400003, {{Common::MTE_PIPE, INSTR25_TABLE0_A2}}},
    {0xff800078, {{Common::MTE_PIPE, INSTR26_TABLE0_A2}}},
    {0xff800000, {{Common::MTE_PIPE, INSTR27_TABLE0_A2}}},
    {0xffc00007, {{Common::MTE_PIPE, INSTR28_TABLE0_A2}, {Common::SIMD_PIPE, INSTR28_TABLE1_A2},
                  {Common::SIMD_PIPE, INSTR28_TABLE2_A2}}},
    {0xfe000f81, {{Common::SIMD_PIPE, INSTR29_TABLE0_A2}, {Common::SIMD_PIPE, INSTR29_TABLE1_A2}}},
    {0xfe000f80, {{Common::SIMD_PIPE, INSTR30_TABLE0_A2}}},
    {0xfe000003, {{Common::SIMD_PIPE, INSTR31_TABLE0_A2}, {Common::SIMD_PIPE, INSTR31_TABLE1_A2},
                  {Common::SIMD_PIPE, INSTR31_TABLE2_A2}, {Common::SIMD_PIPE, INSTR31_TABLE3_A2}}},
    {0xfe000001, {{Common::SIMD_PIPE, INSTR32_TABLE0_A2}, {Common::SIMD_PIPE, INSTR32_TABLE1_A2},
                  {Common::SIMD_PIPE, INSTR32_TABLE2_A2}}},
    {0xff000002, {{Common::SIMD_PIPE, INSTR33_TABLE0_A2}, {Common::SIMD_PIPE, INSTR33_TABLE1_A2}}},
    {0xfe000000, {{Common::SIMD_PIPE, INSTR34_TABLE0_A2}}},
    {0xff000001, {{Common::SIMD_PIPE, INSTR35_TABLE0_A2}, {Common::SIMD_PIPE, INSTR35_TABLE1_A2},
                  {Common::SIMD_PIPE, INSTR35_TABLE2_A2}}},
    {0xfe000002, {{Common::SIMD_PIPE, INSTR36_TABLE0_A2}}},
    {0xff000003, {{Common::SIMD_PIPE, INSTR37_TABLE0_A2}}},
    {0xff800003, {{Common::SIMD_PIPE, INSTR38_TABLE0_A2}}},
    {0xfec00003, {{Common::SIMD_PIPE, INSTR39_TABLE0_A2}}},
    {0xfe400003, {{Common::SIMD_PIPE, INSTR40_TABLE0_A2}}},
    {0xefc00001, {{Common::CUBE_PIPE, INSTR41_TABLE0_A2}}},
};

const std::unordered_map<uint32_t, std::vector<InstrTable>> MASK2GROUPS_A5 = {
    {0xffc0000f, {{Common::SU_PIPE, INSTR0_TABLE0_A5}}},
    {0xffc0007f, {{Common::SU_PIPE, INSTR1_TABLE0_A5}}},
    {0xffc00000, {{Common::SU_PIPE, INSTR2_TABLE0_A5}, {Common::MTE_PIPE, INSTR2_TABLE1_A5}}},
    {0xffc00070, {{Common::SU_PIPE, INSTR3_TABLE0_A5}}},
    {0xffc00f80, {{Common::SU_PIPE, INSTR4_TABLE0_A5}}},
    {0xff000f00, {{Common::SU_PIPE, INSTR5_TABLE0_A5}}},
    {0xff000f9f, {{Common::SU_PIPE, INSTR6_TABLE0_A5}}},
    {0xffc00fc0, {{Common::SU_PIPE, INSTR7_TABLE0_A5}}},
    {0xff000f80, {{Common::SU_PIPE, INSTR8_TABLE0_A5}}},
    {0xff000000, {{Common::FIXP_PIPE, INSTR9_TABLE0_A5}, {Common::VEC_PIPE, INSTR9_TABLE1_A5}}},
    {0xff010000, {{Common::SU_PIPE, INSTR10_TABLE0_A5}}},
    {0xffe20000, {{Common::FC_PIPE, INSTR11_TABLE0_A5}}},
    {0xfffc0000, {{Common::FC_PIPE, INSTR12_TABLE0_A5}}},
    {0xffe1f000, {{Common::FC_PIPE, INSTR13_TABLE0_A5}}},
    {0xfffe0000, {{Common::FC_PIPE, INSTR14_TABLE0_A5}}},
    {0xffe27f80, {{Common::FC_PIPE, INSTR15_TABLE0_A5}}},
    {0xffe03c00, {{Common::FC_PIPE, INSTR16_TABLE0_A5}}},
    {0xfff47fe0, {{Common::FC_PIPE, INSTR17_TABLE0_A5}}},
    {0xfff00000, {{Common::FC_PIPE, INSTR18_TABLE0_A5}}},
    {0xffc00002, {{Common::MTE_PIPE, INSTR19_TABLE0_A5}, {Common::CUBE_PIPE, INSTR19_TABLE1_A5}}},
    {0xffc00003, {{Common::MTE_PIPE, INSTR20_TABLE0_A5}, {Common::CUBE_PIPE, INSTR20_TABLE1_A5}}},
    {0xfe1f0000, {{Common::FC_PIPE, INSTR21_TABLE0_A5}}},
    {0xffc00001, {{Common::SU_PIPE, INSTR22_TABLE0_A5}, {Common::MTE_PIPE, INSTR22_TABLE1_A5}}},
    {0xffe23c00, {{Common::FC_PIPE, INSTR23_TABLE0_A5}}},
    {0xfff83c00, {{Common::FC_PIPE, INSTR24_TABLE0_A5}}},
    {0xfffc3c00, {{Common::FC_PIPE, INSTR25_TABLE0_A5}}},
    {0xf8000000, {{Common::FC_PIPE, INSTR26_TABLE0_A5}}},
    {0xffe00000, {{Common::FC_PIPE, INSTR27_TABLE0_A5}, {Common::VEC_PIPE, INSTR27_TABLE1_A5}}},
    {0xffc00007, {{Common::MTE_PIPE, INSTR28_TABLE0_A5}}},
    {0xc000001f, {{Common::SIMD_PIPE, INSTR29_TABLE0_A5}}},
    {0xc000007f, {{Common::SIMD_PIPE, INSTR30_TABLE0_A5}}},
    {0xc000003f, {{Common::SIMD_PIPE, INSTR31_TABLE0_A5}}},
    {0xc000013f, {{Common::SIMD_PIPE, INSTR32_TABLE0_A5}}},
    {0xc00000c3, {{Common::SIMD_PIPE, INSTR33_TABLE0_A5}}},
    {0xc00000fb, {{Common::SIMD_PIPE, INSTR34_TABLE0_A5}}},
    {0xc00000ff, {{Common::SIMD_PIPE, INSTR35_TABLE0_A5}}},
    {0xc00000e3, {{Common::SIMD_PIPE, INSTR36_TABLE0_A5}}},
    {0xff000003, {{Common::FIXP_PIPE, INSTR37_TABLE0_A5}}},
    {0xc0000043, {{Common::SIMD_PIPE, INSTR38_TABLE0_A5}}},
    {0xc000007b, {{Common::SIMD_PIPE, INSTR39_TABLE0_A5}}},
    {0xc00000f3, {{Common::SIMD_PIPE, INSTR40_TABLE0_A5}}},
    {0xc0080073, {{Common::SIMD_PIPE, INSTR41_TABLE0_A5}}},
    {0xfec00000, {{Common::SU_PIPE, INSTR42_TABLE0_A5}}},
    {0xfec00001, {{Common::SU_PIPE, INSTR43_TABLE0_A5}}},
    {0xfff8801f, {{Common::VEC_PIPE, INSTR44_TABLE0_A5}}},
    {0xffe0801f, {{Common::VEC_PIPE, INSTR45_TABLE0_A5}}},
    {0xffe0001f, {{Common::VEC_PIPE, INSTR46_TABLE0_A5}}},
    {0xffe0003f, {{Common::VEC_PIPE, INSTR47_TABLE0_A5}}},
    {0xc008007f, {{Common::SIMD_PIPE, INSTR48_TABLE0_A5}}},
    {0xc00c007f, {{Common::SIMD_PIPE, INSTR49_TABLE0_A5}}},
    {0xc00c0078, {{Common::SIMD_PIPE, INSTR50_TABLE0_A5}}},
    {0xc0000078, {{Common::SIMD_PIPE, INSTR51_TABLE0_A5}}},
    {0xc000006f, {{Common::SIMD_PIPE, INSTR52_TABLE0_A5}}},
    {0xc000017f, {{Common::SIMD_PIPE, INSTR53_TABLE0_A5}}},
    {0xc00c207f, {{Common::SIMD_PIPE, INSTR54_TABLE0_A5}}},
    {0xff800078, {{Common::MTE_PIPE, INSTR55_TABLE0_A5}}},
};

const std::unordered_map<uint64_t, std::vector<InstrTable64>> MASK2GROUPS_A5_64BIT = {
    {0xffe0001fffe00000, {{Common::VEC_PIPE, INSTR_VEC_TABLE0_A5}}},
    {0x00000000003e0001, {{Common::VEC_PIPE, INSTR_VEC_TABLE1_A5}}},
    {0x000000001c3e0001, {{Common::VEC_PIPE, INSTR_VEC_TABLE2_A5}}},
    {0x00000000001e0001, {{Common::VEC_PIPE, INSTR_VEC_TABLE5_A5}}},
    {0x00000000007e0001, {{Common::VEC_PIPE, INSTR_VEC_TABLE7_A5}}},
    {0x0000000003fe0001, {{Common::VEC_PIPE, INSTR_VEC_TABLE9_A5}}},
    {0x0000000000fe0001, {{Common::VEC_PIPE, INSTR_VEC_TABLE10_A5}}},
    {0x000000001ffe0001, {{Common::VEC_PIPE, INSTR_VEC_TABLE13_A5}}},
    {0x0000000007fe0001, {{Common::VEC_PIPE, INSTR_VEC_TABLE16_A5}}},
    {0x000000003ffe0001, {{Common::VEC_PIPE, INSTR_VEC_TABLE17_A5}}},
};

const std::unordered_map<Common::Enc128, std::vector<InstrTable128>, Common::Enc128Hash> MASK2GROUPS_A5_128BIT = {
    // low64 high64
    {{0x00000000003e0001, 0x0000000000000001}, {{Common::VEC_PIPE, INSTR_VEC_TABLE3_A5}}},
    {{0x00000000001e0001, 0x0000000000000001}, {{Common::VEC_PIPE, INSTR_VEC_TABLE4_A5}}},
    {{0x00000000007e0001, 0x0000000000000001}, {{Common::VEC_PIPE, INSTR_VEC_TABLE6_A5}}},
    {{0x0000000003fe0001, 0x0000000000000001}, {{Common::VEC_PIPE, INSTR_VEC_TABLE8_A5}}},
    {{0x0000000000fe0001, 0x0000000000000001}, {{Common::VEC_PIPE, INSTR_VEC_TABLE11_A5}}},
    {{0x000000001ffe0001, 0x0000000000000001}, {{Common::VEC_PIPE, INSTR_VEC_TABLE12_A5}}},
    {{0x0000000007fe0001, 0x0000000000000001}, {{Common::VEC_PIPE, INSTR_VEC_TABLE14_A5}}},
    {{0x000000003ffe0001, 0x0000000000000001}, {{Common::VEC_PIPE, INSTR_VEC_TABLE18_A5}}},
};

const std::unordered_set<uint32_t> instrWithPipeIDTable0 = {
    0x40a00000,
    0x40c00000,
    0x40e00000,
    0x41000000,
    0x41e00000,
    0x42000000,
    0x42200000,
    0x42600000,
    0x42800000,
    0x42c00000,
};

const std::unordered_set<uint32_t> instrWithPipeIDTable1 = {
    0x41e00000,
    0x41e00020,
    0x41e00040,
    0x41e00060,
};

/**
 * @class InstrEncoding
 * @brief 解析算子文件，得到指令编码，指令名称，pipe信息
 *
 * InstrEncoding类用于表示和处理与特定硬件或虚拟机相关的编码指令。
 */
class InstrEncoding {
public:
    /**
     * @brief 获取InstrEncoding类的实例。
     *
     * @return 返回一个InstrEncoding类的实例指针。
     */

    static InstrEncoding& GetInstance()
    {
        static auto instance = InstrEncoding();
        return instance;
    }

    /**
     * @brief 初始化 instrEncoding实例
     *
     * @param: socVersion芯片类型
     */

    bool Init(ChipProductType socVersion);

    /**
     * 获取算子文件中指令的十六进制编码以及对应指令名称。
     *
     * @kernelPath 算子文件的路径。
     *
     * @res 一个包含字符串对的vector，用于存储生成指令的结果,first:指令十六进制编码,second:指令名称。
     *
     * @return 如果成功生成指令，返回true；否则，返回false。
     */

    bool GenerateEncoding(const std::string &kernelPath, std::vector<EncodingInfo> &res);

    const InstrEncoding& operator=(const InstrEncoding&) = delete;

private:
    InstrEncoding(){};
    ~InstrEncoding() = default;
    InstrEncoding(const InstrEncoding &instrEncoding) = default;
    void UpdateEncoding32(uint32_t encoding, std::string &pipe, std::string &name) const;
    void UpdateEncoding64(uint64_t encoding, std::string &pipe, std::string &name) const;
    void UpdateEncoding128(Common::Enc128 encoding, std::string &pipe, std::string &name) const;
    void InstrUpdate(std::string &pipe, uint32_t encoding) const;
    std::string GetSpecialInstrPipeByEncoding(uint32_t encoding) const;
    bool GetTextEncodingLines(const std::string &kernelPath, const std::string &pcDump,
                              std::vector<EncodingInfo> &encInfo);
    bool GetEncodingPc(const std::string &kernelPath, std::string &pcDump);
    bool ExtractPc(std::string str, uint64_t &pc);
    bool GetEncodingByPc(std::ifstream &binaryFile, const std::string &pcDump,
                                    std::vector<EncodingInfo> &encInfo);
    void BinRead(std::ifstream &binaryFile,  uint64_t *encode, EncodingType &type, uint32_t bitLen);

    template<class keyType, class InstrTableType>
    std::vector<std::pair<keyType, std::vector<InstrTableType>>> MaskOrder(
        const std::unordered_map<keyType, std::vector<InstrTableType>> &rMap)
    {
        std::vector<std::pair<keyType, std::vector<InstrTableType>>> vec(rMap.begin(), rMap.end());
        std::sort(vec.begin(), vec.end(), [](const std::pair<keyType, std::vector<InstrTableType>>& a,
                                             const std::pair<keyType, std::vector<InstrTableType>>& b) {
            const size_t size = sizeof(keyType) * 8;
            std::bitset<size> bitsetA(a.first);
            std::bitset<size> bitsetB(b.first);
            size_t countA = bitsetA.count();
            size_t countB = bitsetB.count();
            if (countA != countB) {
                return countA > countB;
            }
            return a.first > b.first;
        });
        return vec;
    }

    std::string lastKernelPath_;
    std::vector<std::pair<uint32_t, std::vector<InstrTable>>> mask2Groups_;
    std::vector<std::pair<uint64_t, std::vector<InstrTable64>>> mask2Groups64Bit_;
    std::unordered_map<Common::Enc128, std::vector<InstrTable128>, Common::Enc128Hash> mask2Groups128Bit_;

    /**
     * @brief mask到指令集的映射表。
     *
     * 特殊指令的pipe信息需要更多的位来进行判断，map.second记录特殊指令mask过后的指令编码,map.first记录判断特殊指令的mask值
     *
     */
    const std::unordered_map<uint32_t, std::unordered_set<uint32_t>> mask2InstrWithPipeTable_ = {
        {0xffe00000, instrWithPipeIDTable0},
        {0xfff40060, instrWithPipeIDTable1}
    };

    /**
     * @brief 特殊指令pipe映射表。
     *
     * 特殊指令通过指令第11位到第14位进行特判
     *
     */
    std::unordered_map<uint32_t, std::string> pipeIDTable_ = {
        {0x00000000, "SCALAR"},
        {0x00000400, "VECTOR"},
        {0x00000800, "CUBE"},
        {0x00000c00, "MTE1"},
        {0x00001000, "MTE2"},
        {0x00001400, "MTE3"},
        {0x00001800, "ALL"},
        {0x00001c00, "MTE4"},
        {0x00002000, "MTE5"},
        {0x00002800, "FIXP"}
    };
};
}

#endif // MSOPT_INSTR_ENCODING__H
