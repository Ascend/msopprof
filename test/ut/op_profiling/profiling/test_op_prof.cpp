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
#include "mockcpp/mockcpp.hpp"

#include <cstring>
#include <fstream>
#include <sys/stat.h>
#include <dlfcn.h>

#define private public
#define protected public
#include "common/prof_args.h"
#include "profiling/op_prof_task.h"
#include "profiling/op_prof_data_parse.h"
#include "prof_injection/injection_event.h"
#include "profiling/simulator/op_sim_prof.h"
#undef private
#undef protected
#include "op_runner.h"
#include "ascend_helper.h"
#include "cmd_execute.h"
#include "profiling/device/data_parse/device_defs.h"
#include "profiling/device/data_parse/equation_utils.h"
using namespace Profiling;

namespace {
// 测试用Task派生类，便于直接调用基类接口
class TestOpProfTask : public Profiling::Task {
public:
    explicit TestOpProfTask(std::string taskName) : Profiling::Task(std::move(taskName)) {}
    bool Run() override { return true; }
};

// 测试用DataParse派生类，统计ParseKernelFile调用次数
class TestOpProfDataParse : public Profiling::DataParse {
public:
    explicit TestOpProfDataParse(std::string kernelNameFilter = "")
        : Profiling::DataParse(Common::ProfMetricsAbilityConfig(), std::move(kernelNameFilter)) {}
    bool Execute(std::string dataPath) override { return true; }
    void ParseKernelFile(const std::string &kernelDir, const std::string &kernelName,
                         const std::string &deviceId) override { parseKernelCount_++; }
    int parseKernelCount_ {0};
};

// 写文本文件辅助函数
void WriteTextFile(const std::string &path, const std::string &content)
{
    std::ofstream ofs(path.c_str(), std::ios::out | std::ios::trunc);
    ofs << content;
    ofs.close();
}
}

TEST(OpProf, Run_with_OpSimProf_expect_success)
{
    GlobalMockObject::verify();
    MOCKER(&OpSimProf::RunTask)
        .stubs()
        .will(returnValue(true));
    MOCKER(&OpSimProf::RunDataParse)
        .stubs()
        .will(returnValue(true));

    Common::ProfArgs runConfig;
    OpSimProf opSimProf(runConfig);
    ASSERT_TRUE(opSimProf.Run());
}

TEST(OpProf, Run_with_OpSimProf_expect_failed)
{
    GlobalMockObject::verify();
    MOCKER(&OpSimProf::RunTask)
        .stubs()
        .will(returnValue(false));
    MOCKER(&OpSimProf::RunDataParse)
        .stubs()
        .will(returnValue(false));

    Common::ProfArgs runConfig;
    OpSimProf opSimProf(runConfig);
    ASSERT_FALSE(opSimProf.Run());
    ASSERT_FALSE(opSimProf.Run());
}

TEST(OpProf, Run_with_OpSimProf_DataParse_expect_failed)
{
    GlobalMockObject::verify();
    MOCKER(&OpSimProf::RunTask)
        .stubs()
        .will(returnValue(true));
    MOCKER(&OpSimProf::RunDataParse)
        .stubs()
        .will(returnValue(false));

    Common::ProfArgs runConfig;
    OpSimProf opSimProf(runConfig);;
    ASSERT_FALSE(opSimProf.Run());
    ASSERT_FALSE(opSimProf.Run());
}

TEST(OpProf, RunTask_with_OpSimProf_expect_failed)
{
    GlobalMockObject::verify();
    Common::ProfArgs runConfig;
    OpSimProf opSimProf(runConfig);;
    MOCKER(&OpRunner::RunOpBinary)
       .stubs()
       .will(returnValue(false));
    ASSERT_FALSE(opSimProf.RunTask());
}

TEST(OpProf, RunDataParse_with_OpSimProf_expect_failed)
{
    GlobalMockObject::verify();
    MOCKER(&Utility::GetAscendHomePath)
        .stubs()
        .will(returnValue(false));
    Common::ProfArgs runConfig;
    OpSimProf opSimProf(runConfig);
    ASSERT_FALSE(opSimProf.RunDataParse());
    const char *val = "test/ut/resources";
    setenv("ASCEND_HOME_PATH", val, 1);
    ASSERT_FALSE(opSimProf.RunDataParse());
    unsetenv("ASCEND_HOME_PATH");
}

/**
/* | 用例集 | OpProf
/* |测试函数| BandWidthUsage
/* | 用例名 | test_band_width_usage_when_duration_or_maxbw_is_zero_and_expect_no_throw
/* |用例描述| 执行测试函数，当duration或者maxbw为0时，不抛出异常
*/
TEST(OpProf, test_band_width_usage_when_duration_or_maxbw_is_zero_and_expect_no_throw)
{
    GlobalMockObject::verify();
    ASSERT_NO_THROW(BandWidthUsage(10.0, 0.0, TransportType::GM_TO_UB, ChipProductType::ASCEND910B_SERIES));
    float num = 0.0f;
    MOCKER(&GetMaxBandWidthByType)
        .stubs()
        .with(any(), any(), outBound(num))
        .will(returnValue(true));
    ASSERT_NO_THROW(BandWidthUsage(10.0, 1.1, TransportType::GM_TO_UB, ChipProductType::ASCEND910B_SERIES));
    GlobalMockObject::verify();
}

