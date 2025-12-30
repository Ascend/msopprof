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


#ifndef MSOPT_UMASK_GUARD_H
#define MSOPT_UMASK_GUARD_H
#include <sys/stat.h>
#include <sys/types.h>

namespace Utility {
class UmaskGuard {
public:
    explicit UmaskGuard(mode_t mask) noexcept : oldUmask_(umask(mask)) {}
    UmaskGuard(const UmaskGuard &) = delete;
    UmaskGuard &operator=(const UmaskGuard &) = delete;
    UmaskGuard(UmaskGuard &&) = delete;
    UmaskGuard &operator=(UmaskGuard &&) = delete;
    ~UmaskGuard() { umask(oldUmask_); }

private:
    mode_t oldUmask_;
};
}  // namespace Utility
#endif // MSOPT_UMASK_GUARD_H
