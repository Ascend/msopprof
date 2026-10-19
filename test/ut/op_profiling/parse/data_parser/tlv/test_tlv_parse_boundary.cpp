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

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include <gtest/gtest.h>

#define private public
#include "profiling/device/data_parse/tlv_parse.h"
#undef private

namespace {
template <typename T>
void AppendValue(std::vector<uint8_t> &data, const T &value)
{
    const auto *begin = reinterpret_cast<const uint8_t *>(&value);
    data.insert(data.end(), begin, begin + sizeof(T));
}

template <typename T>
std::vector<uint8_t> MakeValueBytes(const T &value)
{
    std::vector<uint8_t> data;
    AppendValue(data, value);
    return data;
}

void AppendTlv(std::vector<uint8_t> &data, TlvTag tag, const std::vector<uint8_t> &value)
{
    AppendValue(data, tag);
    AppendValue(data, static_cast<TlvLengthType>(value.size()));
    data.insert(data.end(), value.begin(), value.end());
}

std::vector<uint8_t> MakeTlv(TlvTag tag, const std::vector<uint8_t> &value)
{
    std::vector<uint8_t> data;
    AppendTlv(data, tag, value);
    return data;
}

std::vector<uint8_t> MakeInstRecord(uint64_t address)
{
    std::vector<uint8_t> value;
    AppendTlv(value, TlvTag::addressTag, MakeValueBytes(address));
    return MakeTlv(TlvTag::InstRecordTag, value);
}

std::vector<uint8_t> MakeInstRecordGroup(uint64_t address)
{
    return MakeTlv(TlvTag::InstRecordListByRegTypeTag, MakeInstRecord(address));
}

class TlvParserUt : public testing::Test {
protected:
    void SetUp() override
    {
        const auto *testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
        filePath_ = ::testing::TempDir() + "msopt_tlv_" + testInfo->name() + ".data";
        (void)std::remove(filePath_.c_str());
    }

    void TearDown() override
    {
        (void)std::remove(filePath_.c_str());
    }

