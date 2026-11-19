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

#include <gtest/gtest.h>
#include <limits>
#include <string>
#include "tlv_reader.h"

using Utility::TlvReader;
namespace {
template <typename T>
void Append(std::vector<uint8_t> &data, T value)
{
    const auto *bytes = reinterpret_cast<const uint8_t *>(&value);
    data.insert(data.end(), bytes, bytes + sizeof(T));
}
template <typename Tag = uint32_t, typename Length = uint64_t>
void Record(std::vector<uint8_t> &data, Tag tag, const std::vector<uint8_t> &value)
{
    Append(data, tag);
    Append(data, static_cast<Length>(value.size()));
    data.insert(data.end(), value.begin(), value.end());
}
} // namespace

TEST(TlvReader, LegacyPackedHeaderAndUnalignedPayload)
{
    std::vector<uint8_t> data = {0xff};
    Record(data, uint32_t{11}, {7, 8});
    size_t offset = 1;
    uint32_t tag = 0;
    uint64_t length = 0;
    ASSERT_TRUE(TlvReader::ReadHeader(data, offset, data.size(), tag, length));
    EXPECT_EQ(tag, 11U);
    EXPECT_EQ(length, 2U);
    EXPECT_EQ(offset, 13U); // No struct padding between uint32 tag and uint64 length.
    uint8_t byte = 0;
    ASSERT_TRUE(TlvReader::ReadValue(data, offset, data.size(), byte));
    EXPECT_EQ(byte, 7U);
}

TEST(TlvReader, TruncatedHeadersNeverReachHandler)
{
    for (size_t size = 1; size < 12; ++size) {
        const std::vector<uint8_t> data(size, 0);
        size_t offset = 0;
        EXPECT_FALSE((TlvReader::ForEach<uint32_t, uint64_t>(data, offset, size,
            [](uint32_t, size_t &, uint64_t) { ADD_FAILURE(); return true; })));
    }
}

TEST(TlvReader, OversizedLengthCannotWrapAround)
{
    std::vector<uint8_t> data;
    Append(data, uint32_t{11});
    Append(data, UINT64_MAX);
    size_t offset = 0;
    uint32_t tag = 0;
    uint64_t length = 0;
    EXPECT_FALSE(TlvReader::ReadHeader(data, offset, data.size(), tag, length));
    EXPECT_EQ(tag, 11U);
    EXPECT_EQ(length, UINT64_MAX);
    EXPECT_EQ(offset, 12U);
}

TEST(TlvReader, PayloadFailureDoesNotConsumeOffsetOrOverwriteValue)
{
    const std::vector<uint8_t> data = {42};
    for (size_t begin : {size_t{0}, size_t{1}, SIZE_MAX}) {
        for (size_t end : {size_t{0}, size_t{1}, SIZE_MAX}) {
            size_t offset = begin;
            uint64_t value = 123;
            EXPECT_FALSE(TlvReader::ReadValue(data, offset, end, value));
            EXPECT_EQ(offset, begin);
            EXPECT_EQ(value, 123U);
        }
    }
}

TEST(TlvReader, ReadValueBypassesOverloadedAddressOperator)
{
    struct Byte {
        uint8_t value = 0;
        uint8_t *operator&()
        {
            static uint8_t other = 7;
            return &other;
        }
    } output;
    const std::vector<uint8_t> data{42};
    size_t offset = 0;
    ASSERT_TRUE(TlvReader::ReadValue(data, offset, data.size(), output));
    EXPECT_EQ(offset, 1U);
    EXPECT_EQ(output.value, 42U);
    EXPECT_EQ(*(&output), 7U);
}

