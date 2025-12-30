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

#include "pc_process.h"
#include <vector>
#include <string>
#include "log.h"
#include "ustring.h"
namespace Utility {
constexpr uint8_t PC_ADDR_ESS_GAP = 4;

std::vector<std::vector<std::string>> MergeAddrRange(const std::set<uint64_t> &addrSet, uint64_t startPc)
{
    if (addrSet.empty()) {
        return {};
    }
    const uint8_t addrSize{8U};
    std::vector<std::vector<std::string>> addrRange{};
    std::vector<uint64_t> pcList(addrSet.begin(), addrSet.end());
    if (UINT64_MAX - startPc < pcList[0]) {
        Utility::LogDebug("Add overflow, can not add startPc");
        return {};
    }
    std::vector<std::string> pcListTemp = {Utility::NumToHexString(pcList[0] + startPc, addrSize),
                                           Utility::NumToHexString(pcList[0] + startPc, addrSize)};
    addrRange.emplace_back(pcListTemp);
    for (size_t i = 1; i < pcList.size(); i++) {
        uint64_t pc = pcList[i];
        uint64_t pcCurMax;
        if (!Utility::StoullConverter(addrRange.back()[1], pcCurMax, RADIX_16)) {
            Utility::LogDebug("Failed to convert pcCurMax string to uint64");
            return {};
        }
        if (UINT64_MAX - startPc >= pc && UINT64_MAX - startPc >= pcCurMax) {
            pc += startPc;
            pcCurMax += startPc;
        } else {
            return {};
        }
        if (pc - pcCurMax <= PC_ADDR_ESS_GAP) {
            addrRange.back()[1] = Utility::NumToHexString(pc, addrSize);
        } else {
            pcListTemp = {Utility::NumToHexString(pc, addrSize), Utility::NumToHexString(pc, addrSize)};
            addrRange.emplace_back(pcListTemp);
        }
    }
    return addrRange;
}

std::vector<std::vector<std::string>> MergeAddrRange(const std::set<std::string> &addrSet, uint64_t startPc)
{
    std::set<uint64_t> addrStringSet;
    for (const auto &addr: addrSet) {
        uint64_t pc;
        if (!Utility::StoullConverter(addr, pc, RADIX_16)) {
            Utility::LogDebug("Failed to convert pc string to uint64");
            continue;
        }
        addrStringSet.insert(pc);
    }
    return MergeAddrRange(addrStringSet, startPc);
}
}