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


#ifndef __MSOPPROF_PROFILING_L2CACHE_H__
#define __MSOPPROF_PROFILING_L2CACHE_H__

#include <memory>
#include <new>
#include <set>
#include "cache_defs.h"
#include "cache_policy.h"
#include "common/dbi_defs.h"
#include "log.h"

namespace Profiling {

/// cache操作的基本单位，操作地址和操作比特数
struct CacheAddrRange {
    uint64_t addr;
    uint64_t length;
    uint64_t pc;
};

struct CacheLine {
    static constexpr int CACHE_LINE_BIT_PARSE = 31;
    explicit CacheLine(uint32_t size) : size(size), bits(CACHE_LINE_BIT_PARSE - __builtin_clz(size)) {}

    uint32_t size; // 芯片的每个cache line大小，Ascend910B对应512B
    uint32_t bits; // 2^bits == size, 用于地址到Cache的组相连映射
};

/// 用于 CacheInject
struct CacheInjectStat {
    explicit CacheInjectStat(bool isvalid) : isValid(isvalid) {}

    CLID id{INVALID_CLID}; // 新插入的cache line id
    bool replaced{false};  // 是否驱逐了cache line
    bool written{false};   // 被驱逐的cache line是否被写回上一级cache，replaced == true时为有效值
    bool isValid;   // 该结构体是否有效
};

/// 用于 Load, Store，记录一次cache操作的是否命中、是否从上一级cache读取、是否写回上一级cache
struct CacheOpStat {
    CacheOpStat() = default;

    explicit CacheOpStat(bool isvalid) : isValid(isvalid) {}

    uint32_t hit{0};
    uint32_t miss{0};
    uint32_t allocate{0};
    uint32_t evictAndWrite{0};
    uint32_t evictWithoutWrite{0};
    bool isValid;   // 该结构体是否有效

    CacheOpStat operator += (const CacheOpStat &stat)
    {
        if (!stat.isValid) {
            Utility::LogDebug("Add invalid cache metric");
            return *this;
        }
        isValid = true;
        hit += stat.hit;
        miss +=  stat.miss;
        allocate +=  stat.allocate;
        evictAndWrite +=  stat.evictAndWrite;
        evictWithoutWrite +=  stat.evictWithoutWrite;
        return *this;
    }
};

class CacheBase {
public:
    CacheBase(const std::string &name, uint32_t clSize) : name_(name), cacheLine_(clSize) {}

    virtual CacheOpStat Load(const CacheAddrRange &range) = 0;
    virtual CacheOpStat Store(const CacheAddrRange &range) = 0;

    inline CLID GetCLID(uint64_t addr) const // 访问地址转换为cache line id
    {
        return addr >> cacheLine_.bits;
    }