TEST(TlvReader, RepeatedUnknownTagsAndEmptyValuesAreCallerPolicy)
{
    std::vector<uint8_t> data;
    Record(data, uint32_t{99}, {});
    Record(data, uint32_t{99}, {42});
    size_t offset = 0;
    std::vector<uint64_t> lengths;
    ASSERT_TRUE((TlvReader::ForEach<uint32_t, uint64_t>(data, offset, data.size(),
        [&](uint32_t tag, size_t &valueOffset, uint64_t length) {
            EXPECT_EQ(tag, 99U);
            lengths.push_back(length);
            valueOffset += static_cast<size_t>(length);
            return true;
        })));
    EXPECT_EQ(lengths, (std::vector<uint64_t>{0, 1}));
    EXPECT_EQ(offset, data.size());
}

TEST(TlvReader, HandlerMustConsumeExactlyOneValueAndCanReject)
{
    std::vector<uint8_t> data;
    Record(data, uint32_t{1}, {42});
    Record(data, uint32_t{2}, {43});
    for (size_t consume : {size_t{0}, size_t{2}}) {
        size_t offset = 0;
        size_t calls = 0;
        EXPECT_FALSE((TlvReader::ForEach<uint32_t, uint64_t>(data, offset, data.size(),
            [&](uint32_t, size_t &valueOffset, uint64_t) { ++calls; valueOffset += consume; return true; })));
        EXPECT_EQ(calls, 1U);
    }
    size_t offset = 0;
    EXPECT_FALSE((TlvReader::ForEach<uint32_t, uint64_t>(data, offset, data.size(),
        [](uint32_t, size_t &, uint64_t) { return false; })));
    EXPECT_EQ(offset, 12U);
}

TEST(TlvReader, ChildCannotReadBytesFromFollowingOuterSibling)
{
    std::vector<uint8_t> child;
    Append(child, uint32_t{2});
    Append(child, uint64_t{1}); // Missing value inside the parent.
    std::vector<uint8_t> data;
    Record(data, uint32_t{1}, child);
    Record(data, uint32_t{3}, {42});
    size_t offset = 0;
    EXPECT_FALSE((TlvReader::ForEach<uint32_t, uint64_t>(data, offset, data.size(),
        [&](uint32_t, size_t &valueOffset, uint64_t length) {
            return TlvReader::ForEach<uint32_t, uint64_t>(data, valueOffset, valueOffset + length,
                [](uint32_t, size_t &, uint64_t) { ADD_FAILURE(); return true; });
        })));
}

TEST(TlvReader, NewBusinessCanUseNarrowHeadersAndExplicitNesting)
{
    std::vector<uint8_t> fields;
    Record<uint16_t, uint16_t>(fields, 51, {'o', 'p'});
    Record<uint16_t, uint16_t>(fields, 52, {7});
    std::vector<uint8_t> data;
    Record(data, uint32_t{100}, fields);
    size_t offset = 0;
    std::string name;
    uint8_t count = 0;
    ASSERT_TRUE((TlvReader::ForEach<uint32_t, uint64_t>(data, offset, data.size(),
        [&](uint32_t tag, size_t &valueOffset, uint64_t length) {
            return tag == 100 && TlvReader::ForEach<uint16_t, uint16_t>(data, valueOffset, valueOffset + length,
                [&](uint16_t field, size_t &begin, uint16_t size) {
                    if (field == 51) {
                        name.assign(reinterpret_cast<const char *>(data.data() + begin), size);
                        begin += size;
                        return true;
                    }
                    return field == 52 && size == 1 && TlvReader::ReadValue(data, begin, begin + size, count);
                });
        })));
    EXPECT_EQ(name, "op");
    EXPECT_EQ(count, 7U);
}

TEST(TlvReader, EmptyContainersAndInvalidRanges)
{
    const std::vector<uint8_t> data = {0};
    const auto handler = [](uint32_t, size_t &, uint64_t) { ADD_FAILURE(); return true; };
    size_t offset = 0;
    EXPECT_TRUE((TlvReader::ForEach<uint32_t, uint64_t>(data, offset, 0, handler)));
    EXPECT_FALSE((TlvReader::ForEach<uint32_t, uint64_t>(data, offset, 2, handler)));
    offset = SIZE_MAX;
    EXPECT_FALSE((TlvReader::ForEach<uint32_t, uint64_t>(data, offset, data.size(), handler)));
}
