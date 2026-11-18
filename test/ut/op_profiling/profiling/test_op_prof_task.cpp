/* -------------------------------------------------------------------------
 *  This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include <algorithm>
#include <cstdlib>
#include <experimental/filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"

#include "filesystem.h"
#include "op_runner.h"
#define private public
#define protected public
#include "profiling/op_prof_task.h"
#undef private
#undef protected

using namespace Profiling;
using namespace Utility;

namespace {
std::string MakeTaskTempDir() {
    char path[] = "/tmp/msopprof_task_ut_XXXXXX";
    char *result = mkdtemp(path);
    return result == nullptr ? std::string() : std::string(result);
}

void WriteTextFile(const std::string &path, const std::string &content) {
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    out << content;
}

std::string ReadTextFile(const std::string &path) {
    std::ifstream in(path);
    std::stringstream content;
    content << in.rdbuf();
    return content.str();
}

bool ContainsLine(const std::vector<std::string> &lines, const std::string &key) {
    return std::any_of(
        lines.begin(), lines.end(), [&key](const std::string &line) { return line.find(key) != std::string::npos; });
}

class TestTask : public Profiling::Task {
public:
    explicit TestTask(std::string name) : Profiling::Task(std::move(name)) {}
    bool Run() override { return true; }
};
} // namespace

/**
 * |  用例集  | OpProfTaskUt
 * | 测试函数 | GenOpConfig
 * |  用例名  | test_GenOpConfig_with_all_param_types_expect_success
 * |  用例描述 | 测试所有参数类型都能正确写入kernel_config.bin并更新命令行
 */
TEST(OpProfTaskUt, test_GenOpConfig_with_all_param_types_expect_success) {
    std::string tempDir = MakeTaskTempDir();
    ASSERT_FALSE(tempDir.empty());
    TestTask task("op_prof_task");
    task.tmpPath_ = tempDir;
    task.kernelConfig.kernelBinaryPath = "/tmp/kernel.o";
    task.kernelConfig.blockDim = 8;
    task.kernelConfig.deviceID = 3;
    task.kernelConfig.kernelName = "add";
    task.kernelConfig.magic = "0x1234";
    task.kernelConfig.outputDataPath = tempDir + "/output";
    task.kernelConfig.hasTilingKey = true;
    task.kernelConfig.tilingKey = 7;
    task.kernelConfig.params = {
        {"fftsAddr", "", "", 0, "", true},
        {"input", "x", "float16", 128, "x.bin", true},
        {"input", "x2", "float16", 64, "x2.bin", false},
        {"tiling", "", "", 32, "tiling.bin", true},
        {"output", "y", "float16", 256, "y.bin", true},
        {"workspace", "", "", 512, "", true},
        {"unknown", "", "", 0, "", true},
    };

    nlohmann::json jsonData;
    EXPECT_TRUE(task.GenOpConfig(jsonData));
    EXPECT_EQ(jsonData["bin_path"], "/tmp/kernel.o");
    EXPECT_EQ(jsonData["block_dim"], "8");
    EXPECT_EQ(jsonData["device_id"], "3");
    EXPECT_EQ(jsonData["ffts"], "Y");
    EXPECT_EQ(jsonData["input_path"], "x.bin;n");
    EXPECT_EQ(jsonData["input_size"], "128;0");
    EXPECT_EQ(jsonData["tiling_data_path"], "tiling.bin;32");
    EXPECT_EQ(jsonData["tiling_key"], "7");
    EXPECT_EQ(jsonData["output_size"], "256");
    EXPECT_EQ(jsonData["output_name"], "y");
    EXPECT_EQ(jsonData["output_dir"], tempDir + "/output");
    EXPECT_EQ(jsonData["workspace_size"], "512");
    EXPECT_EQ(jsonData["kernel_name"], "add");
    EXPECT_EQ(jsonData["magic"], "0x1234");
    ASSERT_EQ(task.cmd.size(), 2u);
    EXPECT_EQ(task.cmd[0], "-c");
    EXPECT_EQ(task.cmd[1], tempDir + "/kernel_config.bin");
    std::ifstream configFile(task.cmd[1]);
    ASSERT_TRUE(configFile.is_open());
    const auto saved = nlohmann::json::parse(configFile, nullptr, false);
    ASSERT_FALSE(saved.is_discarded());
    EXPECT_EQ(saved, jsonData);
    std::experimental::filesystem::remove_all(tempDir);
}

