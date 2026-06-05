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


#include "scalar_calculator.h"

namespace Profiling {
namespace Parse {

PluginErrorCode ScalarCalculator::Entry()
{
    auto icacheDetailTable = dataCenter_.GetDbPtr<scalarHeadCache>();
    auto scalarDetailTable = dataCenter_.GetDbPtr<scalarHead>();
    auto instrDetailTable = dataCenter_.GetDbPtr<InstrDetailTable>();
    if (!icacheDetailTable || !scalarDetailTable || !instrDetailTable) {
        Utility::LogDebug("MergeScalarInfo failed: nullptr table (icache=%d, scalar=%d, instr=%d)",
                        icacheDetailTable != nullptr, scalarDetailTable != nullptr, instrDetailTable != nullptr);
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    MergeScalar(*icacheDetailTable, *scalarDetailTable);
    const auto& mergeInfo = *instrDetailTable->GetColumnData<MergeInfo>(InstrDetailTable::MERGE_INFO);
    const size_t mergeSize = mergeInfo.size();
    std::vector<uint64_t> icacheTicks(mergeSize, UINT64_MAX);
    std::vector<uint64_t> ccuTicks(mergeSize, UINT64_MAX);

    for (size_t idx = 0; idx < mergeSize;) {
        const uint64_t currentPc = mergeInfo[idx].pc;

        // 若scalar中不存在该pc，跳过该pc解析
        auto scalarIt = scalarDetailTable->find(currentPc);
        if (scalarIt == scalarDetailTable->end()) {
            idx = std::find_if(mergeInfo.begin() + idx, mergeInfo.end(),
                            [currentPc](const MergeInfo& info) { return info.pc != currentPc; }) - mergeInfo.begin();
            continue;
        }

        const auto& scalarEntries = scalarIt->second;
        size_t scalarIdx = 0;
        while (idx < mergeSize && mergeInfo[idx].pc == currentPc) {
            if (scalarIdx >= scalarEntries.size() || scalarEntries[scalarIdx].ccuTick > mergeInfo[idx].startTick) {
                // 当前PC解析的scalalr有问题，跳过该pc去解析下一个
                idx = std::find_if(mergeInfo.begin() + idx, mergeInfo.end(),
                                [currentPc](const MergeInfo& info) { return info.pc != currentPc; }) - mergeInfo.begin();
                break;
            }
            ccuTicks[idx] = scalarEntries[scalarIdx].ccuTick;
            icacheTicks[idx] = scalarEntries[scalarIdx].icacheTick;
            ++idx;
            ++scalarIdx;
        }
    }
    instrDetailTable->UpdateColumnAllValue(InstrDetailTable::ICACHE_CYC, icacheTicks);
    instrDetailTable->UpdateColumnAllValue(InstrDetailTable::CCU_CYC, ccuTicks);
    return PluginErrorCode::SUCCESS;
}

void ScalarCalculator::MergeScalar(const scalarHeadCache &icacheDetailTable, scalarHead &scalarDetailTable)
{
    for (auto &instr : scalarDetailTable) {
        uint64_t pc = instr.first;
        std::vector<ScalarInstrInfo>& scalarVec = instr.second;
        // 寻找ifu相同pc中距离ccu最近的1条指令
        auto iter = icacheDetailTable.find(pc);
        if (iter == icacheDetailTable.end()) {
            continue;
        }
        const auto icacheSets = iter->second;
        for (auto i = 0; i < scalarVec.size(); i++) {
            // 使用 lower_bound 找到第一个 >= ccu中tick的元素
            auto it = icacheSets.lower_bound(scalarVec[i].ccuTick);
            if (it == icacheSets.begin()) {
                break;
            }
            // 离ccu中tick的最近元素，即是预取被使用的真实指令
            auto prev_it = std::prev(it);
            scalarVec[i].icacheTick = *prev_it;
        }
    }
}
}
}