    void WriteBytes(const std::vector<uint8_t> &data)
    {
        std::ofstream file(filePath_, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(file.is_open()) << "Failed to create test input: " << filePath_;
        if (!data.empty()) {
            file.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
        }
        ASSERT_TRUE(file.good()) << "Failed to write test input: " << filePath_;
    }

    std::string filePath_;
};
} // namespace

/**
 * |  用例集  | TlvParserUt
 * | 测试函数 | ParseStream
 * |  用例名  | test_ParseStream_when_file_does_not_exist_expect_false
 * | 用例描述 | 使用不存在的文件路径调用公开解析入口，验证文件打开失败时返回false且不进入TLV解析
 */
TEST_F(TlvParserUt, test_ParseStream_when_file_does_not_exist_expect_false)
{
    TlvParser parser(filePath_);
    EXPECT_FALSE(parser.ParseStream());
}

/**
 * |  用例集  | TlvParserUt
 * | 测试函数 | ParseStream
 * |  用例名  | test_ParseStream_when_file_is_empty_expect_false
 * | 用例描述 | 创建可以正常打开的零字节文件，验证输入不含任何TLV数据时返回false
 */
TEST_F(TlvParserUt, test_ParseStream_when_file_is_empty_expect_false)
{
    WriteBytes({});
    TlvParser parser(filePath_);
    EXPECT_FALSE(parser.ParseStream());
}

/**
 * |  用例集  | TlvParserUt
 * | 测试函数 | ParseStream
 * |  用例名  | test_ParseStream_when_top_level_header_is_truncated_expect_false
 * | 用例描述 | 文件只写入顶层RegLivenessTag而缺少Length字段，验证头部读取越界时解析安全失败
 */
TEST_F(TlvParserUt, test_ParseStream_when_top_level_header_is_truncated_expect_false)
{
    std::vector<uint8_t> data;
    AppendValue(data, TlvTag::RegLivenessTag);
    WriteBytes(data);
    TlvParser parser(filePath_);
    EXPECT_FALSE(parser.ParseStream());
}

/**
 * |  用例集  | TlvParserUt
 * | 测试函数 | ParseStream
 * |  用例名  | test_ParseStream_when_top_level_tag_is_not_reg_liveness_expect_false
 * | 用例描述 | 写入结构完整但顶层Tag为InstNumTag的TLV，验证字节充足但顶层语义错误时返回false
 */
TEST_F(TlvParserUt, test_ParseStream_when_top_level_tag_is_not_reg_liveness_expect_false)
{
    WriteBytes(MakeTlv(TlvTag::InstNumTag, {}));
    TlvParser parser(filePath_);
    EXPECT_FALSE(parser.ParseStream());
}

/**
 * |  用例集  | TlvParserUt
 * | 测试函数 | ReadTlvHeader
 * |  用例名  | test_ReadTlvHeader_when_declared_value_exceeds_remaining_bytes_expect_false
 * | 用例描述 | TLV头部声明Value长度为8字节但不提供Value，验证声明长度超过容器剩余空间时返回false
 */
TEST_F(TlvParserUt, test_ReadTlvHeader_when_declared_value_exceeds_remaining_bytes_expect_false)
{
    std::vector<uint8_t> data;
    AppendValue(data, TlvTag::addressTag);
    AppendValue(data, TlvLengthType{8});
    size_t offset = 0;
    TlvTag tag = TlvTag::RegLivenessTag;
    TlvLengthType length = 0;
    TlvParser parser("");

    EXPECT_FALSE(parser.ReadTlvHeader(data, offset, data.size(), tag, length));
    EXPECT_EQ(tag, TlvTag::addressTag);
    EXPECT_EQ(length, 8U);
    EXPECT_EQ(offset, data.size());
}

/**
 * |  用例集  | TlvParserUt
 * | 测试函数 | ReadTlvString
 * |  用例名  | test_ReadTlvString_when_string_exceeds_remaining_bytes_expect_empty_and_offset_unchanged
 * | 用例描述 | 两字节输入从下标1开始请求读取两字节字符串，验证越界时返回空字符串且不消费输入偏移
 */
TEST_F(TlvParserUt, test_ReadTlvString_when_string_exceeds_remaining_bytes_expect_empty_and_offset_unchanged)
{
    const std::vector<uint8_t> data = {0x41, 0x42};
    size_t offset = 1;
    TlvParser parser("");

    EXPECT_TRUE(parser.ReadTlvString(data, offset, 2).empty());
    EXPECT_EQ(offset, 1U);
}

/**
 * |  用例集  | TlvParserUt
 * | 测试函数 | ParseRegLivenessTag
 * |  用例名  | test_ParseRegLivenessTag_when_subtag_is_unknown_expect_false
 * | 用例描述 | 在顶层Value中放入未定义的子Tag，验证解析器不会静默跳过未知标签而是返回false
 */
TEST_F(TlvParserUt, test_ParseRegLivenessTag_when_subtag_is_unknown_expect_false)
{
    const auto data = MakeTlv(static_cast<TlvTag>(0xff), {});
    size_t offset = 0;
    TlvParser parser("");
    EXPECT_FALSE(parser.ParseRegLivenessTag(data, offset, data.size()));
}

/**
 * |  用例集  | TlvParserUt
 * | 测试函数 | ParseRegNumberTag
 * |  用例名  | test_ParseRegNumberTag_when_register_count_differs_from_type_count_expect_false
 * | 用例描述 | 预置一种寄存器类型但输入两个寄存器数量，验证数值可完整读取但列表基数不一致时返回false
 */
TEST_F(TlvParserUt, test_ParseRegNumberTag_when_register_count_differs_from_type_count_expect_false)
{
    std::vector<uint8_t> data;
    AppendValue(data, uint64_t{64});
    AppendValue(data, uint64_t{128});
    size_t offset = 0;
    TlvParser parser("");
    parser.parsedRegLiveness_.regType = {RegType::X};

    EXPECT_FALSE(parser.ParseRegNumberTag(data, offset, data.size()));
    EXPECT_EQ(parser.parsedRegLiveness_.regNumber.size(), 2U);
}

/**
 * |  用例集  | TlvParserUt
 * | 测试函数 | ParseInstRecordRegTag
 * |  用例名  | test_ParseInstRecordRegTag_when_length_is_not_record_size_multiple_expect_false
 * | 用例描述 | 输入三字节Value且输出记录预置旧数据，验证非RecordValue整数倍长度会清空输出并返回false
 */
TEST_F(TlvParserUt, test_ParseInstRecordRegTag_when_length_is_not_record_size_multiple_expect_false)
{
    const std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    size_t offset = 0;
    std::vector<RecordValue> records = {{7, OperationType::Read}};
    TlvParser parser("");

    EXPECT_FALSE(parser.ParseInstRecordRegTag(data, offset, data.size(), records));
    EXPECT_TRUE(records.empty());
}

/**
 * |  用例集  | TlvParserUt
 * | 测试函数 | ParseInstRecordListTag
 * |  用例名  | test_ParseInstRecordListTag_when_pc_addresses_differ_between_register_types_expect_false
 * | 用例描述 | 两种寄存器各有一条PC记录但地址分别为0x1000和0x2000，验证数量合法而地址不一致时返回false
 */
TEST_F(TlvParserUt, test_ParseInstRecordListTag_when_pc_addresses_differ_between_register_types_expect_false)
{
    auto data = MakeInstRecordGroup(0x1000);
    const auto secondGroup = MakeInstRecordGroup(0x2000);
    data.insert(data.end(), secondGroup.begin(), secondGroup.end());
    size_t offset = 0;
    TlvParser parser("");
    parser.parsedRegLiveness_.regType = {RegType::X, RegType::R};
    parser.parsedRegLiveness_.instNum = 1;

    EXPECT_FALSE(parser.ParseInstRecordListTag(data, offset, data.size()));
}

/**
 * |  用例集  | TlvParserUt
 * | 测试函数 | CountPCNum
 * |  用例名  | test_CountPCNum_when_instruction_records_are_empty_expect_false_and_clear_stale_statistics
 * | 用例描述 | 预置旧PC计数和寄存器状态但不提供指令记录，验证统计入口返回false并清除历史结果
 */
TEST_F(TlvParserUt, test_CountPCNum_when_instruction_records_are_empty_expect_false_and_clear_stale_statistics)
{
    TlvParser parser("");
    parser.pcNumCount_[0x1000] = 9;
    parser.gprStatus_[0x1000] = {{"X1", OperationType::Read, 1}};

    EXPECT_FALSE(parser.CountPCNum());
    EXPECT_TRUE(parser.GetPcNumCount().empty());
    EXPECT_TRUE(parser.GetGprStatus().empty());
}

/**
 * |  用例集  | TlvParserUt
 * | 测试函数 | CountPCNum
 * |  用例名  | test_CountPCNum_when_register_has_disjoint_live_ranges_expect_lifetime_on_each_range_first_pc
 * | 用例描述 | X1在PC0单独出现并在PC2至PC3连续出现，验证每PC寄存器数及两个不连续区间起点的存活长度分别为1和2
 */
TEST_F(TlvParserUt, test_CountPCNum_when_register_has_disjoint_live_ranges_expect_lifetime_on_each_range_first_pc)
{
    TlvParser parser("");
    parser.parsedRegLiveness_.regType = {RegType::X};
    parser.parsedRegLiveness_.instNum = 4;
    parser.parsedRegLiveness_.instRecordList = {{
        {0x1000, {{1, OperationType::Read}}},
        {0x1004, {}},
        {0x1008, {{1, OperationType::Write}}},
        {0x100c, {{1, OperationType::ReadAndWrite}}}
    }};

    ASSERT_TRUE(parser.CountPCNum());
    const auto &pcNumCount = parser.GetPcNumCount();
    EXPECT_EQ(pcNumCount.at(0x1000), 1U);
    EXPECT_EQ(pcNumCount.at(0x1004), 0U);
    EXPECT_EQ(pcNumCount.at(0x1008), 1U);
    EXPECT_EQ(pcNumCount.at(0x100c), 1U);

    const auto &gprStatus = parser.GetGprStatus();
    ASSERT_EQ(gprStatus.count(0x1000), 1U);
    ASSERT_EQ(gprStatus.count(0x1008), 1U);
    ASSERT_EQ(gprStatus.at(0x1000).size(), 1U);
    ASSERT_EQ(gprStatus.at(0x1008).size(), 1U);
    EXPECT_EQ(gprStatus.at(0x1000).at(0).regIndex, "X1");
    EXPECT_EQ(gprStatus.at(0x1000).at(0).survivalTime, 1U);
    EXPECT_EQ(gprStatus.at(0x1008).at(0).regIndex, "X1");
    EXPECT_EQ(gprStatus.at(0x1008).at(0).survivalTime, 2U);
}