// 当simulator so路径无法解析为真实路径时，CheckSimulatorSoExist返回false
TEST(OpProfTask, test_check_simulator_so_exist_when_realpath_failed_then_return_false)
{
    GlobalMockObject::verify();
    TestOpProfTask task("test");
    MOCKER(&Utility::GetSoFromEnvVar)
        .stubs()
        .will(returnValue(std::string("/not_exist_dir_xyz/libruntime_camodel.so")));
    EXPECT_FALSE(task.CheckSimulatorSoExist());
    GlobalMockObject::verify();
}

// 当dlopen打开simulator so失败时，CheckSimulatorSoExist返回false
TEST(OpProfTask, test_check_simulator_so_exist_when_dlopen_failed_then_return_false)
{
    GlobalMockObject::verify();
    std::string dir = "/tmp/msopprof_ut_task_so";
    Utility::RemoveAll(dir);
    ASSERT_TRUE(Utility::MkdirRecusively(dir));
    std::string fakeSo = dir + "/fake.so";
    WriteTextFile(fakeSo, "not a so");
    TestOpProfTask task("test");
    MOCKER(&Utility::GetSoFromEnvVar)
        .stubs()
        .will(returnValue(fakeSo));
    EXPECT_FALSE(task.CheckSimulatorSoExist());
    GlobalMockObject::verify();
    Utility::RemoveAll(dir);
}

// 当dlopen打开simulator so成功时，CheckSimulatorSoExist返回true
TEST(OpProfTask, test_check_simulator_so_exist_when_dlopen_success_then_return_true)
{
    GlobalMockObject::verify();
    std::string dir = "/tmp/msopprof_ut_task_so2";
    Utility::RemoveAll(dir);
    ASSERT_TRUE(Utility::MkdirRecusively(dir));
    std::string fakeSo = dir + "/fake.so";
    WriteTextFile(fakeSo, "not a so");
    void *handle = reinterpret_cast<void *>(0x1);
    MOCKER(&Utility::GetSoFromEnvVar)
        .stubs()
        .will(returnValue(fakeSo));
    MOCKER(&dlopen)
        .stubs()
        .will(returnValue(handle));
    MOCKER(&dlclose)
        .stubs()
        .will(returnValue(0));
    TestOpProfTask task("test");
    EXPECT_TRUE(task.CheckSimulatorSoExist());
    GlobalMockObject::verify();
    Utility::RemoveAll(dir);
}

// 当配置文件内容命中替换字符串时，ReadConfigFile替换成功且createNewFile置为true
TEST(OpProfTask, test_read_config_file_when_replace_matched_then_create_new_file_true)
{
    std::string dir = "/tmp/msopprof_ut_task_readcfg";
    Utility::RemoveAll(dir);
    ASSERT_TRUE(Utility::MkdirRecusively(dir));
    WriteTextFile(dir + "/test.toml", "parsim = 0\nparsim_thd_limit = 0\n");
    TestOpProfTask task("test");
    task.camodelLibDir_ = dir;
    bool createNewFile = false;
    std::vector<std::string> newLines;
    std::map<std::string, std::string> replaceMap{{"parsim = 0", "parsim = 1"}};
    EXPECT_TRUE(task.ReadConfigFile("test.toml", replaceMap, createNewFile, newLines));
    EXPECT_TRUE(createNewFile);
    ASSERT_EQ(newLines.size(), 2U);
    EXPECT_EQ(newLines[0], "parsim = 1");
    Utility::RemoveAll(dir);
}

// 当配置目录创建失败时，WriteNewConfig返回false
TEST(OpProfTask, test_write_new_config_when_create_dir_failed_then_return_false)
{
    GlobalMockObject::verify();
    TestOpProfTask task("test");
    task.tmpPath_ = "/tmp/msopprof_ut_task_writecfg_noexist";
    MOCKER(&Utility::MkdirRecusively)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(task.WriteNewConfig("a.toml", {"line1"}));
    GlobalMockObject::verify();
}

// 当目标配置文件不存在时，WriteNewConfig新建文件成功；已存在时走更新流程
TEST(OpProfTask, test_write_new_config_when_create_and_update_then_return_true)
{
    std::string dir = "/tmp/msopprof_ut_task_writecfg";
    Utility::RemoveAll(dir);
    ASSERT_TRUE(Utility::MkdirRecusively(dir));
    TestOpProfTask task("test");
    task.tmpPath_ = dir;
    std::vector<std::string> lines = {"abc", "def"};
    EXPECT_TRUE(task.WriteNewConfig("a.toml", lines));
    std::string destPath = dir + "/config/a.toml";
    ASSERT_TRUE(Utility::IsExist(destPath));
    // 再次写入相同文件，走UpdateNewConfig分支
    EXPECT_TRUE(task.WriteNewConfig("a.toml", lines));
    Utility::RemoveAll(dir);
}

