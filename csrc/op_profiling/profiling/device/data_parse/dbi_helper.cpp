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

#include <sstream>
#include "common/dbi_defs.h"
#include "log.h"
#include "dbi_helper.h"
/*
 * 日志规范
 * coreId -> pc -> dst属性 -> src属性 -> 其余属性
 *
*/
using namespace Common;
using namespace Utility;
namespace Profiling {

std::unordered_map<Common::DataType, std::string> RECORD_DATA_TYPE_MAP {
    {Common::DataType::DATA_B4, "B4"},
    {Common::DataType::DATA_B8, "B8"},
    {Common::DataType::DATA_B16, "B16"},
    {Common::DataType::DATA_B32, "B32"},
    {Common::DataType::DATA_F16, "F16"},
    {Common::DataType::DATA_BF16, "BF16"},
    {Common::DataType::DATA_F32, "F32"},
    {Common::DataType::DATA_S32, "S32"},
};

std::unordered_map<Common::MemType, std::string> RECORD_MEM_TYPE_MAP {
    {Common::MemType::L1, "L1"},
    {Common::MemType::L0A, "L0A"},
    {Common::MemType::L0B, "L0B"},
    {Common::MemType::L0C, "L0C"},
    {Common::MemType::UB, "UB"},
    {Common::MemType::BT, "BT"},
    {Common::MemType::GM, "GM"},
    {Common::MemType::REG, "REG"},
    {Common::MemType::PRIVATE, "PRIVATE"},
};

std::unordered_map<Common::PadMode, std::string> RECORD_PAD_MODE_MAP {
    {Common::PadMode::PAD_NONE, "NONE"},
    {Common::PadMode::PAD_MODE1, "MODE1"},
    {Common::PadMode::PAD_MODE2, "MODE2"},
    {Common::PadMode::PAD_MODE3, "MODE3"},
    {Common::PadMode::PAD_MODE4, "MODE4"},
    {Common::PadMode::PAD_MODE5, "MODE5"},
    {Common::PadMode::PAD_MODE6, "MODE6"},
    {Common::PadMode::PAD_MODE7, "MODE7"},
    {Common::PadMode::PAD_MODE8, "MODE8"},
};

std::ostream &operator<<(std::ostream &os, const MemType& memType)
{
    if (RECORD_MEM_TYPE_MAP.find(memType) == RECORD_MEM_TYPE_MAP.end()) {
        return os << "unknow";
    }
    return os << RECORD_MEM_TYPE_MAP[memType];
}

std::ostream &operator<<(std::ostream &os, const DataType& dataType)
{
    if (RECORD_DATA_TYPE_MAP.find(dataType) == RECORD_DATA_TYPE_MAP.end()) {
        return os << "unknown";
    }
    return os << RECORD_DATA_TYPE_MAP[dataType];
}

std::ostream &operator<<(std::ostream &os, const PadMode& padMode)
{
    if (RECORD_PAD_MODE_MAP.find(padMode) == RECORD_PAD_MODE_MAP.end()) {
        return os << "unknown";
    }
    return os << RECORD_PAD_MODE_MAP[padMode];
}

std::ostream &operator<<(std::ostream &os, const LoopInfo& loopInfo)
{
    return os << "size:" << loopInfo.loopSize
              << ";lpSize:" << static_cast<uint32_t>(loopInfo.loopLpSize)
              << ";rpSize:" << static_cast<uint32_t>(loopInfo.loopRpSize)
              << ";dstStride:" << loopInfo.loopDstStride
              << ";srcStride:" << loopInfo.loopSrcStride;
}

std::ostream &operator<<(std::ostream &os, const MemRecord &record)
{
    return os << "MemRecord serialNo:" << record.serialNo
              << ";" << "pc:0x" << std::hex << record.pc << std::dec
              << ";" << "dst:(" << "type:" << record.dst
              << ";" << "addr:0x" << std::hex << record.dstAddr << std::dec
              << ";" << "memSize:" << record.dstMemSize
              << ");" << "src:(" << "type:" << record.src
              << ";" << "addr:0x" << std::hex << record.srcAddr << std::dec
              << ";" << "memSize:" << record.srcMemSize
              << ");";
}

std::ostream &operator<<(std::ostream &os, const MovRecord &record)
{
    return os << "core:" << record.coreID
              << ";" << "pc:0x" << std::hex << record.pc << std::dec
              << ";" << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "gap:" << record.dstGap
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "gap:" << record.srcGap
              << ");" << "nBurst:" << record.nBurst
              << ";" << "lenBurst:" << record.lenBurst
              << ";" << "dataType:" << record.dataType
              << ";" << "cvtEn:" << record.cvtEn;
}

std::ostream &operator<<(std::ostream &os, const LoadMX2DV2Record &record)
{
    return os << "core:" << record.coreID
              << ";" << "pc:0x" << std::hex << record.pc << std::dec
              << ";" << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ");" << "xStartPos:" << record.xStartPos
              << ";" << "xStep:" << record.xStep
              << ";" << "yStartPos:" << record.yStartPos
              << ";" << "yStep:" << record.yStep;
}

std::ostream &operator<<(std::ostream &os, const MovV2Record &record)
{
    return os << "core:" << record.coreID
              << ";" << "pc:0x" << std::hex << record.pc << std::dec
              << ";" << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ");" << "nBurst:" << record.nBurst
              << ";" << "lenBurst:" << record.lenBurst
              << ";" << "dataType:" << record.dataType
              << ";" << "sid:" << +record.sid
              << ";" << "padMode:" << record.padMode;
}

std::ostream &operator<<(std::ostream &os, const Set2DRecord &record)
{
    return os << "core:" << record.coreID
              << ";" << "pc:0x" << std::hex << record.pc << std::dec
              << ";" << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ");" << "repeat:" << record.repeat
              << ";" << "repeatGap:" << record.repeatGap
              << ";" << "blockNum:" << record.blockNum
              << ";" << "dataType:" << record.dataType;
}

std::ostream &operator<<(std::ostream &os, const Load2DV2Record &record)
{
    return os << "core:" << record.coreID
              << ";" << "pc:0x" << std::hex << record.pc << std::dec
              << ";" << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ");" << "mStartPos:" << record.mStartPos
              << ";" << "mStep:" << static_cast<uint32_t>(record.mStep)
              << ";" << "kStartPos:" << record.kStartPos
              << ";" << "kStep:" << static_cast<uint32_t>(record.kStep)
              << ";" << "transpose:" << +record.transpose
              << ";" << "dataType:" << record.dataType;
}

std::ostream &operator<<(std::ostream &os, const Load2DV2DecRecord &record)
{
    return os << "core:" << record.coreID
              << ";" << "pc:0x" << std::hex << record.pc << std::dec
              << ";" << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ");" << "mStartPos:" << record.mStartPos
              << ";" << "mStep:" << static_cast<uint32_t>(record.mStep)
              << ";" << "kStartPos:" << record.kStartPos
              << ";" << "kStep:" << static_cast<uint32_t>(record.kStep)
              << ";" << "decompMode:" << static_cast<uint32_t>(record.decompMode);
}

std::ostream &operator<<(std::ostream &os, const Load2DTransposeRecord &record)
{
    return os << "core:" << record.coreID
              << ";" << "pc:0x" << std::hex << record.pc << std::dec
              << ";" << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ");" << "baseIdx:" << record.baseIdx
              << ";" << "repeat:" << +record.repeat
              << ";" << "dataType:" << record.dataType;
}

std::ostream &operator<<(std::ostream &os, const DmaMovNd2nzRecord &record)
{
    return os << "core:" << record.coreID
              << ";" << "pc:0x" << std::hex << record.pc << std::dec
              << ";" << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "nzC0Stride:" << record.dstNzC0Stride
              << ";" << "nzNStride:" << record.dstNzNStride
              << ";" << "nzMatrixStride:" << record.dstNzMatrixStride
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "ndMatrixStride:" << record.srcNdMatrixStride
              << ";" << "dValue:" << record.srcDValue
              << ");" << "ndNum:" << record.ndNum
              << ";" << "nValue:" << record.nValue
              << ";" << "dValue:" << record.dValue
              << ";" << "dataType:" << record.dataType;
}

std::ostream &operator<<(std::ostream &os, const DmaMovNd2nzDavRecord &record)
{
    return os << "core:" << record.coreID
              << ";" << "pc:0x" << std::hex << record.pc << std::dec
              << ";" << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "loop2Stride:" << record.loop2DstStride
              << ";" << "loop3Stride:" << record.loop3DstStride
              << ";" << "loop4Stride:" << record.loop4DstStride
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "loop1Stride:" << record.loop1SrcStride
              << ";" << "loop4Stride:" << record.loop4SrcStride
              << ");" << "dataType:" << record.dataType
              << ";" << "D:" << record.dValue
              << ";" << "N:" << record.nValue
              << ";" << "ndNum:" << record.ndNum
              << ";" << "smallC0:" << static_cast<uint32_t>(record.smallC0);
}

std::ostream &operator<<(std::ostream &os, const MovAlignRecord &record)
{
    return os << "core:" << record.coreID
              << ";" << "pc:0x" << std::hex << record.pc << std::dec
              << ";" << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "gap:" << record.dstGap
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "gap:" << record.srcGap
              << ");" << "nBurst:" << record.nBurst
              << ";" << "lenBurst:" << record.lenBurst
              << ";" << "leftPaddingNum:" << record.leftPaddingNum
              << ";" << "rightPaddingNum" << record.rightPaddingNum
              << ";" << "dataType:" << record.dataType;
}

std::ostream &operator<<(std::ostream &os, const MovAlignV2Record &record)
{
    return os << "core:" << record.coreID
              << ";" << "pc:0x" << std::hex << record.pc << std::dec
              << ";" << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ";" << "loop1Stride:" << record.loop1DstStride
              << ";" << "loop2Stride:" << record.loop2DstStride
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ";" << "loop1Stride:" << record.loop1SrcStride
              << ";" << "loop2Stride:" << record.loop2SrcStride
              << ");" << "nBurst:" << record.nBurst
              << ";" << "lenBurst:" << record.lenBurst
              << ";" << "dataType:" << record.dataType
              << ";" << "loop1Size:" << record.loop1Size
              << ";" << "loop2Size:" << record.loop2Size
              << ";" << "leftPaddingNum:" << static_cast<uint32_t>(record.leftPaddingNum)
              << ";" << "rightPaddingNum:" << static_cast<uint32_t>(record.rightPaddingNum);
}

std::ostream &operator<<(std::ostream &os, const DmaMovRecord &record)
{
    return os << "core:" << record.coreID
              << ";" << "pc:0x" << std::hex << record.pc << std::dec
              << ";" << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ");" << "nBurst:" << record.nBurst
              << ";" << "lenBurst:" << record.lenBurst
              << ";" << "padMode:" << record.padMode
              << ";" << "byteMode:" << static_cast<uint16_t>(record.byteMode);
}

std::ostream& operator<<(std::ostream& os, const FixL0CGMRecord& record)
{
    return os << "core:" << record.coreID
              << ";" << "dst:(" << "stride:" << record.dstStride
              << ", ndStride:" << record.dstNdStride
              << ", memType:" << record.dstMemType
              << ");" << "src:(" << "stride:" << record.srcStride
              << ", ndStride:" << record.srcNdStride
              << ", memType:" << record.srcMemType
              << ");" << "nSize:" << record.nSize
              << ";" << "mSize:" << record.mSize
              << ";" << "ndNum:" << record.ndNum
              << ";" << "quantPreBits:" << record.quantPreBits
              << ";" << "enUnitFlag:" << record.enUnitFlag
              << ";" << "int8ChannelMerge:" << record.int8ChannelMerge
              << ";" << "int4ChannelMerge:" << record.int4ChannelMerge
              << ";" << "channelSplit:" << record.channelSplit
              << ";" << "enNZ2ND:" << record.enNZ2ND
              << ";" << "enNZ2DN:" << record.enNZ2DN
              << ";" << "dataType:" << record.dataType;
}

std::ostream &operator<<(std::ostream &os, NdDMAOut2UbRecord const &record)
{
    return os << "core:" << record.coreID
              << ";" << "pc:0x" << std::hex << record.pc << std::dec
              << ", " << "dst:(" << "type:" << MemType::UB
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ");" << "src:(" << "type:" << MemType::GM
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ");" << "loop0Info:(" << record.loop[0]
              << ");" << "loop1Info:(" << record.loop[1]
              << ");" << "loop2Info:(" << record.loop[2]
              << ");" << "loop3Info:(" << record.loop[3]
              << ");" << "loop4Info:(" << record.loop[4]
              << ");" << "dataType:" << record.dataType;
}

template<typename T>
void DBIHelper::PrintRecord(const T &record) const
{
    std::ostringstream oss;
    oss << "DBI record " << record;
    LogDebug("%s", oss.str().c_str());
}

template void DBIHelper::PrintRecord<MemRecord>(const MemRecord &) const;
template void DBIHelper::PrintRecord<MovRecord>(const MovRecord &) const;
template void DBIHelper::PrintRecord<LoadMX2DV2Record>(const LoadMX2DV2Record &) const;
template void DBIHelper::PrintRecord<MovV2Record>(const MovV2Record &) const;
template void DBIHelper::PrintRecord<Set2DRecord>(const Set2DRecord &) const;
template void DBIHelper::PrintRecord<Load2DV2Record>(const Load2DV2Record &) const;
template void DBIHelper::PrintRecord<Load2DV2DecRecord>(const Load2DV2DecRecord &) const;
template void DBIHelper::PrintRecord<Load2DTransposeRecord>(const Load2DTransposeRecord &) const;
template void DBIHelper::PrintRecord<DmaMovNd2nzRecord>(const DmaMovNd2nzRecord &) const;
template void DBIHelper::PrintRecord<DmaMovNd2nzDavRecord>(const DmaMovNd2nzDavRecord &) const;
template void DBIHelper::PrintRecord<MovAlignRecord>(const MovAlignRecord &) const;
template void DBIHelper::PrintRecord<MovAlignV2Record>(const MovAlignV2Record &) const;
template void DBIHelper::PrintRecord<DmaMovRecord>(const DmaMovRecord &) const;
template void DBIHelper::PrintRecord<FixL0CGMRecord>(const FixL0CGMRecord &) const;
template void DBIHelper::PrintRecord<NdDMAOut2UbRecord>(const NdDMAOut2UbRecord &) const;
}