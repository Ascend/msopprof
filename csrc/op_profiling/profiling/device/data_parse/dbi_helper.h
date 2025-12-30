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

#ifndef MSOPT_DBIHELPER_H
#define MSOPT_DBIHELPER_H
#include <unordered_map>
#include "singleton.h"

namespace Profiling {

// 实现指令计数、日志打印
class DBIHelper : public Singleton<DBIHelper> {
friend class Singleton<DBIHelper>;
public:
    template <typename T>
    void PrintRecord(const T &record) const;

private:
    DBIHelper() {};
    ~DBIHelper() {};
    DBIHelper(DBIHelper const &) = delete;
    DBIHelper &operator=(DBIHelper const &) = delete;
};

}
#endif