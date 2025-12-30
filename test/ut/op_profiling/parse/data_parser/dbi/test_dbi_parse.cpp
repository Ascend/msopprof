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


#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"

#define private public
#define protected public
#include "profiling/device/data_parse/dbi_parser.h"
#undef protected
#undef private

using namespace Profiling;
using namespace Common;

/**
 * |  用例集 | DBIParser
 * | 测试函数 | ParseLoad2DTransposeRecord
 * |  用例名  | test_parse_load2dtranspose_record_expect_return_true
 * | 用例描述 | 输入有效值，返回true
 */

TEST(DBIParser, test_parse_load2dtranspose_record_expect_return_true)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(Load2DTransposeRecord), 0);
    Load2DTransposeRecord record{};
    record.srcMemType = MemType::L1;
    record.dstMemType = MemType::L0B;
    record.repeat = 2;
    if (memcpy_s(&buffer[0], sizeof(Load2DTransposeRecord), &record, sizeof(Load2DTransposeRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseLoad2DTransposeRecord(buffer, index, 0, 0));
}

/**
 * |  用例集 | DBIParser
 * | 测试函数 | ParseLoad2DV2Record
 * |  用例名  | test_parse_load2dv2_record_expect_return_true
 * | 用例描述 | 输入有效值，返回true
 */

TEST(DBIParser, test_parse_load2dv2_record_expect_return_true)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(Load2DV2Record), 0);
    Load2DV2Record record{};
    record.srcMemType = MemType::L1;
    record.dstMemType = MemType::L0A;
    if (memcpy_s(&buffer[0], sizeof(Load2DV2Record), &record, sizeof(Load2DV2Record)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseLoad2DV2Record(buffer, index, 0, 0));
}

/**
 * |  用例集 | DBIParser
 * | 测试函数 | ParseLoadMX2DV2Record
 * |  用例名  | test_parse_loadmx2dv2_record_expect_return_true
 * | 用例描述 | 输入有效值，返回true
 */

TEST(DBIParser, test_parse_loadmx2dv2_record_expect_return_true)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(LoadMX2DV2Record), 0);
    LoadMX2DV2Record record{};
    record.srcMemType = MemType::L1;
    record.dstMemType = MemType::L0A;
    if (memcpy_s(&buffer[0], sizeof(LoadMX2DV2Record), &record, sizeof(LoadMX2DV2Record)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseLoadMX2DV2Record(buffer, index, 0, 0));
}

/**
 * |  用例集 | DBIParser
 * | 测试函数 | ParseSet2DRecord
 * |  用例名  | test_parse_set2d_record_expect_return_true
 * | 用例描述 | 输入有效值，返回true
 */

TEST(DBIParser, test_parse_set2d_record_expect_return_true)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(Set2DRecord), 0);
    Set2DRecord record{};
    record.srcMemType = MemType::REG;
    record.dstMemType = MemType::L1;
    if (memcpy_s(&buffer[0], sizeof(Set2DRecord), &record, sizeof(Set2DRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseSet2DRecord(buffer, index, 0, 0));
}

/**
 * |  用例集 | DBIParser
 * | 测试函数 | ParseMovA5Record
 * |  用例名  | test_parse_mova5_record_expect_return_true
 * | 用例描述 | 输入有效值，返回true
 */

TEST(DBIParser, test_parse_mova5_record_expect_return_true)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(MovRecord), 0);
    MovRecord record{};
    if (memcpy_s(&buffer[0], sizeof(MovRecord), &record, sizeof(MovRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseMovA5Record(buffer, index, 0, 0));
}

/**
 * |  用例集 | DBIParser
 * | 测试函数 | ParseMovV2Record
 * |  用例名  | test_parse_movv2_record_expect_return_true
 * | 用例描述 | 输入有效值，返回true
 */

TEST(DBIParser, test_parse_movv2_record_expect_return_true)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(MovV2Record), 0);
    MovV2Record record{};
    if (memcpy_s(&buffer[0], sizeof(MovV2Record), &record, sizeof(MovV2Record)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseMovV2Record(buffer, index, 0, 0));
}

/**
 * |  用例集 | DBIParser
 * | 测试函数 | ParseFixL0CGMRecord
 * |  用例名  | test_parse_movv2_record_expect_return_true
 * | 用例描述 | 输入有效值，返回true
 */

