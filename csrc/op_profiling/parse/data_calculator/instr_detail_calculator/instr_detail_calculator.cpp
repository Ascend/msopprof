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

#include "instr_detail_calculator.h"

#include "log.h"
#include "number_operation.h"
#include "instr_detail_defs.h"
#include "instr_detail_utils.h"
#include "instr_in_camodel/camodel_dtype_defs.h"
#include "ustring.h"

namespace Profiling {
namespace Parse {

constexpr MemOpInfo INVALID_MEM_OP_INFO = {false, 0, 0, 0, 0, 0, 0, 0};

MemOpInfo ParseLoad2D(const InstrRegDetail &instrRegDetail)
{
    if (!instrRegDetail.instrRegCommon.isValid) {
        return INVALID_MEM_OP_INFO;
    }
    return {true,
            instrRegDetail.instrRegCommon.src,
            instrRegDetail.instrRegCommon.dst,
            1,
            512,
            0,
            Utility::ExtractKBits(instrRegDetail.instrRegCommon.config, 16, 8), // repeat
            Utility::ExtractKBits(instrRegDetail.instrRegCommon.config, 24, 16) // srcStride
    };
}

MemOpInfo ParseLoadImage(const InstrRegDetail &instrRegDetail)
{
    if (!instrRegDetail.instrLoadImage.isValid) {
        return INVALID_MEM_OP_INFO;
    }
    uint64_t horSize = Utility::ExtractKBits(instrRegDetail.instrLoadImage.xs, 0, 13);
    uint64_t verSize = Utility::ExtractKBits(instrRegDetail.instrLoadImage.xs, 16, 13);
    uint64_t lPadSize = Utility::ExtractKBits(instrRegDetail.instrLoadImage.xt, 32, 13);
    uint64_t rPadSize = Utility::ExtractKBits(instrRegDetail.instrLoadImage.xt, 45, 13);
    uint64_t topPadSize = Utility::ExtractKBits(instrRegDetail.instrLoadImage.xt, 16, 8);
    uint64_t botPadSize = Utility::ExtractKBits(instrRegDetail.instrLoadImage.xt, 24, 8);
    return {true,
            instrRegDetail.instrLoadImage.dst,
            (horSize + lPadSize + rPadSize) * (verSize + topPadSize + botPadSize),
            32,
            1,
            1,
            0
    };
}

MemOpInfo ParseLoadSmask(const InstrRegDetail &instrRegDetail)
{
    if (!instrRegDetail.instrRegCommon.isValid) {
        return INVALID_MEM_OP_INFO;
    }
    uint64_t smaskSize = ((((instrRegDetail.instrRegCommon.config & 0x800) >> 4) |
                            (instrRegDetail.instrRegCommon.config & 0x7f)) + 15) / 16 * 32;
    return {true,
            instrRegDetail.instrRegCommon.src,
            instrRegDetail.instrRegCommon.dst,
            1,
            smaskSize,
            0,
            1,
            0
    };
}

MemOpInfo ParseDmaMov(const InstrRegDetail &instrRegDetail)
{
    if (!instrRegDetail.instrRegCommon.isValid) {
        return INVALID_MEM_OP_INFO;
    }
    uint64_t nBurst = Utility::ExtractKBits(instrRegDetail.instrRegCommon.config, 4, 12);
    uint64_t lenBurst = Utility::ExtractKBits(instrRegDetail.instrRegCommon.config, 16, 16);
    uint64_t srcStride = Utility::ExtractKBits(instrRegDetail.instrRegCommon.config, 32, 16);
    return {true,
            instrRegDetail.instrRegCommon.src,
            instrRegDetail.instrRegCommon.dst,
            lenBurst,
            32,
            1,
            nBurst,
            lenBurst + srcStride
    };
}

MemOpInfo ParseDmaMovNd2Nz(const InstrRegDetail &instrRegDetail)
{
    if (!instrRegDetail.instrDmaMovNd2Nz.isValid) {
        return INVALID_MEM_OP_INFO;
    }
    return {true,
            instrRegDetail.instrRegCommon.src,
            instrRegDetail.instrRegCommon.dst,
            Utility::ExtractKBits(instrRegDetail.instrDmaMovNd2Nz.xm, 32, 16), // dValue
            static_cast<uint64_t>(instrRegDetail.instrDmaMovNd2Nz.instrDetailDataType), // data type
            1,
            Utility::ExtractKBits(instrRegDetail.instrDmaMovNd2Nz.xm, 16, 16), // nValue
            Utility::ExtractKBits(instrRegDetail.instrDmaMovNd2Nz.xt, 0, 16) // srcDValue
    };
}

MemOpInfo ParseDmaMovAlign(const InstrRegDetail &instrRegDetail)
{
    if (!instrRegDetail.instrDmaMovAlign.isValid) {
        return INVALID_MEM_OP_INFO;
    }
    uint64_t nBurst = Utility::ExtractKBits(instrRegDetail.instrDmaMovAlign.xm, 4, 12);
    uint64_t lenBurst = Utility::ExtractKBits(instrRegDetail.instrDmaMovAlign.xm, 16, 21);
    uint64_t srcGap = Utility::ExtractKBits(instrRegDetail.instrDmaMovAlign.xt, 0, 32);
    uint64_t blockNum = lenBurst;
    uint64_t repeatStride = blockNum + srcGap;
    if (instrRegDetail.instrDmaMovAlign.instrMemType == InstrMemType::UB) {
        blockNum = CeilByAlignSize(lenBurst);
        repeatStride = blockNum + srcGap * UB_ALIGN_SIZE;
    }

    return {true,
            instrRegDetail.instrRegCommon.src,
            instrRegDetail.instrRegCommon.dst,
            blockNum,
            1,
            1,
            nBurst,
            repeatStride,
    };
}

uint8_t ParseFixPipeQuantBits(uint64_t quantPRE, bool enNZ2ND, bool &int8ChannelMerge, bool &int4ChannelMerge)
{
    constexpr uint8_t bits16 = 16;
    constexpr uint8_t bits8 = 8;
    constexpr uint8_t bits4 = 4;
    const std::unordered_map<uint64_t, uint8_t> QuantPre2BitsMap = {
        // 1:f32->f16; 10/11: s32->f16; 12/13: s32->s16; 16: f32->bf16
        {1,  bits16}, {10, bits16}, {11, bits16}, {12, bits16}, {13, bits16}, {16, bits16},
        // 8/9: s32->s8/u8; 23/24: f32->s8/u8
        {8,  bits8}, {9, bits8}, {23, bits8}, {24, bits8},
        // 21/22: s32->s4; 25/26: f32->s4
        {21, bits4}, {22, bits4}, {25, bits4}, {26, bits4},
    };
    auto it = QuantPre2BitsMap.find(quantPRE);
    if (it == QuantPre2BitsMap.end()) {
        return 32; // f32/s32
    }
    if (it->second == bits8) {
        int8ChannelMerge = !enNZ2ND;
    } else if (it->second == bits4) {
        int4ChannelMerge = !enNZ2ND;
    }
    return it->second;
}

MemOpInfo ParseFixPipe(const InstrRegDetail &instrRegDetail)
{
    constexpr uint64_t bitsEachByte = 8;
    constexpr uint64_t blockSize16B = 16;
    constexpr uint64_t blockSize32B = 32;
    constexpr uint64_t even = 2;
    if (!instrRegDetail.instrFixPipe.isValid) {
        return INVALID_MEM_OP_INFO;
    }
    MemOpInfo memInfo {true, instrRegDetail.instrFixPipe.src, instrRegDetail.instrFixPipe.dst, 0, 0, 1, 0, 0};
    uint64_t nSize = Utility::ExtractKBits(instrRegDetail.instrFixPipe.xm, 4, 12);
    uint64_t mSize = Utility::ExtractKBits(instrRegDetail.instrFixPipe.xm, 16, 16);
    uint64_t dstStride = Utility::ExtractKBits(instrRegDetail.instrFixPipe.xm, 32, 32);
    uint64_t quantPRE = Utility::ExtractKBits(instrRegDetail.instrFixPipe.xt, 34, 5);
    uint64_t channelSplitValue = Utility::ExtractKBits(instrRegDetail.instrFixPipe.xt, 42, 1);
    bool channelSplit = (channelSplitValue == 1) &&
                        (instrRegDetail.instrFixPipe.srcDataType == InstrDataType::B32);
    bool enableNZ2ND = Utility::ExtractKBits(instrRegDetail.instrFixPipe.xt, 43, 1) == 1;
    bool int8ChannelMerge{false};
    bool int4ChannelMerge{false};
    uint64_t quantPreBits = ParseFixPipeQuantBits(quantPRE, enableNZ2ND, int8ChannelMerge, int4ChannelMerge);

    if (enableNZ2ND) {
        memInfo.blockNum = quantPreBits == 4 ? (nSize / 2) : nSize;  // 4bits时nSize是2的倍数
        memInfo.blockSize = quantPreBits == 4 ? 1U : (quantPreBits / bitsEachByte);  // 4bits时2个合并为1个
        memInfo.repeatTimes = mSize;
        memInfo.repeatStride = quantPreBits == 4 ? (dstStride / 2) : dstStride; // 4bits时dstStride是2的倍数
        return memInfo;
    }
    memInfo.blockSize = blockSize32B;
    memInfo.repeatStride = dstStride;  // 单位是32B
    uint64_t nColumn = RoundUpDivide<uint64_t>(nSize, 16);  // 除以16向上取整
    if (channelSplit) {
        memInfo.repeatTimes = RoundUpDivide<uint64_t>(nSize, bitsEachByte);  // 除以8向上取整
    } else if (int4ChannelMerge) {
        memInfo.repeatTimes = nColumn / 4;  // 从16x16转换为16x64，N维列数除以4
    } else if (int8ChannelMerge) {
        memInfo.repeatTimes = nColumn / 2;  // 从16x16转换为16x32，N维列数除以2
    } else {
        memInfo.blockNum = mSize * quantPreBits / bitsEachByte / 2;  // *16/32B, 简化为/2
        memInfo.repeatTimes = nColumn;
        return memInfo;
    }
    // channelSplit/int4ChannelMerge/int8ChannelMerge的处理逻辑相同，合并到一处
    memInfo.blockNum = mSize;
    if (int8ChannelMerge && (nColumn % even) == 0) {
        return memInfo;
    }
    // int8ChannelMerge使能下，但N维不是16的偶数倍（被2整除），数据转换为blocksize为16B计算
    memInfo.blockSize = blockSize16B;
    memInfo.repeatTimes = memInfo.repeatTimes * (blockSize32B / blockSize16B) + 1;
    memInfo.repeatStride = dstStride * (blockSize32B / blockSize16B);  // 单位是16B，需要乘以2
    return memInfo;
}


using InstrParseFunc = std::function<MemOpInfo (const InstrRegDetail &instrRegDetail)>;
const std::unordered_map<InstrTypeTemplate, InstrParseFunc> InstrMemInfoParseFuncMap = {
    {InstrTypeTemplate::LOAD_SRC_TO_DST_2D,         ParseLoad2D},
    {InstrTypeTemplate::LOAD_OUT_TO_L1_IMAGE,       ParseLoadImage},
    {InstrTypeTemplate::LOAD_SRC_TO_SMASK,          ParseLoadSmask},
    {InstrTypeTemplate::MOV_SRC_TO_DST,             ParseDmaMov},
    {InstrTypeTemplate::MOV_OUT_TO_L1_MULTI_ND2NZ,  ParseDmaMovNd2Nz},
    {InstrTypeTemplate::MOV_SRC_TO_DST_ALIGN,       ParseDmaMovAlign},
    {InstrTypeTemplate::FIX_L0C_TO_DST,             ParseFixPipe},
};

InstrRegDetail ConstructInstrRegCommon(const RegDetailRegexMap &regMap, const std::string &instrDetail)
{
    // support in 310P A2 A3
    InstrRegDetail instrRegDetail {};
    if (GetRegValue(regMap.at(RegNameKey::XN_VALUE), instrDetail, instrRegDetail.instrRegCommon.src) &&
        GetRegValue(regMap.at(RegNameKey::XD_VALUE), instrDetail, instrRegDetail.instrRegCommon.dst) &&
        GetRegValue(regMap.at(RegNameKey::XM_VALUE), instrDetail, instrRegDetail.instrRegCommon.config)) {
        instrRegDetail.instrRegCommon.isValid = true;
        return instrRegDetail;
    }
    instrRegDetail.instrRegCommon.isValid = false;
    instrRegDetail.instrRegCommon.src = 0;
    instrRegDetail.instrRegCommon.dst = 0;
    instrRegDetail.instrRegCommon.config = 0;
    return instrRegDetail;
}

InstrRegDetail ConstructInstrLoadSmask(const RegDetailRegexMap &regMap, const std::string &instrDetail)
{
    // support in 310P A2 A3
    InstrRegDetail instrRegDetail {};
    if (GetRegValue(regMap.at(RegNameKey::XN_VALUE), instrDetail, instrRegDetail.instrRegCommon.src) &&
        GetRegValue(regMap.at(RegNameKey::XD_VALUE), instrDetail, instrRegDetail.instrRegCommon.dst) &&
        GetRegValue(regMap.at(RegNameKey::XT_VALUE), instrDetail, instrRegDetail.instrRegCommon.config)) {
        instrRegDetail.instrRegCommon.isValid = true;
        return instrRegDetail;
    }
    instrRegDetail.instrRegCommon.isValid = false;
    instrRegDetail.instrRegCommon.src = 0;
    instrRegDetail.instrRegCommon.dst = 0;
    instrRegDetail.instrRegCommon.config = 0;
    return instrRegDetail;
}

InstrRegDetail ConstructInstrLoadImage(const RegDetailRegexMap &regMap, const std::string &instrDetail)
{
    // support in 310P A2 A3
    InstrRegDetail instrRegDetail {};
    if (GetRegValue(regMap.at(RegNameKey::XD_VALUE), instrDetail, instrRegDetail.instrLoadImage.dst) &&
        GetRegValue(regMap.at(RegNameKey::XS_VALUE), instrDetail, instrRegDetail.instrLoadImage.xs) &&
        GetRegValue(regMap.at(RegNameKey::XT_VALUE), instrDetail, instrRegDetail.instrLoadImage.xt)) {
        instrRegDetail.instrLoadImage.isValid = true;
        return instrRegDetail;
    }
    instrRegDetail.instrLoadImage.isValid = false;
    instrRegDetail.instrLoadImage.dst = 0;
    instrRegDetail.instrLoadImage.xs = 0;
    instrRegDetail.instrLoadImage.xt = 0;
    return instrRegDetail;
}

InstrRegDetail ConstructInstrDmaMovNd2Nz(const RegDetailRegexMap &regMap, const std::string &instrDetail)
{
    // support in A2 A3
    InstrRegDetail instrRegDetail {};
    std::string dType {};
    if (GetRegValue(regMap.at(RegNameKey::XN_VALUE), instrDetail, instrRegDetail.instrDmaMovNd2Nz.src) &&
        GetRegValue(regMap.at(RegNameKey::XD_VALUE), instrDetail, instrRegDetail.instrDmaMovNd2Nz.dst) &&
        GetRegValue(regMap.at(RegNameKey::XM_VALUE), instrDetail, instrRegDetail.instrDmaMovNd2Nz.xm) &&
        GetRegValue(regMap.at(RegNameKey::XT_VALUE), instrDetail, instrRegDetail.instrDmaMovNd2Nz.xt) &&
        GetRegString(regMap.at(RegNameKey::DTYPE), instrDetail, dType) &&
        VEC_DTYPE_BYTES_A2A3_MAP.find(dType) != VEC_DTYPE_BYTES_A2A3_MAP.end()) {
        instrRegDetail.instrDmaMovNd2Nz.instrDetailDataType = VEC_DTYPE_BYTES_A2A3_MAP.at(dType);
        instrRegDetail.instrDmaMovNd2Nz.isValid = true;
        return instrRegDetail;
    }
    instrRegDetail.instrDmaMovNd2Nz.isValid = false;
    instrRegDetail.instrDmaMovNd2Nz.src = 0;
    instrRegDetail.instrDmaMovNd2Nz.dst = 0;
    instrRegDetail.instrDmaMovNd2Nz.xm = 0;
    instrRegDetail.instrDmaMovNd2Nz.xt = 0;
    instrRegDetail.instrDmaMovNd2Nz.instrDetailDataType = InstrDataType::INVALID;
    return instrRegDetail;
}


InstrRegDetail ConstructInstrDmaMovAlign(const RegDetailRegexMap &regMap, const std::string &instrDetail)
{
    // support in A2 A3
    InstrRegDetail instrRegDetail {};
    std::string srcMemType {};
    if (GetRegValue(regMap.at(RegNameKey::XN_VALUE), instrDetail, instrRegDetail.instrDmaMovAlign.src) &&
        GetRegValue(regMap.at(RegNameKey::XD_VALUE), instrDetail, instrRegDetail.instrDmaMovAlign.dst) &&
        GetRegValue(regMap.at(RegNameKey::XM_VALUE), instrDetail, instrRegDetail.instrDmaMovAlign.xm) &&
        GetRegValue(regMap.at(RegNameKey::XT_VALUE), instrDetail, instrRegDetail.instrDmaMovAlign.xt) &&
        GetRegString(regMap.at(RegNameKey::SRC), instrDetail, srcMemType) &&
        INSTR_MEM_TYPE_A2A3_MAP.find(srcMemType) != INSTR_MEM_TYPE_A2A3_MAP.end()) {
        instrRegDetail.instrDmaMovAlign.instrMemType = INSTR_MEM_TYPE_A2A3_MAP.at(srcMemType);
        instrRegDetail.instrDmaMovAlign.isValid = true;
        return instrRegDetail;
    }
    instrRegDetail.instrDmaMovAlign.isValid = false;
    instrRegDetail.instrDmaMovAlign.src = 0;
    instrRegDetail.instrDmaMovAlign.dst = 0;
    instrRegDetail.instrDmaMovAlign.xm = 0;
    instrRegDetail.instrDmaMovAlign.xt = 0;
    instrRegDetail.instrDmaMovAlign.instrMemType = InstrMemType::INVALID;
    return instrRegDetail;
}

InstrRegDetail ConstructInstrFixPipe(const RegDetailRegexMap &regMap, const std::string &instrDetail)
{
    // support in A2 A3
    InstrRegDetail instrRegDetail {};
    std::string dstMemType {};
    if (GetRegValue(regMap.at(RegNameKey::XN_VALUE), instrDetail, instrRegDetail.instrFixPipe.src) &&
        GetRegValue(regMap.at(RegNameKey::XD_VALUE), instrDetail, instrRegDetail.instrFixPipe.dst) &&
        GetRegValue(regMap.at(RegNameKey::XM_VALUE), instrDetail, instrRegDetail.instrFixPipe.xm) &&
        GetRegValue(regMap.at(RegNameKey::XT_VALUE), instrDetail, instrRegDetail.instrFixPipe.xt) &&
        GetRegString(regMap.at(RegNameKey::DST), instrDetail, dstMemType) &&
        INSTR_MEM_TYPE_A2A3_MAP.find(dstMemType) != INSTR_MEM_TYPE_A2A3_MAP.end()) {
        std::string srcDataType {};
        if (GetRegString(regMap.at(RegNameKey::SRC), instrDetail, srcDataType) &&
            srcDataType == "L0C32") {
            instrRegDetail.instrFixPipe.srcDataType = InstrDataType::B32;
        } else {
            instrRegDetail.instrFixPipe.srcDataType = InstrDataType::B16;
        }
        instrRegDetail.instrFixPipe.dstMemType = INSTR_MEM_TYPE_A2A3_MAP.at(dstMemType);
        instrRegDetail.instrFixPipe.isValid = true;
        return instrRegDetail;
    }
    instrRegDetail.instrFixPipe.isValid = false;
    instrRegDetail.instrFixPipe.src = 0;
    instrRegDetail.instrFixPipe.dst = 0;
    instrRegDetail.instrFixPipe.xm = 0;
    instrRegDetail.instrFixPipe.xt = 0;
    instrRegDetail.instrFixPipe.srcDataType = InstrDataType::INVALID;
    instrRegDetail.instrFixPipe.dstMemType = InstrMemType::INVALID;
    return instrRegDetail;
}

const InstrProcessMap A2A3InstrProcessMap = {
    {"LOAD_2D",                     {ConstructInstrRegCommon, InstrTypeTemplate::LOAD_SRC_TO_DST_2D}},
    {"LOAD_IMAGE",                  {ConstructInstrLoadImage, InstrTypeTemplate::LOAD_OUT_TO_L1_IMAGE}},
    {"MOV_OUT_TO_UB",               {ConstructInstrRegCommon, InstrTypeTemplate::MOV_SRC_TO_DST}},
    {"MOV_OUT_TO_L1",               {ConstructInstrRegCommon, InstrTypeTemplate::MOV_SRC_TO_DST}},
    {"MOV_UB_TO_OUT",               {ConstructInstrRegCommon, InstrTypeTemplate::MOV_SRC_TO_DST}},
    {"MOV_L1_TO_OUT",               {ConstructInstrRegCommon, InstrTypeTemplate::MOV_SRC_TO_DST}},
    {"MOV_OUT_TO_L1_MULTI_ND2NZ",   {ConstructInstrDmaMovNd2Nz, InstrTypeTemplate::MOV_OUT_TO_L1_MULTI_ND2NZ}},
    {"MOV_SRC_TO_DST_ALIGN",        {ConstructInstrDmaMovAlign, InstrTypeTemplate::MOV_SRC_TO_DST_ALIGN}},
    {"FIX_L0C_TO_DST",              {ConstructInstrFixPipe, InstrTypeTemplate::FIX_L0C_TO_DST}},
};

const std::unordered_map<ChipProductType, InstrProcessMap> ChipBasedInstrRegDetailMap = {
    {ChipProductType::ASCEND310P_SERIES, {
        {"load_out_to_l0a_2d",      {ConstructInstrRegCommon, InstrTypeTemplate::LOAD_SRC_TO_DST_2D}},
        {"load_out_to_l0b_2d",      {ConstructInstrRegCommon, InstrTypeTemplate::LOAD_SRC_TO_DST_2D}},
        {"load_out_to_l1_2d",       {ConstructInstrRegCommon, InstrTypeTemplate::LOAD_SRC_TO_DST_2D}},
        {"load_out_to_l1_image",    {ConstructInstrLoadImage, InstrTypeTemplate::LOAD_OUT_TO_L1_IMAGE}},
        {"load_src_to_smask",       {ConstructInstrLoadSmask, InstrTypeTemplate::LOAD_SRC_TO_SMASK}},
        {"mov_out_to_ub",           {ConstructInstrRegCommon, InstrTypeTemplate::MOV_SRC_TO_DST}},
        {"mov_out_to_l1",           {ConstructInstrRegCommon, InstrTypeTemplate::MOV_SRC_TO_DST}},
        {"mov_ub_to_out",           {ConstructInstrRegCommon, InstrTypeTemplate::MOV_SRC_TO_DST}},
    }},
    {ChipProductType::ASCEND910B_SERIES, A2A3InstrProcessMap},
    {ChipProductType::ASCEND910_93_SERIES, A2A3InstrProcessMap},
};

bool InstrDetailCalculator::GetMemOpInfo(const MergeInfo &instrInfo, MemOpInfo &memOpInfo)
{
    auto it = instrProcessMap_.find(instrInfo.name);
    if (it == instrProcessMap_.end()) {
        return false;
    }
    InstrExtractAndParsePair instrExtractAndParsePair = it->second;
    // Extract instrRegDetail from instr detail
    InstrRegDetail instrRegDetail = instrExtractAndParsePair.first(regDetailRegexMap_, instrInfo.detail);
    // Get MemOpInfo from instrRegDetail
    if (InstrMemInfoParseFuncMap.count(instrExtractAndParsePair.second) == 0) {
        return false;
    }
    memOpInfo = InstrMemInfoParseFuncMap.at(instrExtractAndParsePair.second)(instrRegDetail);
    if (!memOpInfo.isValid) {
        return false;
    }
    return true;
}

void InstrDetailCalculator::AttributeMapInit()
{
    const SpStruct2RegexMap sprRegexMapA2A3 =  {
        {"MOVEMASK", {{std::regex("XN:X[0-9]{1,2}=(?:0x)?([0-9a-f]+),\\s?Pos:([0-9]{1})"), 2, 0}}},
        {"MOV_SPR_XN", {
            {std::regex("SPR:ND_PARA,\\s?XN:X[0-9]{1,2}=(?:0x)?([0-9a-f]+)"), 1, 2},
            {std::regex("SPR:CTRL,\\s?XN:X[0-9]{1,2}=(?:0x)?([0-9a-f]+)"), 1, 3}}
        },
    };
    const std::unordered_map<ChipProductType, SpStruct2RegexMap> SprRegNamePattern = {
        {ChipProductType::ASCEND310P_SERIES, {
            {"movemask", {{std::regex("mask:([0-9]{1}),\\s?X[0-9]{1,2}:(?:0x)?([0-9a-f]+)"), 2, 0}}},
            {"scalar_mov_xd_special", {{std::regex("SPR_CTRL=\\s?(?:0x)?([0-9a-f]+)"), 1, 3}}},
        }},
        {ChipProductType::ASCEND910B_SERIES, sprRegexMapA2A3},
        {ChipProductType::ASCEND910_93_SERIES, sprRegexMapA2A3},
    };
    ChipProductType chipSeriesType = instrDetailConfig_.GetProductSeriesType();
    // chip type is check in DependencyCheck
    GetRegDetailRegexMap(chipSeriesType, regDetailRegexMap_);
    instrProcessMap_ = ChipBasedInstrRegDetailMap.at(chipSeriesType);
    auto it = SprRegNamePattern.find(chipSeriesType);
    if (it == SprRegNamePattern.end()) {
        spEnable_ = false;
    } else {
        spEnable_ = true;
        spStruct2RegexMap_ = it->second;
    }
}

void InstrDetailCalculator::UpdateSpReg(const std::string &name, const std::string &detail, SpReg &spReg)
{
    auto it = spStruct2RegexMap_.find(name);
    if (it == spStruct2RegexMap_.end()) {
        return;
    }
    for (const auto &spRegex : it->second) {
        std::vector<uint64_t> resValueList;
        if (!Utility::GetUint64ListFromStr(spRegex.pattern, detail, resValueList) ||
            resValueList.size() != spRegex.expectResNum) {
            continue;
        }
        // 下面的索引在上面spRegex.expectResNum判断已校验，无越界风险
        uint8_t pos = spRegex.posInSpReg;
        if (pos == static_cast<uint8_t>(SpRegPosEnum::VECTOR_MASK0)) {
            uint64_t maskSelected;
            uint64_t value;
            // 临时特殊判断，后续仿真器修改后会移除。当前A2A3先mask再pos，310P先pos，再mask
            if (IsChipSeriesTypeValid(instrDetailConfig_.GetChipType(),
                                      ChipProductType::ASCEND910B_SERIES) ||
                IsChipSeriesTypeValid(instrDetailConfig_.GetChipType(),
                                      ChipProductType::ASCEND910_93_SERIES)) {
                value = resValueList[0];
                maskSelected = resValueList[1];
            } else {
                maskSelected = resValueList[0];
                value = resValueList[1];
            }
            if (maskSelected == 1) {
                spReg.vectorMask1 = value;
            } else {
                spReg.vectorMask0 = value;
            }
        } else if (pos == static_cast<uint8_t>(SpRegPosEnum::ND_PARA_CONFIG)) {
            spReg.ndParaConfig = resValueList[0];
        } else if (pos == static_cast<uint8_t>(SpRegPosEnum::MASK_MODE)) {
            spReg.maskMode = static_cast<MaskMode>(
                    Utility::ExtractKBits(resValueList[0], MASK_MODE_BIT_IN_CTRL_REGISTER, 1));
        }
        break;
    }
}
}
}
