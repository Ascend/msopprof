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


#ifndef __MSOPPROF_PROFILING_CACHE_DEFS_H__
#define __MSOPPROF_PROFILING_CACHE_DEFS_H__

#include <string>

namespace Profiling {
/// 地址映射后的cache line id
using CLID = uint64_t;

constexpr CLID INVALID_CLID = 0xffffffffffffffff;

/// cache line存放的数据，只关心cache逻辑处理相关的标志位，而不是具体数据内容
struct CLEntry {
    CLID id;
    bool isDirty; // 是否被修改过
    bool isValid; // 数据是否有效，模拟时默认没有无效数据
};
}
#endif // __MSOPPROF_PROFILING_CACHE_DEFS_H__