TEST(DBIParser, test_parse_fixccGM_f32_record_expect_return_true)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(FixL0CGMRecord), 0);
    FixL0CGMRecord record{};
    record.srcMemType = MemType::L0C;
    record.dstMemType = MemType::GM;
    record.dataType = DataType::DATA_F32;
    record.src = 0x1000;
    record.dst = 0x2000;
    record.pc = 0x4000;
    record.mSize = 24; // 确保mSize不为0
    record.nSize = 48; // 确保nSize不为0
    record.srcStride = 64; // 确保srcStride是16的倍数
    record.dstStride = 64; // 确保dstStride是16的倍数
    record.quantPreBits = 32; // 确保quantPreBits是合理的值
    record.enNZ2ND = true;
    record.channelSplit = false;
    record.ndNum = 2;
    record.srcNdStride = 16;
    record.dstNdStride = 32;
    record.srcNzC0Stride = 2;

    if (memcpy_s(&buffer[0], sizeof(FixL0CGMRecord), &record, sizeof(FixL0CGMRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }

    std::size_t index = 0;
    ASSERT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords.size(), 0);
    ASSERT_TRUE(dbiParser.ParseFixL0CGMRecord(buffer, index, 0, 0));
    ASSERT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords.size(), 48);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[5].srcMemSize, 192);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[5].dstMemSize, 192);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[5].srcAddr, 0x1500);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[5].dstAddr, 0x2500);
}

TEST(DBIParser, test_parse_fixccGM_s32_record_expect_return_true)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(FixL0CGMRecord), 0);
    FixL0CGMRecord record{};
    record.srcMemType = MemType::L0C;
    record.dstMemType = MemType::GM;
    record.dataType = DataType::DATA_S32;
    record.src = 0x1000;
    record.dst = 0x2000;
    record.pc = 0x4000;
    record.mSize = 24; // 确保mSize不为0
    record.nSize = 48; // 确保nSize不为0
    record.srcStride = 64; // 确保srcStride是16的倍数
    record.dstStride = 64; // 确保dstStride是16的倍数
    record.quantPreBits = 32; // 确保quantPreBits是合理的值
    record.enNZ2ND = false;
    record.channelSplit = true;
    record.ndNum = 2;
    record.srcNdStride = 16;
    record.dstNdStride = 32;
    record.srcNzC0Stride = 2;
    if (memcpy_s(&buffer[0], sizeof(FixL0CGMRecord), &record, sizeof(FixL0CGMRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }

    std::size_t index = 0;
    ASSERT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords.size(), 0);
    ASSERT_TRUE(dbiParser.ParseFixL0CGMRecord(buffer, index, 0, 0));
    ASSERT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords.size(), 6);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[1].srcMemSize, 768);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[1].dstMemSize, 768);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[1].srcAddr, 0x1800);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[1].dstAddr, 0x2800);
}
/*
 * | 测试函数 | ParseMovAlignV2Record
 * |  用例名  | test_parse_movalign2d_record_expect_return_true
 * | 用例描述 | 输入有效值，返回true
 */

TEST(DBIParser, test_parse_movalign2d_record_expect_return_true)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(MovAlignV2Record), 0);
    MovAlignV2Record record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L1;
    record.dstStride = 64;
    record.srcStride = 48;
    record.loop1DstStride = 128;
    record.loop2DstStride = 64;
    record.loop1SrcStride = 96;
    record.loop2SrcStride = 48;
    record.loop1Size = 1;
    record.loop2Size = 1;
    record.nBurst = 2;
    record.lenBurst = 48;
    record.src = 0x1000;
    record.dst = 0x2000;
    record.pc = 0x4000;
    if (memcpy_s(&buffer[0], sizeof(MovAlignV2Record), &record, sizeof(MovAlignV2Record)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseMovAlignV2Record(buffer, index, 0, 0));
}

/**
 * |  用例集 | DBIParser
 * | 测试函数 | ParseDmaMovNd2nzDavRecord
 * |  用例名  | test_parse_dma_mov_nd2nz_dav_record_normal_mode_expect_return_true
 * | 用例描述 | 输入有效值，返回true
 */