// 当待更新文件不存在时，UpdateNewConfig中stat失败返回false
TEST(OpProfTask, test_update_new_config_when_stat_failed_then_return_false)
{
    TestOpProfTask task("test");
    EXPECT_FALSE(task.UpdateNewConfig("/tmp/msopprof_ut_no_such_file_xyz.toml", {"a"}));
}

// 当kernel配置包含各种类型参数时，GenOpConfig生成json并写入文件成功
TEST(OpProfTask, test_gen_op_config_when_full_params_then_return_true)
{
    std::string dir = "/tmp/msopprof_ut_task_genopcfg";
    Utility::RemoveAll(dir);
    ASSERT_TRUE(Utility::MkdirRecusively(dir));
    TestOpProfTask task("test");
    task.tmpPath_ = dir;
    task.kernelConfig.kernelBinaryPath = "/tmp/kernel.o";
    task.kernelConfig.blockDim = 1;
    task.kernelConfig.deviceID = 0;
    task.kernelConfig.kernelName = "add";
    task.kernelConfig.magic = "RT_DEV_BINARY_MAGIC_ELF";
    task.kernelConfig.hasTilingKey = true;
    task.kernelConfig.tilingKey = 11;
    task.kernelConfig.outputDataPath = "/tmp/out";
    OpRunner::Param fftsParam = {"fftsAddr", "", "", 0, "", true};
    OpRunner::Param inputParam = {"input", "x", "", 16, "/tmp/in.bin", true};
    OpRunner::Param inputOptionalParam = {"input", "y", "", 0, "", false};
    OpRunner::Param tilingParam = {"tiling", "", "", 32, "/tmp/tiling.bin", true};
    OpRunner::Param outputParam = {"output", "out0", "", 64, "", true};
    OpRunner::Param workspaceParam = {"workspace", "", "", 128, "", true};
    task.kernelConfig.params = {fftsParam, inputParam, inputOptionalParam, tilingParam, outputParam, workspaceParam};
    nlohmann::json jsonData;
    EXPECT_TRUE(task.GenOpConfig(jsonData));
    EXPECT_EQ(jsonData["ffts"], "Y");
    EXPECT_EQ(jsonData["kernel_name"], "add");
    EXPECT_EQ(jsonData["tiling_key"], "11");
    EXPECT_EQ(jsonData["workspace_size"], "128");
    EXPECT_EQ(jsonData["output_size"], "64");
    EXPECT_EQ(jsonData["output_name"], "out0");
    EXPECT_EQ(jsonData["tiling_data_path"], "/tmp/tiling.bin;32");
    ASSERT_EQ(task.cmd.size(), 2U);
    EXPECT_EQ(task.cmd[0], "-c");
    Utility::RemoveAll(dir);
}

// 当临时目录不存在导致json写入失败时，GenOpConfig返回false
TEST(OpProfTask, test_gen_op_config_when_write_file_failed_then_return_false)
{
    TestOpProfTask task("test");
    task.tmpPath_ = "/tmp/msopprof_ut_genopcfg_noexist/sub";
    nlohmann::json jsonData;
    EXPECT_FALSE(task.GenOpConfig(jsonData));
    EXPECT_TRUE(task.cmd.empty());
}

// 当查找到part且目标列表缺少item时，CheckIfNeedAppend插入item并返回true
TEST(OpProfTask, test_check_if_need_append_when_item_missing_then_insert_and_return_true)
{
    std::string dir = "/tmp/msopprof_ut_task_append";
    Utility::RemoveAll(dir);
    ASSERT_TRUE(Utility::MkdirRecusively(dir));
    WriteTextFile(dir + "/pem.toml", "[LOG]\r\nenable_list = [\"aic_log\"]\r\n");
    TestOpProfTask task("test");
    task.tmpPath_ = dir;
    task.camodelConfigDir_ = dir;
    std::vector<std::string> newLines;
    EXPECT_TRUE(task.CheckIfNeedAppend("pem.toml", "[LOG]", "enable_list", "mte_log", newLines));
    ASSERT_EQ(newLines.size(), 2U);
    EXPECT_NE(newLines[1].find("mte_log"), std::string::npos);
    Utility::RemoveAll(dir);
}

// 当配置文件中找不到指定part时，CheckIfNeedAppend返回false
TEST(OpProfTask, test_check_if_need_append_when_part_not_found_then_return_false)
{
    std::string dir = "/tmp/msopprof_ut_task_append2";
    Utility::RemoveAll(dir);
    ASSERT_TRUE(Utility::MkdirRecusively(dir));
    WriteTextFile(dir + "/pem.toml", "[OTHER]\r\nenable_list = []\r\n");
    TestOpProfTask task("test");
    task.tmpPath_ = dir;
    task.camodelConfigDir_ = dir;
    std::vector<std::string> newLines;
    EXPECT_FALSE(task.CheckIfNeedAppend("pem.toml", "[LOG]", "enable_list", "mte_log", newLines));
    Utility::RemoveAll(dir);
}

