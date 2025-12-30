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


#include "cache_policy.h"

namespace Profiling {
bool LRUPolicy::Inject(uint32_t maxSize, const CLEntry &entry, std::vector<CLEntry> &data, CLEntry &replaced)
{
    data.insert(data.begin(), entry);
    if (data.size() > maxSize) {
        replaced = data.back();
        data.pop_back();
        return true;
    }
    return false;
}

void LRUPolicy::HitRefresh(size_t location, std::vector<CLEntry> &data)
{
    if (location >= data.size()) {
        Utility::LogDebug("Receive wrong param when parse cache policy in LRU");
        return;
    }
    CLEntry entry = data[location];
    for (size_t i = location; i > 0; --i) {
        data[i] = data[i - 1];
    }
    data[0] = entry;
}

bool FIFOPolicy::Inject(uint32_t maxSize, const CLEntry &entry, std::vector<CLEntry> &data, CLEntry &replaced)
{
    data.emplace_back(entry);
    if (data.size() > maxSize) {
        replaced = data[0];
        data.erase(data.begin());
        return true;
    }
    return false;
}

void FIFOPolicy::HitRefresh(size_t location, std::vector<CLEntry> &data)
{
    if (location >= data.size()) {
        Utility::LogDebug("Receive wrong param when parse cache policy in FIFO");
        return;
    }
    CLEntry entry = data[location];
    for (size_t i = location + 1; i < data.size(); ++i) {
        data[i - 1] = data[i];
    }
    data.back() = entry;
}
}