TEST(DBIParser, test_parse_dma_mov_nd2nz_dav_record_normal_mode_expect_return_true)
{
    DBIParser dbiParser("");
    DmaMovNd2nzDavRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L1;
    record.src = 0x1000;
    record.dst = 0x2000;
    record.pc = 0x4000;
    record.smallC0 = false;
    record.dataType = DataType::DATA_B8;
    record.ndNum = 2;
    record.dValue = 10;
    record.nValue = 3;
    record.loop1SrcStride = 10;
    record.loop2DstStride = 8;
    record.loop3DstStride = 16;
    record.loop4SrcStride = 32;
    record.loop4DstStride = 64;
    std::string buffer(sizeof(DmaMovNd2nzDavRecord), 0);
    if (memcpy_s(&buffer[0], sizeof(DmaMovNd2nzDavRecord), &record, sizeof(DmaMovNd2nzDavRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords.size(), 0);
    ASSERT_TRUE(dbiParser.ParseDmaMovNd2nzDavRecord(buffer, index, 0, 0));
    ASSERT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords.size(), 6);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[5].srcMemSize, 10);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[5].dstMemSize, 32);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[5].srcAddr, 0x1034);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[5].dstAddr, 0x2a00);
}

/**
 * |  用例集 | DBIParser
 * | 测试函数 | ParseDmaMovNd2nzDavRecord
 * |  用例名  | test_parse_dma_mov_nd2nz_dav_record_small_c0_mode_expect_return_true
 * | 用例描述 | 输入有效值，返回true
 */

TEST(DBIParser, test_parse_dma_mov_nd2nz_dav_record_small_c0_mode_expect_return_true)
{
    DBIParser dbiParser("");
    DmaMovNd2nzDavRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L1;
    record.src = 0x1000;
    record.dst = 0x2000;
    record.pc = 0x4000;
    record.smallC0 = true;
    record.dataType = DataType::DATA_B16;
    record.ndNum = 1;
    record.dValue = 3;
    record.nValue = 5;
    record.loop1SrcStride = 6;
    record.loop4DstStride = 4;
    std::string buffer(sizeof(DmaMovNd2nzDavRecord), 0);
    if (memcpy_s(&buffer[0], sizeof(DmaMovNd2nzDavRecord), &record, sizeof(DmaMovNd2nzDavRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords.size(), 0);
    ASSERT_TRUE(dbiParser.ParseDmaMovNd2nzDavRecord(buffer, index, 0, 0));
    ASSERT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords.size(), 5);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[4].srcMemSize, 6);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[4].dstMemSize, 8);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[4].srcAddr, 0x1018);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[4].dstAddr, 0x2020);
}


/**
 * |  用例集 | DBIParser
 * | 测试函数 | ParseMovAlignV2Record
 * |  用例名  | test_parse_mov_align2d_record_with_unaligned_ub_to_gm_expect_parse_correct
 * | 用例描述 | 输入有效值，返回true
 */

TEST(DBIParser, test_parse_mov_align2d_record_with_unaligned_ub_to_gm_expect_parse_correct)
{
    DBIParser dbiParser("");
    uint32_t key = 0;
    ASSERT_EQ(dbiParser.memoryChartMetrics_[key].memoryRecords.size(), 0); // manual insert key into chart metrics
    std::string buffer(sizeof(MovAlignV2Record), 0);
    MovAlignV2Record record{};
    record.srcMemType = MemType::UB;
    record.dstMemType = MemType::GM;
    record.dstStride = 64;
    record.srcStride = 32;
    record.loop1DstStride = 128;
    record.loop2DstStride = 64;
    record.loop1SrcStride = 96;
    record.loop2SrcStride = 32;
    record.loop1Size = 1;
    record.loop2Size = 1;
    record.nBurst = 2;
    record.lenBurst = 48;
    record.src = 0x1000;
    record.dst = 0x2000;
    record.pc = 0x4000;
    if (!memcpy_s(&buffer[0], sizeof(MovAlignV2Record), &record, sizeof(MovAlignV2Record)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseMovAlignV2Record(buffer, index, key, 0));
    // ub is 48b per bust, unaligned.
    printf("before insert %zu %zu\n", dbiParser.memoryChartMetrics_.size(), dbiParser.memoryChartMetrics_[0].memoryRecords.size());
    ASSERT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords.size(), 2);
    ASSERT_TRUE(dbiParser.memoryChartMetrics_[0].memoryRecords[0].srcMemSize == 64u);
    ASSERT_TRUE(dbiParser.memoryChartMetrics_[0].memoryRecords[0].dstMemSize == 48u);
    ASSERT_TRUE(dbiParser.memoryChartMetrics_[0].memoryRecords[1].srcMemSize == 64u);
    ASSERT_TRUE(dbiParser.memoryChartMetrics_[0].memoryRecords[1].dstMemSize == 48u);
}