// 当part之后遇到空行时，CheckIfNeedAppend停止扫描并返回false
TEST(OpProfTask, test_check_if_need_append_when_empty_line_then_return_false)
{
    std::string dir = "/tmp/msopprof_ut_task_append3";
    Utility::RemoveAll(dir);
    ASSERT_TRUE(Utility::MkdirRecusively(dir));
    WriteTextFile(dir + "/pem.toml", "[LOG]\r\n\nenable_list = []\r\n");
    TestOpProfTask task("test");
    task.tmpPath_ = dir;
    task.camodelConfigDir_ = dir;
    std::vector<std::string> newLines;
    EXPECT_FALSE(task.CheckIfNeedAppend("pem.toml", "[LOG]", "enable_list", "mte_log", newLines));
    Utility::RemoveAll(dir);
}

// 当目标行没有左括号时，CheckIfNeedAppend跳过该行继续扫描并返回false
TEST(OpProfTask, test_check_if_need_append_when_no_left_bracket_then_return_false)
{
    std::string dir = "/tmp/msopprof_ut_task_append4";
    Utility::RemoveAll(dir);
    ASSERT_TRUE(Utility::MkdirRecusively(dir));
    WriteTextFile(dir + "/pem.toml", "[LOG]\r\nenable_list = \"aic_log\"\r\nother = 1\r\n");
    TestOpProfTask task("test");
    task.tmpPath_ = dir;
    task.camodelConfigDir_ = dir;
    std::vector<std::string> newLines;
    EXPECT_FALSE(task.CheckIfNeedAppend("pem.toml", "[LOG]", "enable_list", "mte_log", newLines));
    Utility::RemoveAll(dir);
}

// 当目标列表已包含item时，CheckIfNeedAppend无需更新返回false
TEST(OpProfTask, test_check_if_need_append_when_item_exist_then_return_false)
{
    std::string dir = "/tmp/msopprof_ut_task_append5";
    Utility::RemoveAll(dir);
    ASSERT_TRUE(Utility::MkdirRecusively(dir));
    WriteTextFile(dir + "/pem.toml", "[LOG]\r\nenable_list = [\"mte_log\"]\r\n");
    TestOpProfTask task("test");
    task.tmpPath_ = dir;
    task.camodelConfigDir_ = dir;
    std::vector<std::string> newLines;
    EXPECT_FALSE(task.CheckIfNeedAppend("pem.toml", "[LOG]", "enable_list", "mte_log", newLines));
    Utility::RemoveAll(dir);
}

// 当需要追加item时，CreateConfigFileWithAppend写入新配置并返回true
TEST(OpProfTask, test_create_config_file_with_append_when_need_append_then_return_true)
{
    std::string dir = "/tmp/msopprof_ut_task_append6";
    Utility::RemoveAll(dir);
    ASSERT_TRUE(Utility::MkdirRecusively(dir));
    WriteTextFile(dir + "/pem.toml", "[LOG]\r\nenable_list = [\"aic_log\"]\r\n");
    TestOpProfTask task("test");
    task.tmpPath_ = dir;
    task.camodelConfigDir_ = dir;
    EXPECT_TRUE(task.CreateConfigFileWithAppend("pem.toml", "[LOG]", "enable_list", "mte_log"));
    Utility::RemoveAll(dir);
}

// 当simSocVersion为空时，CreateCamodelConfig直接返回不生成配置
TEST(OpProfTask, test_create_camodel_config_when_soc_version_empty_then_return)
{
    TestOpProfTask task("test");
    task.simSocVersion = "";
    task.CreateCamodelConfig(false);
    EXPECT_TRUE(task.camodelLibDir_.empty());
    EXPECT_TRUE(task.env.empty());
}

// 当910B系列开启pm采样时，CreateCamodelConfig尝试追加mte_log配置
TEST(OpProfTask, test_create_camodel_config_910b_with_pm_sampling)
{
    GlobalMockObject::verify();
    std::string home = "/tmp/msopprof_ut_ascend_home_910b";
    Utility::RemoveAll(home);
    ASSERT_TRUE(Utility::MkdirRecusively(home));
    TestOpProfTask task("test");
    task.simSocVersion = "Ascend910B4";
    MOCKER(&Utility::GetAscendHomePath)
        .stubs()
        .with(outBound(home))
        .will(returnValue(true));
    task.CreateCamodelConfig(true);
    EXPECT_TRUE(task.env.find("CAMODEL_CONFIG_PATH") == task.env.end());
    GlobalMockObject::verify();
    Utility::RemoveAll(home);
}

