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


#include "l2cache.h"
#include <map>
#include "smart_pointer.h"

namespace Profiling {
int32_t L2Cache::GetCacheLineLocation(CLID id) const
{
    const std::vector<CLEntry> &vec = placement_[GetSetId(id)];
    for (size_t i = 0; i < vec.size(); ++i) {
        if (vec[i].isValid && vec[i].id == id) {
            return static_cast<int>(i);
        }
    }
    return INVALID_LOCATION;
}

void L2Cache::Evict(const CLEntry &entry)
{
    if (storeTo_) {
        (void)storeTo_->Store(GetRangeFromCLId(entry.id));
    }
}

CacheInjectStat L2Cache::Inject(const CLEntry &entry)
{
    uint32_t setId = GetSetId(entry.id);
    CLEntry replaced{};
    CacheInjectStat injectStat{false};
    if (!policy_->Inject(ways_, entry, placement_[setId], replaced)) {
        return injectStat;
    }

    injectStat.isValid = true;
    injectStat.id = entry.id;
    injectStat.replaced = true;
    if (writeBack_ && replaced.isDirty && replaced.isValid) {
        /// 有修改过的cache line写回上一级cache
        Evict(replaced);
        injectStat.written = true;
    }
    return injectStat;
}

void L2Cache::LoadMissEntry(const CLEntry &entry, CacheOpStat &opStat)
{
    if (loadFrom_) {
        (void)loadFrom_->Load(GetRangeFromCLId(entry.id));
    }
    ++opStat.allocate;
    CacheInjectStat injectStat = Inject(entry);
    if (injectStat.isValid && injectStat.replaced) {
        /// 根据被驱逐的cache是否写回了上一级cache修改统计数据
        if (injectStat.written) {
            ++opStat.evictAndWrite;
        } else {
            ++opStat.evictWithoutWrite;
        }
    }
}

CacheOpStat L2Cache::Load(const CacheAddrRange &range)
{
    CacheOpStat loadStat{true};
    CLID first = GetCLID(range.addr);
    uint64_t lastId = (range.addr + range.length) == 0 ? 0 : range.addr + range.length - 1;
    CLID last = GetCLID(lastId);
    for (CLID id = first; id <= last; ++id) {
        uint32_t setId = GetSetId(id);
        CacheMetrics cacheMetrics {setId};
        int32_t location = GetCacheLineLocation(id);
        if (setId >= clidBasedCacheData_.size()) {
            Utility::LogDebug("Receive wrong param when parse l2 cache");
            continue;
        }
        if (location == INVALID_LOCATION) {
            /// cache load miss
            ++loadStat.miss;
            cacheMetrics.Miss(range.pc);
            clidBasedCacheData_[setId] += cacheMetrics;
            LoadMissEntry({id, false, true}, loadStat);
            continue;
        }

        ++loadStat.hit;
        cacheMetrics.Hit(range.pc);
        clidBasedCacheData_[setId] += cacheMetrics;
        policy_->HitRefresh(location, placement_[setId]);
    }
    return loadStat;
}

CacheOpStat L2Cache::Store(const CacheAddrRange &range)
{
    CacheOpStat storeStat{true};
    CLID first = GetCLID(range.addr);
    uint64_t lastId = (range.addr + range.length) == 0 ? 0 : range.addr + range.length - 1;
    CLID last = GetCLID(lastId);
    for (CLID id = first; id <= last; ++id) {
        int32_t location = GetCacheLineLocation(id);
        uint32_t setId = GetSetId(id);
        CacheMetrics cacheMetrics {setId};
        if (setId >= clidBasedCacheData_.size()) {
            Utility::LogDebug("Receive wrong param when parse l2 cache");
            continue;
        }
        if (location == INVALID_LOCATION) {
            /// cache store miss
            ++storeStat.miss;
            cacheMetrics.Miss(range.pc);
            clidBasedCacheData_[setId] += cacheMetrics;
            if (writeBack_ && writeAllocate_) {
                LoadMissEntry({id, true, true}, storeStat);
            } else if (storeTo_) {
                /// 直接写回上一级cache的情况
                storeTo_->Store(GetRangeFromCLId(id));
            }
            continue;
        }

        /// cache store hit
        ++storeStat.hit;
        cacheMetrics.Hit(range.pc);
        clidBasedCacheData_[setId] += cacheMetrics;
        if (writeBack_) {
            placement_[setId][location].isDirty = true;
        } else if (storeTo_) {
            storeTo_->Store(GetRangeFromCLId(id));
        }
        policy_->HitRefresh(location, placement_[setId]);
    }
    return storeStat;
}

void L2Cache::Modeling(const std::vector<Common::MemRecord> &memoryRecords)
{
    uint32_t count = 0;
    for (auto &record : memoryRecords) {
        CacheOpStat opStat{false};
        if (record.src == Common::MemType::GM) {
            opStat = Load({record.srcAddr, record.srcMemSize, record.pc});
        } else if (record.dst == Common::MemType::GM) {
            opStat = Store({record.dstAddr, record.dstMemSize, record.pc});
        }
        if (!opStat.isValid) {
            ++count;
            pcBasedCacheData_[record.pc].isValid = false;
            continue;
        }
        if (pcBasedCacheData_.find(record.pc) == pcBasedCacheData_.end()) {
            pcBasedCacheData_.insert({record.pc, opStat});
        } else {
            if (pcBasedCacheData_[record.pc].isValid) {
                pcBasedCacheData_[record.pc] += opStat;
            }
        }
    }
    if (count != 0) {
        Utility::LogDebug("Record with invalid memory type collected, %u.", count);
    }
}

CacheOpStat GM::Load(const CacheAddrRange &range)
{
    CacheOpStat loadStat{true};
    CLID first = GetCLID(range.addr);
    uint64_t lastId = (range.addr + range.length) == 0 ? 0 : range.addr + range.length - 1;
    CLID last = GetCLID(lastId);
    loadStat.hit = last - first + 1;
    return loadStat;
}

CacheOpStat GM::Store(const CacheAddrRange &range)
{
    CacheOpStat storeStat{true};
    CLID first = GetCLID(range.addr);
    uint64_t lastId = (range.addr + range.length) == 0 ? 0 : range.addr + range.length - 1;
    CLID last = GetCLID(lastId);
    storeStat.hit = last - first + 1;
    return storeStat;
}

std::shared_ptr<L2Cache> GetDefaultL2Cache(const std::string &socVersion)
{
    // 910B CacheLineSize 512B
    constexpr uint32_t cacheLineSize = 512;
    // 910B CacheWays 24
    constexpr uint32_t ways = 24;
    // 910B4 sets 8192
    // 910B1 sets 16384
    std::map<std::string, uint32_t> chipWithSets {
            {"Ascend910B1",     16384},
            {"Ascend910B2",     16384},
            {"Ascend910B3",     16384},
            {"Ascend910B2C",    16384},
            {"Ascend910B4",     8192},
            {"Ascend910B4-1",   14336},
            {"Ascend910_9381",  16384},
            {"Ascend910_9382",  16384},
            {"Ascend910_9391",  16384},
            {"Ascend910_9392",  16384},
            {"Ascend910_9362",  14336},
            {"Ascend910_9372",  16384},
    };
    uint32_t sets = chipWithSets.at("Ascend910B1");
    if (chipWithSets.find(socVersion) != chipWithSets.end()) {
        sets = chipWithSets.at(socVersion);
    }
    std::shared_ptr<GM> gm = Utility::MakeShared<GM>("GM", cacheLineSize);
    if (gm == nullptr) {
        return nullptr;
    }
    CacheConfig cacheConfig = {"L2 Cache", gm, gm, sets,
                               ways, cacheLineSize, CachePolicy::LRU, true, true};
    return Utility::MakeShared<L2Cache>(cacheConfig);
}

}
