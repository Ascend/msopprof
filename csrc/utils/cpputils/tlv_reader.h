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

#ifndef MSOPPROF_TLV_READER_H
#define MSOPPROF_TLV_READER_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

namespace Utility {
// Packed, native-endian T-L-V framing. Length counts only value bytes.
// Header widths are selected by Tag/Length; business tags are caller-owned.
class TlvReader {
public:
    template <typename T>
    static bool ReadValue(const std::vector<uint8_t> &data, size_t &offset, size_t end, T &value)
    {
        static_assert(std::is_trivially_copyable<T>::value, "TLV values must be trivially copyable");
        if (!IsValidRange(data, offset, end) || sizeof(T) > end - offset) {
            return false;
        }
        std::copy_n(data.data() + offset, sizeof(T), reinterpret_cast<unsigned char *>(std::addressof(value)));
        offset += sizeof(T);
        return true;
    }

    // Failure may consume a complete tag or length, matching legacy cursors.
    // Never add a wire length before checking it against remaining bytes.
    template <typename Tag, typename Length>
    static bool ReadHeader(const std::vector<uint8_t> &data, size_t &offset, size_t end,
                           Tag &tag, Length &length)
    {
        static_assert(std::is_integral<Tag>::value || std::is_enum<Tag>::value, "Integral TLV tag required");
        static_assert(std::is_integral<Length>::value && std::is_unsigned<Length>::value,
                      "Unsigned TLV length required");
        return ReadValue(data, offset, end, tag) && ReadValue(data, offset, end, length) && length <= end - offset;
    }

    // Handler(tag, valueOffset, valueLength) decodes one value in place.
    // It must consume exactly that value; false stops traversal immediately.
    // Nesting is explicit: call ForEach with the enclosing value's end.
    template <typename Tag, typename Length, typename Handler, typename HeaderError = void (*)(size_t, size_t)>
    static bool ForEach(const std::vector<uint8_t> &data, size_t &offset, size_t end, Handler handler,
                        HeaderError onHeaderError = [](size_t, size_t) {})
    {
        if (!IsValidRange(data, offset, end)) {
            return false;
        }
        while (offset < end) {
            const size_t headerOffset = offset;
            Tag tag{};
            Length length{};
            if (!ReadHeader(data, offset, end, tag, length)) {
                onHeaderError(headerOffset, end);
                return false;
            }
            const size_t valueEnd = offset + static_cast<size_t>(length);
            if (!handler(tag, offset, length) || offset != valueEnd) {
                return false;
            }
        }
        return true;
    }

private:
    static bool IsValidRange(const std::vector<uint8_t> &data, size_t begin, size_t end)
    {
        return begin <= end && end <= data.size();
    }
};
} // namespace Utility

#endif // MSOPPROF_TLV_READER_H