// 当310P系列spec文件命中替换时，CreateCamodelConfig生成配置并设置环境变量
TEST(OpProfTask, test_create_camodel_config_310p_when_spec_matched_then_set_env)
{
    GlobalMockObject::verify();
    std::string home = "/tmp/msopprof_ut_ascend_home_310p";
    std::string tmpPath = "/tmp/msopprof_ut_task_tmp_310p";
    Utility::RemoveAll(home);
    Utility::RemoveAll(tmpPath);
    std::string libDir = home + "/tools/simulator/Ascend310P3/lib";
    ASSERT_TRUE(Utility::MkdirRecusively(libDir));
    ASSERT_TRUE(Utility::MkdirRecusively(tmpPath));
    WriteTextFile(libDir + "/davinci_vec_core.spec", "flush_level = \"3\"\n");
    WriteTextFile(libDir + "/common.spec", "common\n");
    TestOpProfTask task("test");
    task.simSocVersion = "Ascend310P3";
    task.tmpPath_ = tmpPath;
    MOCKER(&Utility::GetAscendHomePath)
        .stubs()
        .with(outBound(home))
        .will(returnValue(true));
    task.CreateCamodelConfig(false);
    EXPECT_EQ(task.env["CAMODEL_CONFIG_PATH"], tmpPath + "/config");
    GlobalMockObject::verify();
    Utility::RemoveAll(home);
    Utility::RemoveAll(tmpPath);
}

// 当310P系列没有任何配置文件时，CreateCamodelConfig不设置环境变量
TEST(OpProfTask, test_create_camodel_config_310p_when_no_config_file_then_not_set_env)
{
    GlobalMockObject::verify();
    std::string home = "/tmp/msopprof_ut_ascend_home_310p2";
    Utility::RemoveAll(home);
    ASSERT_TRUE(Utility::MkdirRecusively(home));
    TestOpProfTask task("test");
    task.simSocVersion = "Ascend310P3";
    MOCKER(&Utility::GetAscendHomePath)
        .stubs()
        .with(outBound(home))
        .will(returnValue(true));
    task.CreateCamodelConfig(false);
    EXPECT_TRUE(task.env.find("CAMODEL_CONFIG_PATH") == task.env.end());
    GlobalMockObject::verify();
    Utility::RemoveAll(home);
}

