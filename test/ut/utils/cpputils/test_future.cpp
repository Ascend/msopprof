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

#include <gtest/gtest.h>

#include "smart_pointer.h"

using namespace Utility;

struct Foo {
    Foo(bool v) : val(v) { }
    bool val;
};

TEST(Future, make_unique_with_built_in_type_expect_correct_value)
{
    auto i = MakeUnique<int>(1);
    ASSERT_EQ(*i, 1);
}

TEST(Future, make_unique_with_user_defined_class_expect_correct_object)
{
    auto foo = MakeUnique<Foo>(true);
    ASSERT_TRUE(foo->val);
}
