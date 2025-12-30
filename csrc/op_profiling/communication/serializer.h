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


#ifndef MSOPT_SERIALIZER_H
#define MSOPT_SERIALIZER_H

#include <string>
#include <type_traits>
#include <algorithm>

namespace Communication {
template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
inline std::string Serialize(const T &val)
{
    constexpr std::size_t size = sizeof(T);
    std::string msg(static_cast<char const *>(static_cast<void const *>(&val)), size);
    return msg;
}

template<typename T, typename... Ts, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
inline std::string Serialize(const T &val, const Ts &... vals)
{
    return Serialize(val) + Serialize(vals...);
}

template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
inline bool Deserialize(const std::string &msg, T &val)
{
    constexpr std::size_t size = sizeof(T);
    if (msg.size() < size) {
        return false;
    }
    std::copy_n(msg.data(), size, static_cast<char *>(static_cast<void *>(&val)));
    return true;
}

}
#endif  // MSOPT_SERIALIZER_H