// 注册运行事件后，各消息处理函数能正确处理空包、普通日志及采集开始结束事件
TEST(OpProfTask, test_register_running_event_handlers_process_packets)
{
    GlobalMockObject::verify();
    TestOpProfTask task("test");
    task.realTimeSimParseContext_.chipType = ChipProductType::ASCEND910B4;
    task.realTimeSimParseContext_.metricsConfig.pmSamplingEnable = true;
    task.realTimeDataParser_ =
        std::make_shared<Profiling::Parse::RealTimeDataParser>(task.realTimeSimParseContext_);

    MOCKER(static_cast<void (Profiling::Parse::RealTimeDataParser::*)(const Common::DvcInstrLog &)>(
               &Profiling::Parse::RealTimeDataParser::SetInstrLog)).stubs();
    MOCKER(static_cast<void (Profiling::Parse::RealTimeDataParser::*)(const Common::DvcInstrLogV2 &)>(
               &Profiling::Parse::RealTimeDataParser::SetInstrLog)).stubs();
    MOCKER(static_cast<void (Profiling::Parse::RealTimeDataParser::*)(const Common::DvcInstrLog &)>(
               &Profiling::Parse::RealTimeDataParser::SetPopInstrLog)).stubs();
    MOCKER(static_cast<void (Profiling::Parse::RealTimeDataParser::*)(const Common::DvcInstrLogV2 &)>(
               &Profiling::Parse::RealTimeDataParser::SetPopInstrLog)).stubs();
    MOCKER(&Profiling::Parse::RealTimeDataParser::SetICacheLog).stubs();
    MOCKER(&Profiling::Parse::RealTimeDataParser::SetMteLog).stubs();
    MOCKER(&Profiling::Parse::RealTimeDataParser::SetCcuLog).stubs();
    MOCKER(&Profiling::Parse::RealTimeDataParser::Start).stubs();
    MOCKER(&Profiling::Parse::RealTimeDataParser::Stop).stubs();

    auto &injectionEvent = ProfStub::InjectionEvent::Instance();
    // 防止其他用例残留同名handler，先清理再注册
    for (auto type : {ProfPacketType::INSTR_LOG, ProfPacketType::POPPED_LOG, ProfPacketType::MTE_LOG,
                      ProfPacketType::ICACHE_LOG, ProfPacketType::CCU_LOG, ProfPacketType::COLLECT_START,
                      ProfPacketType::PROF_FINISH}) {
        injectionEvent.UnregisterPacketHandler(type);
    }
    Profiling::Task::needRegisterEvent_ = true;
    task.RegisterRunningEvent();
    EXPECT_FALSE(Profiling::Task::needRegisterEvent_.load());
    ASSERT_EQ(injectionEvent.msgProcessFunc_.count(ProfPacketType::INSTR_LOG), 1U);
    ASSERT_EQ(injectionEvent.msgProcessFunc_.count(ProfPacketType::CCU_LOG), 1U);

    auto pkt = std::make_shared<ProfStub::Packet>(0);
    std::memset(&pkt->payload_, 0, sizeof(pkt->payload_));

    // INSTR_LOG：空包、V1包、V2包
    EXPECT_EQ(injectionEvent.msgProcessFunc_[ProfPacketType::INSTR_LOG](nullptr, 0), "");
    EXPECT_EQ(injectionEvent.msgProcessFunc_[ProfPacketType::INSTR_LOG](pkt, 0), "");
    pkt->isInstrLogV2_ = true;
    EXPECT_EQ(injectionEvent.msgProcessFunc_[ProfPacketType::INSTR_LOG](pkt, 0), "");
    pkt->isInstrLogV2_ = false;

    // POPPED_LOG：空包、V1包、V2包
    EXPECT_EQ(injectionEvent.msgProcessFunc_[ProfPacketType::POPPED_LOG](nullptr, 0), "");
    EXPECT_EQ(injectionEvent.msgProcessFunc_[ProfPacketType::POPPED_LOG](pkt, 0), "");
    pkt->isInstrLogV2_ = true;
    EXPECT_EQ(injectionEvent.msgProcessFunc_[ProfPacketType::POPPED_LOG](pkt, 0), "");
    pkt->isInstrLogV2_ = false;

    // MTE_LOG：空包与非空包
    EXPECT_EQ(injectionEvent.msgProcessFunc_[ProfPacketType::MTE_LOG](nullptr, 0), "");
    EXPECT_EQ(injectionEvent.msgProcessFunc_[ProfPacketType::MTE_LOG](pkt, 0), "");

    // ICACHE_LOG/CCU_LOG：空包与非空包
    EXPECT_EQ(injectionEvent.msgProcessFunc_[ProfPacketType::ICACHE_LOG](nullptr, 0), "");
    EXPECT_EQ(injectionEvent.msgProcessFunc_[ProfPacketType::ICACHE_LOG](pkt, 0), "");
    EXPECT_EQ(injectionEvent.msgProcessFunc_[ProfPacketType::CCU_LOG](nullptr, 0), "");
    EXPECT_EQ(injectionEvent.msgProcessFunc_[ProfPacketType::CCU_LOG](pkt, 0), "");

    // COLLECT_START：空包、空路径包、合法包
    EXPECT_EQ(injectionEvent.msgProcessFunc_[ProfPacketType::COLLECT_START](nullptr, 0), "");
    EXPECT_EQ(injectionEvent.msgProcessFunc_[ProfPacketType::COLLECT_START](pkt, 0), "SUC");
    EXPECT_FALSE(task.isCaLogTransStartSuc_);
    std::strncpy(pkt->payload_.collectLogStart.outputPath, "/tmp/msopprof_ut_rt",
                 sizeof(pkt->payload_.collectLogStart.outputPath) - 1);
    std::strncpy(pkt->payload_.collectLogStart.kernelName, "add",
                 sizeof(pkt->payload_.collectLogStart.kernelName) - 1);
    EXPECT_EQ(injectionEvent.msgProcessFunc_[ProfPacketType::COLLECT_START](pkt, 0), "SUC");
    EXPECT_TRUE(task.isCaLogTransStartSuc_);

    // PROF_FINISH
    EXPECT_EQ(injectionEvent.msgProcessFunc_[ProfPacketType::PROF_FINISH](pkt, 0), "SUC");

    for (auto type : {ProfPacketType::INSTR_LOG, ProfPacketType::POPPED_LOG, ProfPacketType::MTE_LOG,
                      ProfPacketType::ICACHE_LOG, ProfPacketType::CCU_LOG, ProfPacketType::COLLECT_START,
                      ProfPacketType::PROF_FINISH}) {
        injectionEvent.UnregisterPacketHandler(type);
    }
    GlobalMockObject::verify();
}

// 当目标文件是kernel_config.bin时，ModifyKernelData将其中的旧路径替换为新路径
TEST(OpProfDataParse, test_modify_kernel_data_when_kernel_config_then_replace_path)
{
    std::string dir = "/tmp/msopprof_ut_modify_kernel";
    Utility::RemoveAll(dir);
    ASSERT_TRUE(Utility::MkdirRecusively(dir));
    std::string file = dir + "/kernel_config.bin";
    WriteTextFile(file, "{\"bin_path\":\"" + dir + "/kernel_data/add.o\"}");
    Profiling::DataParse::ModifyKernelData(file, dir);
    std::ifstream in(file.c_str());
    std::string content;
    std::getline(in, content);
    in.close();
    EXPECT_EQ(content, "{\"bin_path\":\"" + dir + "/add.o\"}");
    Utility::RemoveAll(dir);
}

