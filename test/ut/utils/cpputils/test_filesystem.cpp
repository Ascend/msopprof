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

#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <experimental/filesystem>
#include "mockcpp/mockcpp.hpp"

#include "filesystem.h"

using namespace Utility;

TEST(FileSystem, get_current_working_dir_expect_success)
{
    std::string dir;
    ASSERT_TRUE(GetCurrentWorkingDir(dir));
}

TEST(FileSystem, change_working_dir_to_current_dir_expect_success_and_equal_to_origin)
{
    std::string current;
    GetCurrentWorkingDir(current);
    ASSERT_TRUE(ChangeWorkingDir(current));
    std::string after;
    ASSERT_TRUE(GetCurrentWorkingDir(after));
    ASSERT_EQ(current, after);
}

TEST(FileSystem, list_dir_with_not_exist_path_expect_return_false)
{
    std::vector<std::string> dirs;
    ASSERT_FALSE(ListDir("not_exist_dir", std::back_inserter(dirs)));
}

TEST(FileSystem, list_dir_with_dev_dir_expect_non_empty_dirs)
{
    std::vector<std::string> dirs;
    ASSERT_TRUE(ListDir("/dev/", std::back_inserter(dirs)));
    ASSERT_FALSE(dirs.empty());
}

TEST(FileSystem, join_path_with_comparing_result_with_expect_string)
{
    std::string targetString = "test/ut/resources";
    ASSERT_EQ(JoinPath({"test", "ut", "resources"}), targetString);
}

TEST(FileSystem, is_exit_expect_success)
{
    std::string targetPath = "test/ut/CMakeLists.txt";
    ASSERT_TRUE(IsExist(targetPath));
}

TEST(FileSystem, is_exit_expect_return_false)
{
    GlobalMockObject::verify();
    std::string targetPath = "test/ut/fail";
    ASSERT_FALSE(IsExist(targetPath));
}

TEST(FileSystem, is_readable_expect_success)
{
    std::string targetPath = "test/ut/CMakeLists.txt";
    ASSERT_TRUE(IsReadable(targetPath));
}

TEST(FileSystem, is_readable_expect_return_false)
{
    std::string targetPath = "test/ut/no_exist.txt";
    ASSERT_FALSE(IsReadable(targetPath));
}

TEST(FileSystem, is_dir_expect_success)
{
    std::string targetPath = "test/ut";
    ASSERT_TRUE(IsDir(targetPath));
}

TEST(FileSystem, is_softlink_expect_false)
{
    std::string targetPath = "/////.////..///";
    std::string targetPath2 = "/";
    std::string targetPath3 = "./";
    ASSERT_FALSE(IsSoftLink(targetPath));
    ASSERT_FALSE(IsSoftLink(targetPath2));
    ASSERT_FALSE(IsSoftLink(targetPath3));
}

TEST(FileSystem, file_handle_utils)
{
    const char *casefile = "test/ut/resources/op_test/add_custom/test_add_custom.json";
    size_t size = 32768;
    char *buffer;
    buffer = (char *)malloc(size);
    ASSERT_TRUE(buffer);
    std::string inputPath = "test/ut/resources/op_test/add_custom/input_x.bin";
    EXPECT_TRUE(ReadFile(inputPath, (uint8_t *)buffer, size));

    std::string outputPath = "test/ut/resources/op_test/add_custom/test_output.bin";
    EXPECT_NO_THROW(WriteBinaryFile(outputPath, buffer, size));

    EXPECT_TRUE(MkdirRecusively("test/ut/resources/op_test/add_custom/aaa/bbb", DIR_DEFAULT_MODE));

    std::string caseContent = "core_name,running_time(us),duration_time(us)";
    EXPECT_TRUE(WriteFileByStream(casefile, caseContent));

    EXPECT_TRUE(WriteFileByBinary(casefile, caseContent.c_str(), caseContent.size()));

    std::string binPath = "test/ut/resources/op_test/add_custom/add_custom.o";
    size_t binSize = GetFileSize(binPath);
    std::vector<char> bin(binSize);
    EXPECT_TRUE(ReadBinaryFile(binPath, bin));
    free(buffer);
}

