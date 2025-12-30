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


#include "log.h"

#include "data_center.h"

namespace Profiling {
namespace Parse {

std::set<std::type_index> DataCenter::DataTableUnRegister(const std::set<std::type_index>& activateDb)
{
    std::set<std::type_index> removedTypes;
    std::lock_guard<std::mutex> lock(dataCenterMutex_);
    for (auto it = dataTablePtrMap_.begin(); it != dataTablePtrMap_.end();) {
        if (activateDb.find(it->first) == activateDb.end()) {
            removedTypes.insert(it->first);
            it = dataTablePtrMap_.erase(it);
            continue;
        }
        ++it;
    }
    return removedTypes;
}

BaseTypePtr DataCenter::GetDbPtrByClassIdx(std::type_index idx) const
{
    std::lock_guard<std::mutex> lock(dataCenterMutex_);
    auto it = dataTablePtrMap_.find(idx);
    if (it == dataTablePtrMap_.end()) {
        return nullptr;
    }
    return it->second;
}

std::vector<bool> DataCenter::IsRequiredDbExist(const std::vector<std::type_index>& queryDbList) const
{
    std::lock_guard<std::mutex> lock(dataCenterMutex_);
    std::vector<bool> dbExistRes = {};
    for (auto dbTypeIdx: queryDbList) {
        if (dataTablePtrMap_.find(dbTypeIdx) == dataTablePtrMap_.end()) {
            dbExistRes.emplace_back(false);
            continue;
        }
        dbExistRes.emplace_back(true);
    }
    return dbExistRes;
}

}
}