// 当kernel_config.bin内容不包含输出根路径时，ModifyKernelData告警并直接返回
TEST(OpProfDataParse, test_modify_kernel_data_when_root_path_not_found_then_return)
{
    std::string dir = "/tmp/msopprof_ut_modify_kernel2";
    Utility::RemoveAll(dir);
    ASSERT_TRUE(Utility::MkdirRecusively(dir));
    std::string file = dir + "/kernel_config.bin";
    WriteTextFile(file, "{\"bin_path\":\"/other/kernel_data/add.o\"}");
    Profiling::DataParse::ModifyKernelData(file, dir);
    std::ifstream in(file.c_str());
    std::string content;
    std::getline(in, content);
    in.close();
    EXPECT_EQ(content, "{\"bin_path\":\"/other/kernel_data/add.o\"}");
    Utility::RemoveAll(dir);
}

// 当文件路径为空时，RenameCsvFileAndModifyKernelData直接返回
TEST(OpProfDataParse, test_rename_csv_when_path_empty_then_return)
{
    EXPECT_NO_THROW(Profiling::DataParse::RenameCsvFileAndModifyKernelData("", "/tmp/root"));
}

// 当没有任何kernel数据时，ExecuteSummary告警并返回false
TEST(OpProfDataParse, test_execute_summary_when_no_kernel_then_return_false)
{
    TestOpProfDataParse dataParse;
    EXPECT_FALSE(dataParse.ExecuteSummary("/tmp"));
}

// 当数据路径无法枚举且配置了kernel过滤时，ParserDeviceIdDir告警并返回false
TEST(OpProfDataParse, test_parser_device_id_dir_when_get_filenames_fail_then_return_false)
{
    TestOpProfDataParse dataParse("kernel*");
    EXPECT_FALSE(dataParse.ParserDeviceIdDir("/tmp/msopprof_ut_not_exist_dir_xyz"));
    EXPECT_EQ(dataParse.parseKernelCount_, 0);
}

// 当device目录下包含非目录条目且已记录唯一device路径时，跳过该条目并重置标记
TEST(OpProfDataParse, test_parser_device_id_dir_with_non_dir_entries_then_skip)
{
    std::string dir = "/tmp/msopprof_ut_parse_device";
    Utility::RemoveAll(dir);
    ASSERT_TRUE(Utility::MkdirRecusively(dir + "/dev1/kernelA"));
    WriteTextFile(dir + "/dev0_file", "x");
    WriteTextFile(dir + "/dev1/k_file", "x");
    TestOpProfDataParse dataParse;
    dataParse.theOnlyDevicePath_ = "/tmp/xxx";
    EXPECT_TRUE(dataParse.ParserDeviceIdDir(dir));
    EXPECT_EQ(dataParse.parseKernelCount_, 1);
    EXPECT_FALSE(dataParse.isTheOnlyDevice_);
    Utility::RemoveAll(dir);
}

// 当symbolizer执行成功时，Parse记录解析关系并返回true
TEST(OpProfDataParse, test_symbolizer_parse_when_symbolizer_success_then_return_true)
{
    GlobalMockObject::verify();
    Profiling::SymbolizerParser parser;
    std::string jsonOut = "[{\"Address\":\"0x0\",\"ModuleName\":\"./aicore_binary.o\",\"Symbol\":[{\"Column\":0,\"Discriminator\":0,\"FileName\":\"/IncreFlashAttention.cpp\",\"FunctionName\":\"IncreFlashAttention_mix_aic\",\"Line\":23,\"StartAddress\":\"0x0\",\"StartFileName\":\"/IncreFlashAttention.cpp\",\"StartLine\":22}]}]";
    MOCKER(&Utility::GetMsopprofPath)
        .stubs()
        .will(returnValue(std::string("/tmp")));
    MOCKER(&Utility::CmdExecute)
        .stubs()
        .with(any(), any(), outBound(jsonOut))
        .will(returnValue(true));
    EXPECT_TRUE(parser.Parse("/tmp/reloc.o", {"0x0"}));
    EXPECT_FALSE(parser.offset2Lines_.empty());
    GlobalMockObject::verify();
}

// 当symbolizer命令执行失败时，SymbolizerPartAddr返回false
TEST(OpProfDataParse, test_symbolizer_part_addr_when_cmd_execute_fail_then_return_false)
{
    GlobalMockObject::verify();
    Profiling::SymbolizerParser parser;
    MOCKER(&Utility::CmdExecute)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(parser.SymbolizerPartAddr({"llvm-symbolizer", "-e", "a.o", "0x0"}));
    GlobalMockObject::verify();
}

// 当symbolizer输出不是合法json时，SymbolizerPartAddr解析异常返回false
TEST(OpProfDataParse, test_symbolizer_part_addr_when_json_invalid_then_return_false)
{
    GlobalMockObject::verify();
    Profiling::SymbolizerParser parser;
    std::string out = "not a json";
    MOCKER(&Utility::CmdExecute)
        .stubs()
        .with(any(), any(), outBound(out))
        .will(returnValue(true));
    EXPECT_FALSE(parser.SymbolizerPartAddr({"llvm-symbolizer"}));
    GlobalMockObject::verify();
}