/**
 * |  用例集  | OpProfTaskUt
 * | 测试函数 | GenOpConfig
 * |  用例名  | test_GenOpConfig_without_optional_fields_expect_success
 * |  用例描述 | 测试无可选参数时不生成tiling_key和output字段
 */
TEST(OpProfTaskUt, test_GenOpConfig_without_optional_fields_expect_success) {
    std::string tempDir = MakeTaskTempDir();
    ASSERT_FALSE(tempDir.empty());
    TestTask task("op_prof_task");
    task.tmpPath_ = tempDir;
    task.kernelConfig.kernelBinaryPath = "kernel.o";
    task.kernelConfig.blockDim = 1;
    task.kernelConfig.deviceID = 0;
    task.kernelConfig.kernelName = "minimal";
    task.kernelConfig.magic = "0x0";

    nlohmann::json jsonData;
    EXPECT_TRUE(task.GenOpConfig(jsonData));
    EXPECT_EQ(jsonData["ffts"], "N");
    EXPECT_EQ(jsonData["input_path"], "");
    EXPECT_EQ(jsonData["input_size"], "");
    EXPECT_FALSE(jsonData.contains("tiling_key"));
    EXPECT_FALSE(jsonData.contains("output_size"));
    EXPECT_FALSE(jsonData.contains("output_name"));
    EXPECT_FALSE(jsonData.contains("workspace_size"));
    EXPECT_EQ(task.cmd.size(), 2u);
    std::experimental::filesystem::remove_all(tempDir);
}

/**
 * |  用例集  | OpProfTaskUt
 * | 测试函数 | ReadConfigFile
 * |  用例名  | test_ReadConfigFile_with_valid_file_expect_replace_lines
 * |  用例描述 | 测试读取配置文件并替换指定字符串
 */
TEST(OpProfTaskUt, test_ReadConfigFile_with_valid_file_expect_replace_lines) {
    std::string tempDir = MakeTaskTempDir();
    ASSERT_FALSE(tempDir.empty());
    std::string libDir = tempDir + "/lib";
    std::experimental::filesystem::create_directories(libDir);
    WriteTextFile(libDir + "/sim.toml", "parsim = 0\nplain line\n");

    TestTask task("op_prof_task");
    task.camodelLibDir_ = libDir;
    bool createNewFile = false;
    std::vector<std::string> newLines;
    EXPECT_TRUE(task.ReadConfigFile("sim.toml", {{"parsim = 0", "parsim = 1"}}, createNewFile, newLines));
    EXPECT_TRUE(createNewFile);
    ASSERT_EQ(newLines.size(), 2u);
    EXPECT_EQ(newLines[0], "parsim = 1");
    EXPECT_EQ(newLines[1], "plain line");

    createNewFile = false;
    newLines.clear();
    EXPECT_TRUE(task.ReadConfigFile("sim.toml", {{"not_exist", "value"}}, createNewFile, newLines));
    EXPECT_FALSE(createNewFile);
    ASSERT_EQ(newLines.size(), 2u);
    EXPECT_EQ(newLines[0], "parsim = 0");
    EXPECT_FALSE(task.ReadConfigFile("missing.toml", {}, createNewFile, newLines));
    std::experimental::filesystem::remove_all(tempDir);
}

/**
 * |  用例集  | OpProfTaskUt
 * | 测试函数 | WriteNewConfig、UpdateNewConfig
 * |  用例名  | test_WriteNewConfig_and_UpdateNewConfig_expect_success
 * |  用例描述 | 测试创建配置目录、写入新配置以及更新已存在配置
 */