/**
 * |  用例集 | DBIParser
 * | 测试函数 | ParseDmaMovDn2nzDavRecord
 * |  用例名  | test_parse_dma_mov_dn2nz_dav_expect_parse_correct
 * | 用例描述 | 输入有效值，返回true
 */

TEST(DBIParser, test_parse_dma_mov_dn2nz_dav_expect_parse_correct)
{
    DBIParser dbiParser("");

    std::string buffer(sizeof(DmaMovNd2nzDavRecord), 0);
    uint32_t key = 0;
    ASSERT_EQ(dbiParser.memoryChartMetrics_[key].memoryRecords.size(), 0); // manual insert key into chart metrics
    DmaMovNd2nzDavRecord record{};
    record.loop4SrcStride = 960;
    record.loop1SrcStride = 32;
    record.dValue = 24;
    record.nValue = 8;
    record.ndNum = 2;
    record.loop2DstStride = 2;
    record.loop3DstStride = 512;
    record.loop4DstStride = 256;
    record.smallC0 = 0;
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L1;
    record.dataType = Common::DataType::DATA_B16;
    record.src = 0x1000;
    record.dst = 0x2000;
    record.pc = 0x4000;
    if (memcpy_s(&buffer[0], sizeof(DmaMovNd2nzDavRecord), &record, sizeof(DmaMovNd2nzDavRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseDmaMovDn2nzDavRecord(buffer, index, key, 0));
    ASSERT_EQ(dbiParser.memoryChartMetrics_[key].memoryRecords.size(), record.dValue * record.ndNum);
    ASSERT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[0].srcMemSize, record.nValue * 2); // b16
    ASSERT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[1].dstMemSize, 64);
}
/**
 * |  用例集 | DBIParser
 * | 测试函数 | ParseNdDMAOut2UbRecord
 * |  用例名  | test_parse_NdDMAOut2UbRecord_record_expect_return_true
 * | 用例描述 | 输入有效值，返回true
 */
TEST(DBIParser, test_parse_NdDMAOut2UbRecord_record_expect_return_true)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(NdDMAOut2UbRecord), 0);
    NdDMAOut2UbRecord record{};
    LoopInfo info = {5, 10, 1, 2, 3};
    for (auto & i : record.loop) {
        i = info;
    }
    record.coreID = 0;
    record.dataType = DataType::DATA_B8;
    record.src = 0x1000;
    record.dst = 0x2000;
    record.pc = 0x4000;
    if (memcpy_s(&buffer[0], sizeof(NdDMAOut2UbRecord), &record, sizeof(NdDMAOut2UbRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    dbiParser.memoryChartMetrics_[0] = {};
    ASSERT_TRUE(dbiParser.ParseNdDMAOut2UbRecord(buffer, index, 0, 0));
    ASSERT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords.size(), pow(6, 5));
}

/**
 * |  用例集 | DBIParser
 * | 测试函数 | ParseLoad2DV2DecRecord
 * |  用例名  | test_parse_load2dv2dec_record_expect_return_true
 * | 用例描述 | 输入有效值，返回true
 */

TEST(DBIParser, test_parse_load2dv2dec_record_expect_return_true)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(Load2DV2DecRecord), 0);
    Load2DV2DecRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L1;
    record.decompMode = 0;
    record.srcStride = 1;
    record.kStartPos = 0;
    record.mStartPos = 0;
    record.kStep = 1;
    record.mStep = 1;
    record.src = 0x1000;
    record.dst = 0x2000;
    record.pc = 0x4000;
    if (memcpy_s(&buffer[0], sizeof(Load2DV2DecRecord), &record, sizeof(Load2DV2DecRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords.size(), 0);
    ASSERT_TRUE(dbiParser.ParseLoad2DV2DecRecord(buffer, index, 0, 0));
    ASSERT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords.size(), 1);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[0].srcMemSize, 512);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[0].dstMemSize, 512);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[0].srcAddr, 0x1000);
    EXPECT_EQ(dbiParser.memoryChartMetrics_[0].memoryRecords[0].dstAddr, 0x2000);
}
