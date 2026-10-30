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

#ifndef MSOPT_INSTR_BINARY_H
#define MSOPT_INSTR_BINARY_H

#include <cstdint>
#include <functional>

namespace Encode {
enum class EncodingType {
    BIT32 = 0,
    BIT64,
    BIT128,
    INVALID,
};

struct InstrBinary {
    // Numerical order: words[0] contains bits 31:0, independent of host endianness.
    uint32_t words[4] = {};
    EncodingType bitType = EncodingType::INVALID;

    bool Matches(const InstrBinary &other, EncodingType type = EncodingType::BIT128) const noexcept
    {
        unsigned int count = 0;
        switch (type) {
            case EncodingType::BIT32: count = 1; break;
            case EncodingType::BIT64: count = 2; break;
            case EncodingType::BIT128: count = 4; break;
            default: return false;
        }
        for (unsigned int i = 0; i < count; ++i) {
            if (words[i] != other.words[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator==(const InstrBinary &other) const noexcept
    {
        return Matches(other);
    }

    bool empty() const noexcept
    {
        return bitType == EncodingType::INVALID;
    }
};

struct InstrBinaryHash {
    size_t operator()(const InstrBinary &binary) const noexcept
    {
        size_t seed = 0;
        for (uint32_t word : binary.words) {
            seed ^= std::hash<uint32_t>{}(word) + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
        }
        return seed;
    }
};
}
#endif // MSOPT_INSTR_BINARY_H
