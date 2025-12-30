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

#include <algorithm>

#include "thread_safe_unordered_map.h"

using namespace Utility;
using namespace std;

std::unordered_map<string, int> sampleMapData = {
        {"data1", 1}, {"data2", 2}, {"data3", 3}
};

std::unordered_map<string, int> sampleMapData2 = {
        {"data1", 3}, {"data2", 2}, {"data3", 1}, {"data4", 10}
};

TEST(ThreadSafeUnorderedMap, test_ThreadSafeUnorderedMap_index_operator_with_instantiate_by_map_expect_success)
{
    // instantiate by map
    ThreadSafeUnorderedMap<string, int> tsMap(sampleMapData);

    // test [] operator
    ASSERT_EQ(tsMap["data1"], sampleMapData["data1"]);
    ASSERT_EQ(tsMap["data3"], sampleMapData["data3"]);
}

TEST(ThreadSafeUnorderedMap, test_ThreadSafeUnorderedMap_index_operator_with_instantiate_by_object_expect_success)
{
    ThreadSafeUnorderedMap<string, int> tsMap(sampleMapData);
    // instantiate by object
    ThreadSafeUnorderedMap<string, int> tsMap2(tsMap);

    // test [] operator
    ASSERT_EQ(tsMap2["data1"], sampleMapData["data1"]);
    ASSERT_EQ(tsMap2["data3"], sampleMapData["data3"]);
}

TEST(ThreadSafeUnorderedMap, test_ThreadSafeUnorderedMap_assign_value_operator_expect_success)
{
    // test = operator
    ThreadSafeUnorderedMap<string, int> tsMap(sampleMapData);
    tsMap = sampleMapData2;
    ASSERT_EQ(tsMap["data1"], sampleMapData2["data1"]);
    ASSERT_EQ(tsMap["data4"], sampleMapData2["data4"]);
}

TEST(ThreadSafeUnorderedMap, test_ThreadSafeUnorderedMap_Size_func_expect_success)
{
    ThreadSafeUnorderedMap<string, int> tsMap(sampleMapData2);
    //test Size func
    ASSERT_EQ(tsMap.Size(), 4);
}

TEST(ThreadSafeUnorderedMap, test_ThreadSafeUnorderedMap_Empty_func_expect_success)
{
    ThreadSafeUnorderedMap<string, int> tsMap;
    //test Empty func
    ASSERT_TRUE(tsMap.Empty());

    tsMap = sampleMapData2;
    ASSERT_FALSE(tsMap.Empty());
}

TEST(ThreadSafeUnorderedMap, test_ThreadSafeUnorderedMap_Count_func_expect_success)
{
    ThreadSafeUnorderedMap<string, int> tsMap(sampleMapData2);
    //test Count func
    ASSERT_EQ(tsMap.Count("data4"), 1);
    ASSERT_EQ(tsMap.Count("data5"), 0);
}

TEST(ThreadSafeUnorderedMap, test_ThreadSafeUnorderedMap_Erase_func_expect_success)
{
    ThreadSafeUnorderedMap<string, int> tsMap(sampleMapData2);
    //test Erase func
    tsMap.Erase("data4");
    ASSERT_EQ(tsMap.Count("data4"), 0);
    ASSERT_EQ(tsMap.Size(), 3);
}

TEST(ThreadSafeUnorderedMap, test_ThreadSafeUnorderedMap_Insert_func_expect_success)
{
    ThreadSafeUnorderedMap<string, int> tsMap(sampleMapData);
    //test Insert func
    int newValue = 9;
    tsMap.Insert("data4", newValue);
    ASSERT_EQ(tsMap.Count("data4"), 1);
    ASSERT_EQ(tsMap["data4"], newValue);

    string newKey = "data5";
    tsMap.Insert(make_pair(newKey, newValue));
    ASSERT_EQ(tsMap.Count(newKey), 1);
    ASSERT_EQ(tsMap[newKey], newValue);
}

TEST(ThreadSafeUnorderedMap, test_ThreadSafeUnorderedMap_Find_func_expect_success)
{
    ThreadSafeUnorderedMap<string, int> tsMap(sampleMapData2);
    //test Find func
    int resValue = 0;
    ASSERT_TRUE(tsMap.Find("data4", resValue));
    ASSERT_EQ(resValue, sampleMapData2["data4"]);
    ASSERT_FALSE(tsMap.Find("data6", resValue));
    ASSERT_EQ(resValue, sampleMapData2["data4"]);

    ASSERT_TRUE(tsMap.Find("data4"));
    ASSERT_FALSE(tsMap.Find("data5"));
}

TEST(ThreadSafeUnorderedMap, test_ThreadSafeUnorderedMap_FindAndInsertIfNotExist_func_expect_success)
{
    ThreadSafeUnorderedMap<string, int> tsMap(sampleMapData2);
    //test FindAndInsertIfNotExist func
    int insertValue = -1;
    ASSERT_TRUE(tsMap.FindAndInsertIfNotExist("data4", insertValue));
    ASSERT_EQ(tsMap["data4"], sampleMapData2["data4"]);
    ASSERT_FALSE(tsMap.FindAndInsertIfNotExist("data5", insertValue));
    ASSERT_EQ(tsMap["data5"], insertValue);
}

TEST(ThreadSafeUnorderedMap, test_ThreadSafeUnorderedMap_Clear_func_expect_success)
{
    ThreadSafeUnorderedMap<string, int> tsMap(sampleMapData);
    //test Clear func
    tsMap.Clear();
    ASSERT_TRUE(tsMap.Empty());
}

TEST(ThreadSafeUnorderedMap, test_ThreadSafeUnorderedMap_iterator_func_expect_success)
{
    ThreadSafeUnorderedMap<string, int> tsMap(sampleMapData2);

    for (auto keyValuePair : tsMap) {
        ASSERT_EQ(keyValuePair.second, sampleMapData2[keyValuePair.first]);
    }
}