// 当symbolizer输出是合法json行时，SymbolizerPartAddr解析成功返回true
TEST(OpProfDataParse, test_symbolizer_part_addr_when_valid_json_then_return_true)
{
    GlobalMockObject::verify();
    Profiling::SymbolizerParser parser;
    std::string jsonOut = "noise line\n[{\"Address\":\"0x0\",\"ModuleName\":\"./aicore_binary.o\",\"Symbol\":[{\"Column\":0,\"Discriminator\":0,\"FileName\":\"/IncreFlashAttention.cpp\",\"FunctionName\":\"f\",\"Line\":23,\"StartAddress\":\"0x0\",\"StartFileName\":\"/IncreFlashAttention.cpp\",\"StartLine\":22}]}]\ntail";
    MOCKER(&Utility::CmdExecute)
        .stubs()
        .with(any(), any(), outBound(jsonOut))
        .will(returnValue(true));
    EXPECT_TRUE(parser.SymbolizerPartAddr({"llvm-symbolizer"}));
    EXPECT_EQ(parser.offset2Lines_.size(), 1U);
    GlobalMockObject::verify();
}

// 当addr2Line缺少Symbol字段时，GetSymbolsFromAddr返回false
TEST(OpProfDataParse, test_get_symbols_from_addr_when_no_symbol_then_return_false)
{
    Profiling::SymbolizerParser parser;
    nlohmann::json addr2Line = {{"Address", "0x0"}};
    std::vector<Profiling::Symbol> symbols;
    EXPECT_FALSE(parser.GetSymbolsFromAddr(addr2Line, symbols, 0));
}

// 当Symbol数组为空时，GetSymbolsFromAddr返回false
TEST(OpProfDataParse, test_get_symbols_from_addr_when_symbol_empty_then_return_false)
{
    Profiling::SymbolizerParser parser;
    nlohmann::json addr2Line = {{"Symbol", nlohmann::json::array()}};
    std::vector<Profiling::Symbol> symbols;
    EXPECT_FALSE(parser.GetSymbolsFromAddr(addr2Line, symbols, 1));
    EXPECT_TRUE(symbols.empty());
}

// 当kernel文件来自ascend-toolkit时，ParseAddr2Offset使用末尾symbol保存映射成功
TEST(OpProfDataParse, test_parse_addr2offset_when_kernel_from_ascend_toolkit_then_return_true)
{
    Profiling::SymbolizerParser parser;
    std::string jsonContext = "[{\"Address\":\"0x0\",\"Symbol\":[{\"Column\":0,\"Discriminator\":0,\"FileName\":\"/home/ascend-toolkit/kernel.cpp\",\"FunctionName\":\"f\",\"Line\":23,\"StartAddress\":\"0x0\",\"StartFileName\":\"/home/ascend-toolkit/kernel.cpp\",\"StartLine\":22}]}]";
    auto addr2Lines = nlohmann::json::parse(jsonContext);
    EXPECT_TRUE(parser.ParseAddr2Offset(addr2Lines));
    EXPECT_EQ(parser.offset2Lines_.size(), 1U);
}

// 当ascend-toolkit的symbol字段不完整时，ParseAddr2Offset返回false
TEST(OpProfDataParse, test_parse_addr2offset_when_toolkit_symbol_incomplete_then_return_false)
{
    Profiling::SymbolizerParser parser;
    std::string jsonContext = "[{\"Address\":\"0x0\",\"Symbol\":[{\"FileName\":\"/home/ascend-toolkit/kernel.cpp\"}]}]";
    auto addr2Lines = nlohmann::json::parse(jsonContext);
    EXPECT_FALSE(parser.ParseAddr2Offset(addr2Lines));
}

// 当用户kernel的symbol字段不完整时，ParseAddr2Offset返回false
TEST(OpProfDataParse, test_parse_addr2offset_when_user_symbol_incomplete_then_return_false)
{
    Profiling::SymbolizerParser parser;
    std::string jsonContext = "[{\"Address\":\"0x0\",\"Symbol\":[{\"FileName\":\"/home/user/kernel.cpp\",\"Line\":23}]}]";
    auto addr2Lines = nlohmann::json::parse(jsonContext);
    EXPECT_FALSE(parser.ParseAddr2Offset(addr2Lines));
}

// 当pcStart文件存在且内容非默认值时，GetStartPcFromDump返回文件中的起始pc
TEST(OpProfDataParse, test_get_start_pc_from_dump_when_valid_file_then_return_line)
{
    std::string dir = "/tmp/msopprof_ut_startpc";
    Utility::RemoveAll(dir);
    ASSERT_TRUE(Utility::MkdirRecusively(dir));
    WriteTextFile(dir + "/pc_start.txt", "0x1234\n");
    EXPECT_EQ(Profiling::GetStartPcFromDump(dir, "pc_start.txt"), "0x1234");
    WriteTextFile(dir + "/pc_start.txt", "0x0\n");
    EXPECT_EQ(Profiling::GetStartPcFromDump(dir, "pc_start.txt"), "");
    Utility::RemoveAll(dir);
}