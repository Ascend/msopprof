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

#include "simd_vf_utilization_calculator.h"

#include <algorithm>
#include <unordered_set>
#include <utility>
#include "log.h"
#include "smart_pointer.h"
#include "json.hpp"
#include "instr_in_camodel/camodel_vec_name_map.h"
#include "parse/data_parser/instr_parser/instr_log_parser.h"
#include "parse/data_table/instr_detail_table.h"
#include "profiling/simulator/data_parse/sim_defs.h"

namespace Profiling {
namespace Parse {
namespace {
using ExecutionKey = std::pair<uint64_t, uint64_t>;
struct ExecutionKeyHash {
    size_t operator()(const ExecutionKey &key) const {
        return std::hash<uint64_t>{}(key.first) ^ (std::hash<uint64_t>{}(key.second) << 1);
    }
};

// Decode one eligible record. Instruction selection belongs to the caller.
SimdVfUtilization CalculateSimdVfDetail(const std::string &detail) {
    SimdVfUtilization unavailable;
    unavailable.invalid = true;
    const auto json = nlohmann::json::parse(detail, nullptr, false);
    if (!json.is_object()) {
        return unavailable;
    }
    for (const auto &classification : {std::make_pair("queue_type", "SIMD"), std::make_pair("vf_class", "EXECUTION")}) {
        const auto value = json.find(classification.first);
        if (value == json.end()) {
            continue;
        }
        if (!value->is_string()) {
            return unavailable;
        }
        if (*value != classification.second) {
            return {};
        }
    }
    const auto field = json.find("processed_bytes");
    if (field == json.end() || !field->is_number_integer() ||
        (!field->is_number_unsigned() && field->get<int64_t>() < 0)) {
        return unavailable;
    }
    constexpr uint64_t VECTOR_CAPACITY_BYTES = 256;
    const auto bytes = field->get<uint64_t>();
    if (bytes > VECTOR_CAPACITY_BYTES) {
        return unavailable;
    }
    SimdVfUtilization result;
    result.processedBytes = bytes;
    result.capacityBytes = VECTOR_CAPACITY_BYTES;
    return result;
}
}

void SimdVfUtilizationCalculator::DependencyRegister() {
    RegisterPluginName("SIMD VF Vector Utilization Calculation");
    RegisterMandatoryDb({typeid(InstrLogParser), typeid(InstrDetailTable)});
    RegisterChip({ChipProductType::ASCEND950_SERIES});
}

PluginErrorCode SimdVfUtilizationCalculator::Entry() {
    const auto parser = dataCenter_.GetDbPtr<InstrLogParser>();
    const auto paired = dataCenter_.GetDbPtr<InstrDetailTable>();
    if (parser == nullptr || paired == nullptr) {
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    const auto coreNames = dataCenter_.GetDbPtr<PhysicalAndLogicalPair>();
    const auto *coreName = coreNames == nullptr ? "unknown" : coreNames->first.c_str();
    auto table = Utility::MakeShared<SimdVfResultTable>();
    if (table == nullptr) {
        Utility::LogWarn("Failed to create SIMD VF result table for %s", coreName);
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    const auto rows = paired->GetColumnData<MergeInfo>(InstrDetailTable::MERGE_INFO);
    if (rows == nullptr) {
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    // Count a paired execution once. PC keeps reused IDs at different instructions separate.
    // Do not reserve rows->size(): it also includes non-compute and duplicate timing rows.
    std::unordered_set<ExecutionKey, ExecutionKeyHash> executions;
    size_t valid = 0;
    size_t invalid = 0;
    size_t missing = 0;
    size_t excluded = 0;
    size_t duplicates = 0;
    const auto &raw = parser->GetInstrLog();
    for (const auto &row : *rows) {
        if (row.pipe != "RVECEX" || !IsSimdVfComputeInstrA5(row.name)) {
            continue;
        }
        if (!executions.emplace(row.id, row.pc).second) {
            ++duplicates;
            continue;
        }
        const auto execution = raw.find(row.id);
        if (execution == raw.end()) {
            ++missing;
            continue;
        }
        // Match instruction identity even when another record shares this ID.
        const auto instr =
            std::find_if(execution->second.rbegin(), execution->second.rend(), [&row](const InstrParseInfo &candidate) {
                return candidate.pc == row.pc && candidate.name == row.name && candidate.pipe == row.pipe;
            });
        if (instr == execution->second.rend()) {
            ++missing;
            continue;
        }
        // A final invalid value must stay invalid; an earlier sample is not its replacement.
        const auto metric = CalculateSimdVfDetail(instr->detail);
        if (metric.capacityBytes == 0 && !metric.invalid) {
            ++excluded;
            continue;
        }
        if (metric.invalid) {
            ++invalid;
        } else {
            ++valid;
        }
        table->byPc[row.pc].Add(metric);
    }
    Utility::LogDebug("SIMD VF summary core=%s paired_rows=%zu compute_executions=%zu valid=%zu invalid=%zu "
                      "missing=%zu excluded=%zu duplicate_rows=%zu output_pcs=%zu",
        coreName, rows->size(), executions.size(), valid, invalid, missing, excluded, duplicates, table->byPc.size());
    if (!table->byPc.empty() && !dataCenter_.DataTableRegister(table)) {
        Utility::LogWarn("Failed to register SIMD VF result table for %s", coreName);
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    return PluginErrorCode::SUCCESS;
}

}
}