/**
 * |  用例集  | FileSystem
 * | 测试函数 | ReadFile
 * |  用例名  | test_ReadFile_with_empty_buffer_expect_false
 * | 用例描述 | 在buff指针为空的情况下返回false
 */
TEST(FileSystem, test_ReadFile_with_empty_buffer_expect_false)
{
    uint8_t *emptyBuffer = nullptr;
    ASSERT_FALSE(ReadFile("", emptyBuffer, 10));
}

/**
 * |  用例集  | FileSystem
 * | 测试函数 | ReadFile
 * |  用例名  | test_ReadFile_with_inconsistent_buffersize_expect_false
 * | 用例描述 | 测试读取的filepath的文件大小与输入的bufferSize不同的情况下返回false
 */
TEST(FileSystem, test_ReadFile_with_inconsistent_buffersize_expect_false)
{
    GlobalMockObject::verify();
     MOCKER(&Utility::GetFileSize)
             .stubs()
             .will(returnValue(100));
    uint8_t tmpNum = 1;
    uint8_t *buffer = &tmpNum;
    ASSERT_FALSE(ReadFile("", buffer, 10));

    GlobalMockObject::verify();
}

/**
 * |  用例集  | FileSystem
 * | 测试函数 | ReadFile
 * |  用例名  | test_ReadFile_with_inconsistent_buffersize_expect_false
 * | 用例描述 | 测试读取的filepath的文件大小与输入的bufferSize不同的情况下返回false
 */
TEST(FileSystem, test_ReadFile_with_nonexist_file_expect_false)
{
    GlobalMockObject::verify();
     MOCKER(&Utility::GetFileSize)
             .stubs()
             .will(returnValue(100));
    uint8_t tmpNum = 1;
    uint8_t *buffer = &tmpNum;
    ASSERT_FALSE(ReadFile("test/ut/nonexist.log", buffer, 100));

    GlobalMockObject::verify();
}


TEST(FileSystem, get_file_num)
{
    std::string invalidPath = "aaasss";
    EXPECT_TRUE(FilesNumGreaterThanX("./", 0));
}

TEST(FileSystem, roll_back_path_true)
{
    std::string dir = "/home//aaa/bbb";
    ASSERT_TRUE(RollbackPath(dir, 3));
    dir = "/home//aaa/bbb";
    ASSERT_TRUE(RollbackPath(dir, 2));
    dir = "/home//aaa/bbb";
    ASSERT_TRUE(RollbackPath(dir, 1));
    dir = "/////home";
    ASSERT_TRUE(RollbackPath(dir, 1));
}

TEST(FileSystem, roll_back_path_false)
{
    std::string dir = "/home/aaa/bbb";
    ASSERT_FALSE(RollbackPath(dir, 4));
}

TEST(FileSystem, CheckPermission)
{
    using namespace std::experimental::filesystem;
    bool flag {false};
    std::string dir1 = "test/ut/resources/op_profiling/simulator/tmp_dump";
    std::string dir2 = "test/ut/resources/op_profiling/simulator/dump";
    CopyFolder(dir1, dir2, dir2, nullptr);
    MkdirRecusively(dir1);
    CopyFolder(dir1, dir2, dir2, nullptr);
    MkdirRecusively(dir2);
    WriteFileByStream(dir1 + "/a.txt", "test");

    CopyFolder(dir1, dir2, dir2,nullptr);
    auto NewCallable =
        [&flag](const std::string &, const std::string &){
            flag = true;
    };
    CopyFolder(dir1, dir2, dir2, NewCallable);
    remove_all(dir1);
    remove_all(dir2);
    ASSERT_TRUE(flag);
}

TEST(FileSystem, check_permission_warning_mode) {
    ASSERT_TRUE(CheckPermission("test/ut/resources/op_profiling/111"));
    std::string path = "test/ut/resources/op_profiling/device310P/dump/op_basic_info.txt";
    chmod(path.c_str(), 0640);
    ASSERT_TRUE(CheckPermission(path));
    if (!IsRootUser()) {
        chmod(path.c_str(), 0660);
        ASSERT_TRUE(CheckPermission(path));
    }
}

