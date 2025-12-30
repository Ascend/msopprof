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
#include <fstream>
#include "mockcpp/mockcpp.hpp"

#include "data_save.h"
#include "data_format.h"

using namespace Utility;

TEST(DataSave, save_expect_success_with_overwrite)
{
    DataSave dataSave;
    std::string saveFilePath = "./test.csv";
    bool appendMode = false;
    std::vector<std::vector<std::string>> saveData = {{"1", "2"}};
    std::vector<std::vector<std::string>> saveData2 = {{"3", "4"}};
    dataSave.Save(saveFilePath, saveData, appendMode);

    std::ifstream inputFile(saveFilePath);
    std::string singleLine;
    getline(inputFile, singleLine);
    ASSERT_EQ(singleLine, "1,2,");
    inputFile.close();

    dataSave.Save(saveFilePath, saveData2, appendMode);
    inputFile.open(saveFilePath);
    getline(inputFile, singleLine);
    ASSERT_EQ(singleLine, "3,4,");
    inputFile.close();

    std::remove(saveFilePath.c_str());
}

TEST(DataSave, save_expect_success_with_append)
{
    DataSave dataSave;
    std::string saveFilePath = "./test.csv";
    bool appendMode = true;
    std::vector<std::vector<std::string>> saveData = {{"1", "2"}};
    std::vector<std::vector<std::string>> saveData2 = {{"3", "4"}};
    dataSave.Save(saveFilePath, saveData, appendMode);
    dataSave.Save(saveFilePath, saveData2, appendMode);


    std::ifstream inputFile(saveFilePath);
    std::string singleLine;
    getline(inputFile, singleLine);
    ASSERT_EQ(singleLine, "1,2,");
    getline(inputFile, singleLine);
    ASSERT_EQ(singleLine, "3,4,");
    inputFile.close();

    std::remove(saveFilePath.c_str());
}

TEST(DataSave, csv_save_expect_success)
{
    DataSave dataSave;
    CsvFileStruct testDataStruct;
    testDataStruct.fileName = "./test.csv";
    testDataStruct.headers = {"a", "b", "c"};
    testDataStruct.data.push_back({"1", "2", "3"});
    testDataStruct.data.push_back({"3", "4", "10"});
    testDataStruct.headerIndex["a"] = 0;
    testDataStruct.headerIndex["b"] = 1;
    testDataStruct.headerIndex["c"] = 2;
    testDataStruct.valid = true;
    ASSERT_TRUE(dataSave.CsvSave(testDataStruct));

    std::ifstream inputFile(testDataStruct.fileName);
    std::string singleLine;
    getline(inputFile, singleLine);
    ASSERT_EQ(singleLine, "a,b,c,");
    getline(inputFile, singleLine);
    ASSERT_EQ(singleLine, "1,2,3,");
    getline(inputFile, singleLine);
    ASSERT_EQ(singleLine, "3,4,10,");
    inputFile.close();

    std::remove(testDataStruct.fileName.c_str());
}

TEST(DataSave, csv_save_expect_failed)
{
    DataSave dataSave;
    CsvFileStruct testDataStruct;
    testDataStruct.fileName = "./test.csv";
    testDataStruct.headers = {"a", "b", "c"};
    testDataStruct.data.push_back({"1", "2", "3"});
    testDataStruct.data.push_back({"3", "4", "10"});
    testDataStruct.headerIndex["a"] = 0;
    testDataStruct.headerIndex["b"] = 1;
    testDataStruct.headerIndex["c"] = 2;
    testDataStruct.valid = true;
    MOCKER(&chmod)
        .stubs()
        .will(returnValue(-1));
    ASSERT_FALSE(dataSave.CsvSave(testDataStruct));
}