    std::string name_;
    CacheLine cacheLine_;
};

/// 用于cache初始化
struct CacheConfig {
    std::string name;
    std::shared_ptr<CacheBase> loadFrom; // 读未命中时访问的上一级cache
    std::shared_ptr<CacheBase> storeTo;  // 写未命中时访问的上一级cache
    uint32_t sets;                       // cache为sets * ways的二维数组
    uint32_t ways;
    uint32_t cacheLineSize;
    CachePolicy::Policy policy;          // cache策略，支持LRU = 0、FIFO = 1
    bool writeBack;      // 置true表示操作cache store时，将cache line的dirty位置true，该cache line被驱逐时再写回上一级
    bool writeAllocate;  // writeBack == true时生效，控制cache store未命中时，是否直接写回上一级cache，true表示不写回
};

struct CacheMetrics {
    CacheMetrics() = default;
    explicit CacheMetrics(CLID clid) : id(clid), stat(CacheOpStat{true}) {}
    CacheMetrics operator += (const CacheMetrics &metric)
    {
        if (!metric.stat.isValid) {
            Utility::LogDebug("Add invalid cache metric");
            return *this;
        }
        stat += metric.stat;
        hitPc.insert(metric.hitPc.begin(), metric.hitPc.end());
        missPc.insert(metric.missPc.begin(), metric.missPc.end());
        return *this;
    }
    void Miss(uint64_t pc)
    {
        stat.miss++;
        missPc.insert(pc);
    }
    void Hit(uint64_t pc)
    {
        stat.hit++;
        hitPc.insert(pc);
    }
    std::set<uint64_t> hitPc {};
    std::set<uint64_t> missPc {};
    CLID id{INVALID_CLID};
    CacheOpStat stat;
};

/// 本方案为对cache的访问行为进行功能性仿真（只考虑cacheline状态的更新，不考虑时序），
///   进而确定某访问下的cacheline hit、cacheline evict、cacheline write back等情况。
/// 本方案的特点有:
///   1.模拟inclusive类型的cache，即Ascend910B L2中的所有数据都在GM中有对应的备份，即loadFrom和storeTo设置为MainMemory的实例
///   2.cache不包含victim cache，即一旦数据被换出，则会写回下一级内存（cache、gm）或丢弃
class L2Cache : public CacheBase {
public:
    explicit L2Cache(const CacheConfig &cacheConfig)
        : CacheBase(cacheConfig.name, cacheConfig.cacheLineSize),
          loadFrom_(cacheConfig.loadFrom), storeTo_(cacheConfig.storeTo),
          placement_(cacheConfig.sets), ways_(cacheConfig.ways),
          writeBack_(cacheConfig.writeBack), writeAllocate_(cacheConfig.writeAllocate)
    {
        policy_ = CachePolicyFactory::Construct(cacheConfig.policy);
        if (policy_ == nullptr) {
            throw std::bad_alloc();
        }
        clidBasedCacheData_.resize(cacheConfig.sets);
    }
    void Modeling(const std::vector<Common::MemRecord> &memoryRecords);
    const std::unordered_map<uint64_t, CacheOpStat>& GetPcBasedCacheData() const
    {
        return pcBasedCacheData_;
    }
    const std::vector<CacheMetrics>& GetCIDBasedCacheData() const
    {
        return clidBasedCacheData_;
    }

    virtual ~L2Cache() = default;
private:
    CacheOpStat Load(const CacheAddrRange &range) override;
    CacheOpStat Store(const CacheAddrRange &range) override;
    inline uint64_t GetAddrFromCLId(CLID id) const { return id << cacheLine_.bits; }
    inline uint32_t GetSetId(CLID id) const { return id % placement_.size(); }
    inline CacheAddrRange GetRangeFromCLId(CLID id) const { return {GetAddrFromCLId(id), cacheLine_.size}; }

    int32_t GetCacheLineLocation(CLID id) const;
    void Evict(const CLEntry &entry);
    CacheInjectStat Inject(const CLEntry &entry);
    void LoadMissEntry(const CLEntry &entry, CacheOpStat &opStat);

    static constexpr int32_t INVALID_LOCATION = -1;

    std::shared_ptr<CacheBase> loadFrom_;
    std::shared_ptr<CacheBase> storeTo_;
    std::shared_ptr<CachePolicy> policy_;
    std::vector<std::vector<CLEntry>> placement_;
    std::unordered_map<uint64_t, CacheOpStat> pcBasedCacheData_;
    std::vector<CacheMetrics> clidBasedCacheData_;
    uint32_t ways_;
    bool writeBack_;
    bool writeAllocate_;
};

/// 模拟中假设L2Cache会访问的所有数据都在GM有对应的备份
class GM : public CacheBase {
public:
    explicit GM(const std::string &name, uint32_t clSize) : CacheBase(name, clSize) {}
    CacheOpStat Load(const CacheAddrRange &range) override;
    CacheOpStat Store(const CacheAddrRange &range) override;
    virtual ~GM() = default;
};

std::shared_ptr<L2Cache> GetDefaultL2Cache(const std::string &socVersion);

}

#endif // __MSOPPROF_PROFILING_L2CACHE_H__