/**
 * |  用例集  | FileSystem
 * | 测试函数 | GetFileSuffix
 * |  用例名  | test_GetFileSuffix_with_normal_scenario_expect_success
 * | 用例描述 | 执行正常情况下基于.进行分隔，获得dump尾缀保存在入参resSuffix中，并返回true
 */
TEST(FileSystem, test_GetFileSuffix_with_normal_scenario_expect_success)
{
    std::string file = "core0.dump";
    std::string resSuffix;
    GetFileSuffix(file, resSuffix);
    ASSERT_EQ(resSuffix, "dump");
}

/**
 * |  用例集  | FileSystem
 * | 测试函数 | GetFileSuffix
 * |  用例名  | test_GetFileSuffix_with_normal_scenario_expect_success
 * | 用例描述 | 执行非正常情况下.不存在的情况，导致的分隔失败，函数返回false
 */
TEST(FileSystem, test_GetFileSuffix_without_dot_seperator_expect_success)
{
    std::string file = "core0dump";
    std::string resSuffix;
    ASSERT_FALSE(GetFileSuffix(file, resSuffix));
}

/**
 * |  用例集  | FileSystem
 * | 测试函数 | CheckOwnerPermission,CheckPermission
 * |  用例名  | test_root_user_permission_file_expect_success
 * | 用例描述 | root用户不做任何权限类限制（属主、读写、others权限），只做基础校验，如文件存在性等。
 */
TEST(FileSystem, test_root_user_permission_file_expect_success)
{
    GlobalMockObject::verify();
    std::string msg;
    std::string path{"test/ut/resources/op_profiling/simulator_sample/dump/core0.veccore0.instr_log.dump"};
    __uid_t root = 0;
    MOCKER(getuid)
        .stubs()
        .will(returnValue(root));
    ASSERT_TRUE(CheckOwnerPermission(path, msg));
    ASSERT_TRUE(CheckPermission(path));
    GlobalMockObject::verify();
}

/**
 * |  用例集  | FileSystem
 * | 测试函数 | CheckOwnerPermission,CheckPermission
 * |  用例名  | test_root_user_permission_dir_expect_success
 * | 用例描述 | root用户不做任何权限类限制（属主、读写、others权限），只做基础校验，如文件存在性等。
 */
TEST(FileSystem, test_root_user_permission_dir_expect_success)
{
    GlobalMockObject::verify();
    std::string msg;
    std::string path{"test/ut/resources/op_profiling/simulator_sample/dump"};

    __uid_t root = 0;
    MOCKER(getuid)
        .stubs()
        .will(returnValue(root));
    ASSERT_TRUE(CheckOwnerPermission(path, msg));
    ASSERT_TRUE(CheckPermission(path));
    GlobalMockObject::verify();
}

/**
 * |  用例集  | FileSystem
 * | 测试函数 | FindExecutableCommand
 * |  用例名  | test_IsExecutableCommand_expect_success
 * | 用例描述 | ls应当所有环境都可以执行
 */
TEST(FileSystem, test_FindExecutableCommand_expect_success)
{
    GlobalMockObject::verify();

    ASSERT_FALSE(FindExecutableCommand("ls").empty());

    MOCKER(GetCurrentWorkingDir)
        .stubs()
        .will(returnValue(false));
    ASSERT_FALSE(FindExecutableCommand("ls").empty());
    GlobalMockObject::verify();
}

/**
 * |  用例集  | FileSystem
 * | 测试函数 | FindExecutableCommand
 * |  用例名  | test_IsExecutableCommand_expect_fail
 * | 用例描述 | 执行不存在的命令应当失败
 */
TEST(FileSystem, test_FindExecutableCommand_expect_fail)
{
    GlobalMockObject::verify();
    ASSERT_TRUE(FindExecutableCommand("something_really_not_exist123").empty());
}

/**
 * |  用例集  | FileSystem
 * | 测试函数 | GetAbsolutePath
 * |  用例名  | test_GetAbsolutePath_expect_success
 * | 用例描述 | 路径开头~返回正确绝对路径
 */
TEST(FileSystem, test_GetAbsolutePath_expect_success)
{
    GlobalMockObject::verify();
    std::string path = "~/../../././home";
    std::string absPath = GetAbsolutePath(path);
    ASSERT_STREQ(absPath.c_str(), "/home");
}
