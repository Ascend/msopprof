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


#ifndef __CPPUTILS_MAKE_SHATED_H__
#define __CPPUTILS_MAKE_SHATED_H__

#include <memory>
#include "log.h"

namespace Utility {

template <typename T, typename... Args>
inline std::shared_ptr<T> MakeShared(Args &&... args)
{
    try {
        return std::make_shared<T>(std::forward<Args>(args)...);
    } catch (...) {
        Utility::LogDebug("Make shared failed");
    }
    return nullptr;
}

template <typename T, typename... Args>
inline std::unique_ptr<T> MakeUnique(Args &&... args)
{
    try {
        return std::unique_ptr<T>{new T(std::forward<Args>(args)...)};
    } catch (const std::exception& e) {
        Utility::LogDebug("Make unique failed, exception caught: %s", e.what());
    }
    return nullptr;
}

}

#endif  // __CPPUTILS_MAKE_SHATED_H__
