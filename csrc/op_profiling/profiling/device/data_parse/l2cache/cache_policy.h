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


#ifndef __MSOPPROF_PROFILING_CACHE_POLICY_H__
#define __MSOPPROF_PROFILING_CACHE_POLICY_H__

#include <memory>
#include <vector>
#include "cache_defs.h"
#include "smart_pointer.h"

namespace Profiling {
/// cache操作策略，目前支持FIFO和LRU
class CachePolicy {
public:
    enum Policy : uint32_t {
        LRU = 0,
        FIFO = 1
    };
    /// 表示插入cache line进行的操作
    virtual bool Inject(uint32_t maxSize, const CLEntry &entry, std::vector<CLEntry> &data, CLEntry &replaced) = 0;
    /// 表示访问cache line命中时的刷新操作
    virtual void HitRefresh(size_t location, std::vector<CLEntry> &data) = 0;
    virtual ~CachePolicy() = default;
};

class LRUPolicy : public CachePolicy {
public:
    bool Inject(uint32_t maxSize, const CLEntry &entry, std::vector<CLEntry> &data, CLEntry &replaced) override;
    void HitRefresh(size_t location, std::vector<CLEntry> &data) override;
};

class FIFOPolicy : public CachePolicy {
public:
    bool Inject(uint32_t maxSize, const CLEntry &entry, std::vector<CLEntry> &data, CLEntry &replaced) override;
    void HitRefresh(size_t location, std::vector<CLEntry> &data) override;
};

class NullPolicy : public CachePolicy {
public:
    bool Inject(uint32_t maxSize, const CLEntry &entry, std::vector<CLEntry> &data, CLEntry &replaced) override
    {
        return false;
    }
    void HitRefresh(size_t location, std::vector<CLEntry> &data) override {}
};

class CachePolicyFactory {
public:
    static std::shared_ptr<CachePolicy> Construct(CachePolicy::Policy policy)
    {
        switch (policy) {
            case CachePolicy::LRU:
                return std::make_shared<LRUPolicy>();
            case CachePolicy::FIFO:
                return std::make_shared<FIFOPolicy>();
            default:
                return std::make_shared<NullPolicy>();
        }
    }
};
}

#endif // __MSOPPROF_PROFILING_CACHE_POLICY_H__