TEST(OpProfTaskUt, test_WriteNewConfig_and_UpdateNewConfig_expect_success) {
    std::string tempDir = MakeTaskTempDir();
    ASSERT_FALSE(tempDir.empty());
    TestTask task("op_prof_task");
    task.tmpPath_ = tempDir;
    task.camodelConfigDir_.clear();

    EXPECT_TRUE(task.WriteNewConfig("config.toml", {"first", "second"}));
    std::string destPath = tempDir + "/config/config.toml";
    EXPECT_TRUE(std::experimental::filesystem::exists(destPath));
    EXPECT_EQ(ReadTextFile(destPath), "first\nsecond\n");

    EXPECT_TRUE(task.WriteNewConfig("config.toml", {"updated"}));
    EXPECT_EQ(ReadTextFile(destPath), "updated\n");
    EXPECT_FALSE(task.UpdateNewConfig(tempDir + "/missing.toml", {"value"}));
    std::experimental::filesystem::remove_all(tempDir);
}

/**
 * |  用例集  | OpProfTaskUt
 * | 测试函数 | CreateConfigFile
 * |  用例名  | test_CreateConfigFile_with_replace_and_no_change_expect_expected_result
 * |  用例描述 | 测试需要替换、无需替换和源文件不存在三种场景
 */
TEST(OpProfTaskUt, test_CreateConfigFile_with_replace_and_no_change_expect_expected_result) {
    std::string tempDir = MakeTaskTempDir();
    ASSERT_FALSE(tempDir.empty());
    std::string libDir = tempDir + "/lib";
    std::string configDir = tempDir + "/config";
    std::experimental::filesystem::create_directories(libDir);
    std::experimental::filesystem::create_directories(configDir);
    WriteTextFile(libDir + "/cache.toml", "cache_enable = 0\n");
    WriteTextFile(libDir + "/nochange.toml", "cache_enable = 0\n");

    TestTask task("op_prof_task");
    task.camodelLibDir_ = libDir;
    task.camodelConfigDir_ = configDir;
    EXPECT_TRUE(task.CreateConfigFile("cache.toml", {{"cache_enable = 0", "cache_enable = 1"}}));
    EXPECT_EQ(ReadTextFile(configDir + "/cache.toml"), "cache_enable = 1\n");
    EXPECT_FALSE(task.CreateConfigFile("nochange.toml", {{"not_exist", "value"}}));
    EXPECT_FALSE(task.CreateConfigFile("missing.toml", {}));
    std::experimental::filesystem::remove_all(tempDir);
}

/**
 * |  用例集  | OpProfTaskUt
 * | 测试函数 | CheckIfNeedAppend、CreateConfigFileWithAppend
 * |  用例名  | test_CreateConfigFileWithAppend_expect_item_appended
 * |  用例描述 | 测试在指定分区的列表中追加配置项并生成新配置文件
 */
TEST(OpProfTaskUt, test_CreateConfigFileWithAppend_expect_item_appended) {
    std::string tempDir = MakeTaskTempDir();
    ASSERT_FALSE(tempDir.empty());
    std::string configDir = tempDir + "/config";
    std::experimental::filesystem::create_directories(configDir);
    WriteTextFile(configDir + "/pem.toml", "[LOG]\r\nenable_list = [\"mte_log\"]\r\n\r\n");

    TestTask task("op_prof_task");
    task.camodelConfigDir_ = configDir;
    std::vector<std::string> newLines;
    EXPECT_TRUE(task.CheckIfNeedAppend("pem.toml", "[LOG]", "enable_list", "icache_log", newLines));
    EXPECT_TRUE(ContainsLine(newLines, "icache_log"));
    newLines.clear();
    EXPECT_FALSE(task.CheckIfNeedAppend("pem.toml", "[MISSING]", "enable_list", "icache_log", newLines));

    EXPECT_TRUE(task.CreateConfigFileWithAppend("pem.toml", "[LOG]", "enable_list", "icache_log"));
    EXPECT_TRUE(ContainsLine({ReadTextFile(configDir + "/pem.toml")}, "icache_log"));
    std::experimental::filesystem::remove_all(tempDir);
}
