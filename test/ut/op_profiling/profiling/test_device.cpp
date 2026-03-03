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

#include <string>
#include <sys/stat.h>
#include "profiling/device/op_device_prof.h"
#define private public
#define protected public
#include "profiling/device/run/device_task.h"
#include "instr_encoding/instr_encoding.h"
#include "profiling/device/data_parse/device_data_parse.h"
#include "profiling/device/data_parse/pmu_calculate.h"
#include "profiling/device/data_parse/parse_timeline.h"
#include "profiling/device/data_parse/dbi_parser.h"
#include "profiling/device/data_parse/hotspot_function_generator.h"
#include "profiling/device/data_parse/l2cache/l2cache.h"
#include "profiling/device/data_parse/metric_csv_header.h"
#include "profiling/device/data_parse/metric_data_handler.h"
#include "profiling/device/data_visualize/storage_access.h"
#include "instr_encoding/instr_encoding.h"
#include "common/hal_helper.h"
#include "common/visualize.h"
#include "packet.h"
#undef private
#undef protected
#include "op_runner.h"
#include "common/defs.h"
#include "filesystem.h"
#include "json_parser.h"
#include "cmd_execute.h"

using namespace Utility;
using namespace Profiling;
using namespace Common;
using namespace std;
using namespace Encode;

std::map<std::string, uint64_t> basicScalarPmu = {{"Scalar Time", 1832}, {"Scalar Single", 1899}, {"Scalar Dual", 1835}, {"Scalar Mte1 Stall", 1412}, {"Scalar Mte2 Stall", 1455}, {"Scalar Mte3 Stall", 1777},
    {"Scalar Wait IB", 133}, {"Scalar Wait", 1844}, {"Scalar Cube Stall", 1877}, {"Scalar Ub Stall", 1478}, {"Scalar Vector Stall", 1428},
    {"Scalar Time Vec0", 1148}, {"Scalar Single Vec0", 4318}, {"Scalar Dual Vec0", 7158}, {"Scalar Mte2 Stall Vec0", 1388}, {"Scalar Mte3 Stall Vec0", 8158}, {"Scalar Vector Stall Vec0", 118}, {"Scalar Wait IB Vec0", 318}, {"Scalar Wait Vec0", 148}, {"Scalar Ub Stall Vec0", 818},
    {"Scalar Time Vec1", 1148}, {"Scalar Single Vec1", 4318}, {"Scalar Dual Vec1", 7158}, {"Scalar Mte2 Stall Vec1", 1388}, {"Scalar Mte3 Stall Vec1", 8158}, {"Scalar Vector Stall Vec1", 118}, {"Scalar Wait IB Vec1", 318}, {"Scalar Wait Vec1", 148}, {"Scalar Ub Stall Vec1", 818},
    {"Scalar Internuclear ID0", 1275}, {"Scalar Internuclear ID1", 3542}, {"Scalar Internuclear ID2", 4345}, {"Scalar Internuclear ID3", 6514}, {"Scalar Internuclear ID4", 3144}, {"Scalar Internuclear ID5", 5315},
    {"Scalar Internuclear ID6", 6432}, {"Scalar Internuclear ID7", 3152}, {"Scalar Internuclear ID8", 5476}, {"Scalar Internuclear ID9", 2453}, {"Scalar Internuclear ID10", 653},
    {"Scalar Internuclear ID11", 324}, {"Scalar Internuclear ID12", 424}, {"Scalar Internuclear ID13", 653}, {"Scalar Internuclear ID14", 536}, {"Scalar Internuclear ID15", 133}, 

    {"Scalar Internuclear ID0 Vec0", 1275}, {"Scalar Internuclear ID1 Vec0", 3542}, {"Scalar Internuclear ID2 Vec0", 4345}, {"Scalar Internuclear ID3 Vec0", 6514},
    {"Scalar Internuclear ID4 Vec0", 3144}, {"Scalar Internuclear ID5 Vec0", 5315}, {"Scalar Internuclear ID6 Vec0", 6432}, {"Scalar Internuclear ID7 Vec0", 3152},
    {"Scalar Internuclear ID8 Vec0", 5476}, {"Scalar Internuclear ID9 Vec0", 2453}, {"Scalar Internuclear ID10 Vec0", 653}, {"Scalar Internuclear ID11 Vec0", 324},
    {"Scalar Internuclear ID12 Vec0", 424}, {"Scalar Internuclear ID13 Vec0", 653}, {"Scalar Internuclear ID14 Vec0", 536}, {"Scalar Internuclear ID15 Vec0", 133},

    {"Scalar Internuclear ID0 Vec1", 1275}, {"Scalar Internuclear ID1 Vec1", 3542}, {"Scalar Internuclear ID2 Vec1", 4345}, {"Scalar Internuclear ID3 Vec1", 6514},
    {"Scalar Internuclear ID4 Vec1", 3144}, {"Scalar Internuclear ID5 Vec1", 5315}, {"Scalar Internuclear ID6 Vec1", 6432}, {"Scalar Internuclear ID7 Vec1", 3152},
    {"Scalar Internuclear ID8 Vec1", 5476}, {"Scalar Internuclear ID9 Vec1", 2453}, {"Scalar Internuclear ID10 Vec1", 653}, {"Scalar Internuclear ID11 Vec1", 324},
    {"Scalar Internuclear ID12 Vec1", 424}, {"Scalar Internuclear ID13 Vec1", 653}, {"Scalar Internuclear ID14 Vec1", 536}, {"Scalar Internuclear ID15 Vec1", 133}, 
};

TEST(OpDeviceProf, GetDataParser_expect_success)
{
    GlobalMockObject::verify();
    MOCKER(&HalHelper::GetPlatformType)
        .stubs()
        .will(returnValue(ChipType::ASCEND910B));
    ProfArgs args;
    OpDeviceProf opDeviceProf(args);
    ASSERT_TRUE(opDeviceProf.GetDataParser().get() != nullptr);
}

TEST(DeviceDataParse, Parse_data_expect_failed)
{
    GlobalMockObject::verify();
    ProfArgs args;
    PmuEventsId pmuEventsId;
    ProfMetricsAbilityConfig metrics;
    args.argConfig = "test.json";
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metrics);
    ASSERT_FALSE(deviceDataParse.ParseExactKernelData(""));
}

TEST(DeviceDataParse, Parse_data_expect_failed_with_wrong_chip)
{
    GlobalMockObject::verify();
    ProfArgs config;
    PmuEventsId pmuEventsId;
    ProfMetricsAbilityConfig metrics;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910A, pmuEventsId, metrics);
    ASSERT_FALSE(deviceDataParse.ParseExactKernelData(""));
}

TEST(DeviceDataParse, Parse_data_basicinfo_expect_success_910B)
{
    PmuEventsId pmuEventsId;
    ProfMetricsAbilityConfig metrics;
    metrics.isBasicInfo = true;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metrics);
    ASSERT_FALSE(deviceDataParse.ParseExactKernelData(""));
}

TEST(DeviceDataParse, Parse_data_expect_success_910B)
{
    GlobalMockObject::verify();
    MOCKER(&DataHandler::LoadOpBasicInfoTxtFile)
        .stubs()
        .will(returnValue(true));
    MOCKER(&DataHandler::SaveOpBasicInfo)
            .stubs()
            .will(returnValue(true));
    ProfArgs config;
    PmuEventsId pmuEventsId;
    ProfMetricsAbilityConfig metricsConfig;
    config.argOutput = "test/ut/resources/op_profiling/device910B";
    metricsConfig.occupancyEnable = true;
    metricsConfig.roofline = true;
    pmuEventsId.LoadPmuVec(metricsConfig, ChipType::ASCEND910B);
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);

    deviceDataParse.ParserInit();
    ASSERT_TRUE(deviceDataParse.ParseExactKernelData(config.argOutput));
}

TEST(DeviceDataParse, Parse_data_expect_success_310P)
{
    GlobalMockObject::verify();
    MOCKER(&DataHandler::LoadOpBasicInfoTxtFile)
        .stubs()
        .will(returnValue(true));
    MOCKER(&DataHandler::SaveOpBasicInfo)
            .stubs()
            .will(returnValue(true));
    ProfArgs config;
    ProfMetricsAbilityConfig metricsConfig;
    PmuEventsId pmuEventsId;
    config.argOutput = "test/ut/resources/op_profiling/device310P";
    metricsConfig.roofline = true;
    pmuEventsId.LoadPmuVec(metricsConfig, ChipType::ASCEND310P);
    DeviceDataParse deviceDataParse(ChipType::ASCEND310P, pmuEventsId, metricsConfig);
    deviceDataParse.ParserInit();
    ASSERT_TRUE(deviceDataParse.ParseExactKernelData(config.argOutput));
}

TEST(DeviceDataParse, Parse_data_expect_success_A5)
{
    GlobalMockObject::verify();
    MOCKER(&DataHandler::LoadOpBasicInfoTxtFile)
            .stubs()
            .will(returnValue(true));
    MOCKER(&DataHandler::SaveOpBasicInfo)
            .stubs()
            .will(returnValue(true));
    ProfArgs config;
    PmuEventsId pmuEventsId;
    ProfMetricsAbilityConfig metricsConfig;
    config.argOutput = "test/ut/resources/op_profiling/deviceA5";
    metricsConfig.occupancyEnable = true;
    pmuEventsId.LoadPmuVec(metricsConfig, ChipType::ASCEND950);
    DeviceDataParse deviceDataParse(ChipType::ASCEND950, pmuEventsId, metricsConfig);

    deviceDataParse.ParserInit();
    ASSERT_TRUE(deviceDataParse.ParseExactKernelData(config.argOutput));
}

TEST(DeviceDataParse, Execute_expect_success_with_one_kernel)
{
    GlobalMockObject::verify();
    MOCKER(&DeviceDataParse::ParseExactKernelData)
            .stubs()
            .will(returnValue(true));
    ProfMetricsAbilityConfig metricsConfig;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    auto dumpDir = JoinPath({testDir, "device0/add/0/dump"});
    MkdirRecusively(dumpDir);
    ASSERT_TRUE(deviceDataParse.Execute(testDir));
    deviceDataParse.SingleKernelOutputReorganize(testDir);
    std::string oneKernelDir = "test/ut/resources/op_profiling/OPPO/dump";
    ASSERT_TRUE(IsExist(oneKernelDir));
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceDataParse, Execute_expect_success_with_one_device)
{
    GlobalMockObject::verify();
    MOCKER(&DeviceDataParse::ParseExactKernelData)
            .stubs()
            .will(returnValue(true));
    ProfMetricsAbilityConfig metricsConfig;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    MkdirRecusively(JoinPath({testDir, "device0/add/0/dump"}));
    MkdirRecusively(JoinPath({testDir, "device0/sub/0/dump"}));
    ASSERT_TRUE(deviceDataParse.Execute(testDir));
    deviceDataParse.SingleKernelOutputReorganize(testDir);
    ASSERT_TRUE(IsExist("test/ut/resources/op_profiling/OPPO/add"));
    ASSERT_TRUE(IsExist("test/ut/resources/op_profiling/OPPO/sub"));
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceDataParse, GetRangeFreq_expert_NA_when_get_req_failed)
{
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    std::string path = JoinPath({testDir, "tmp_dump/device0/21211/0"});
    MkdirRecusively(path);
    ProfMetricsAbilityConfig metricsConfig;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    vector<string> freqs1;
    deviceDataParse.GetRangeFreq(path, freqs1);
    ASSERT_EQ(freqs1[0], "Current Freq=NA");
    ASSERT_EQ(freqs1[1], "Rated Freq=NA");

    string freqTxt = JoinPath({testDir, "tmp_dump/device0/21211/0", "freq.txt"});
    std::ofstream outFile(freqTxt.c_str(), std::ios::out | std::ios::binary);
    outFile << "-1" << "\n";
    outFile.close();
    chmod(freqTxt.c_str(), SAVE_DATA_FILE_AUTHORITY);
    vector<string> freqs2;
    deviceDataParse.GetRangeFreq(path, freqs2);
    ASSERT_EQ(freqs2[0], "Current Freq=NA");
    ASSERT_EQ(freqs2[1], "Rated Freq=NA");
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceDataParse, GetRangeFreq_expert_value_when_get_req_success)
{
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    std::string path = JoinPath({testDir, "tmp_dump/device0/21211/0"});
    MkdirRecusively(path);
    ProfMetricsAbilityConfig metricsConfig;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    string freqTxt = JoinPath({testDir, "tmp_dump/device0/21211/0", "freq.txt"});
    std::ofstream outFile(freqTxt.c_str(), std::ios::out | std::ios::binary);
    outFile << "Current Freq=800" << "\n";
    outFile << "Rated Freq=1850" << "\n";
    outFile.close();
    chmod(freqTxt.c_str(), SAVE_DATA_FILE_AUTHORITY);
    vector<string> freqs;
    deviceDataParse.GetRangeFreq(path, freqs);
    ASSERT_EQ(freqs[0], "Current Freq=800");
    ASSERT_EQ(freqs[1], "Rated Freq=1850");
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceDataParse, ParseSingleRangeData_expert_generate_bin_success)
{
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    std::string addDir = JoinPath({testDir, "device0/add/0/dump"});
    std::string tmpDir = JoinPath({testDir, "tmp_dump/device0/21211/0"});
    MkdirRecusively(addDir);
    MkdirRecusively(tmpDir);
    string outputTxt = JoinPath({tmpDir, "output.txt"});
    std::ofstream outFile(outputTxt.c_str(), std::ios::out | std::ios::binary);
    outFile << addDir << "\n";
    outFile << "-1" << "\n";
    outFile.close();
    chmod(outputTxt.c_str(), SAVE_DATA_FILE_AUTHORITY);
    std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/duration.bin", JoinPath({tmpDir, "duration.bin"}));
    for (int i = 1; i <= 6; i++) {
        string file = "DeviceProf" + to_string(i) + ".bin";
        std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/" + file, JoinPath({tmpDir, file}));
    }
    ProfMetricsAbilityConfig metricsConfig;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    deviceDataParse.ParseSingleRangeData(tmpDir);
    ASSERT_TRUE(IsExist(JoinPath({addDir, "duration.bin"})));
    for (int i = 1; i <= 6; i++) {
        string file = "DeviceProf" + to_string(i) + ".bin";
        ASSERT_TRUE(IsExist(JoinPath({addDir, file})));
    }
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceDataParse, ParseSingleRangeData_expert_generate_basic_info_bin_success)
{
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    std::string addDir = JoinPath({testDir, "device0/add/0/dump"});
    std::string tmpDir = JoinPath({testDir, "tmp_dump/device0/21211/0"});
    MkdirRecusively(addDir);
    MkdirRecusively(tmpDir);
    string outputTxt = JoinPath({tmpDir, "output.txt"});
    std::ofstream outFile(outputTxt.c_str(), std::ios::out | std::ios::binary);
    outFile << addDir << "\n";
    outFile.close();
    chmod(outputTxt.c_str(), SAVE_DATA_FILE_AUTHORITY);
    std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/duration.bin", JoinPath({tmpDir, "duration.bin"}));
    std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/DeviceProf1.bin", JoinPath({tmpDir, "DeviceProf1.bin"}));
    ProfMetricsAbilityConfig metricsConfig;
    metricsConfig.isBasicInfo = true;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    deviceDataParse.ParseSingleRangeData(tmpDir);
    ASSERT_TRUE(IsExist(JoinPath({addDir, "duration.bin"})));
    ASSERT_TRUE(IsExist(JoinPath({addDir, "DeviceProf1.bin"})));
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceDataParse, ParseSingleRangeData_expert_generate_bin_fail)
{
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    std::string addDir = JoinPath({testDir, "device0/add/0/dump"});
    std::string tmpDir = JoinPath({testDir, "tmp_dump/device0/21211/0"});
    MkdirRecusively(addDir);
    MkdirRecusively(tmpDir);
    std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/duration.bin", JoinPath({tmpDir, "duration.bin"}));
    for (int i = 1; i <= 6; i++) {
        string file = "DeviceProf" + to_string(i) + ".bin";
        std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/" + file, JoinPath({tmpDir, file}));
    }
    ProfMetricsAbilityConfig metricsConfig;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    deviceDataParse.ParseSingleRangeData(tmpDir);
    ASSERT_FALSE(IsExist(JoinPath({addDir, "duration.bin"})));
    for (int i = 1; i <= 6; i++) {
        string file = "DeviceProf" + to_string(i) + ".bin";
        ASSERT_FALSE(IsExist(JoinPath({addDir, file})));
    }
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceDataParse, ParseTmpDump_expert_generate_bin_success)
{
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    std::string addDir = JoinPath({testDir, "device0/add/0/dump"});
    std::string subDir = JoinPath({testDir, "device1/sub/0/dump"});
    std::string tmp0Dir = JoinPath({testDir, "tmp_dump/device0/21211/0"});
    std::string tmp1Dir = JoinPath({testDir, "tmp_dump/device1/21212/0"});
    MkdirRecusively(addDir);
    MkdirRecusively(subDir);
    MkdirRecusively(tmp0Dir);
    MkdirRecusively(tmp1Dir);
    string outputTxt = JoinPath({tmp0Dir, "output.txt"});
    std::ofstream outFile0(outputTxt.c_str(), std::ios::out | std::ios::binary);
    outFile0 << addDir << "\n";
    outFile0.close();
    chmod(outputTxt.c_str(), SAVE_DATA_FILE_AUTHORITY);
    outputTxt = JoinPath({tmp1Dir, "output.txt"});
    std::ofstream outFile1(outputTxt.c_str(), std::ios::out | std::ios::binary);
    outFile1 << subDir << "\n";
    outFile1.close();
    chmod(outputTxt.c_str(), SAVE_DATA_FILE_AUTHORITY);
    std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/duration.bin", JoinPath({tmp0Dir, "duration.bin"}));
    std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/duration.bin", JoinPath({tmp1Dir, "duration.bin"}));
    for (int i = 1; i <= 6; i++) {
        string file = "DeviceProf" + to_string(i) + ".bin";
        std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/" + file, JoinPath({tmp0Dir, file}));
        std::experimental::filesystem::copy("test/ut/resources/op_profiling/device910B/dump/" + file, JoinPath({tmp1Dir, file}));
    }
    ProfMetricsAbilityConfig metricsConfig;
    PmuEventsId pmuEventsId;
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    deviceDataParse.ParseTmpDump(JoinPath({testDir, "tmp_dump"}));
    ASSERT_TRUE(IsExist(JoinPath({addDir, "duration.bin"})));
    ASSERT_TRUE(IsExist(JoinPath({subDir, "duration.bin"})));
    for (int i = 1; i <= 6; i++) {
        string file = "DeviceProf" + to_string(i) + ".bin";
        ASSERT_TRUE(IsExist(JoinPath({addDir, file})));
        ASSERT_TRUE(IsExist(JoinPath({subDir, file})));
    }
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceTask, Run_binary_expect_success)
{
    GlobalMockObject::verify();
    MOCKER(&OpRunner::RunOpBinary)
        .stubs()
        .will(returnValue(true));
    MOCKER(&DeviceTask::PreProcess)
        .stubs()
        .will(returnValue(true));
    ProfArgs args;
    OpDeviceProf opDeviceProf(args);
    DeviceTask deviceTask{"DeviceProf", opDeviceProf};
    deviceTask.replayCount_ = 1;
    deviceTask.inExitMode = false;
    ASSERT_TRUE(deviceTask.Run());
}

TEST(DeviceTask, Run_kernel_expect_success)
{
    GlobalMockObject::verify();
    MOCKER(&DeviceTask::PreProcess)
            .stubs()
            .will(returnValue(true));
    CaseConfig caseConfig;
    std::vector<CaseConfig> caseConfigs = {caseConfig};
    MOCKER(&ParseRunConfigJson)
            .stubs()
            .will(returnValue(caseConfigs));
    MOCKER(&OpRunner::RunOpBinary)
            .stubs()
            .will(returnValue(true));
    ProfArgs args;
    OpDeviceProf opDeviceProf(args);
    DeviceTask deviceTask{"DeviceProf", opDeviceProf};
    deviceTask.replayCount_ = 1;
    deviceTask.inExitMode = false;
    deviceTask.opRunMode = OpRunnerMode::RUN_KERNEL;
    ASSERT_TRUE(deviceTask.Run());
}

TEST(DeviceTask, PreProcess_expect_success)
{
    GlobalMockObject::verify();
    MOCKER(&IsExist)
        .stubs()
        .will(returnValue(false));
    MOCKER(&MkdirRecusively)
        .stubs()
        .will(returnValue(true));
    ProfArgs args;
    args.argOutput = "./";
    OpDeviceProf opDeviceProf(args);
    DeviceTask deviceTask{"DeviceProf", opDeviceProf};
    deviceTask.opRunMode = OpRunnerMode::EXECUTE_BINARY;
    deviceTask.metrics_ = ProfMetricsAbilityConfig(false);
    deviceTask.metrics_[ProfMetrics::ARITHMETIC_UTILIZATION].isOn = true;
    deviceTask.pmuValue_.LoadPmuVec(deviceTask.metrics_, ChipType::ASCEND910B);
    ASSERT_TRUE(deviceTask.PreProcess());
    ASSERT_EQ(deviceTask.profMessage_.aicPmu[2], 1280);
    ASSERT_EQ(deviceTask.profMessage_.aivPmu[11], 1288);
    ASSERT_EQ(deviceTask.profMessage_.l2CachePmu[0], 0);
}

TEST(DeviceTask, PreProcess_mkdir_output_directory_failed)
{
    GlobalMockObject::verify();
    MOCKER(&IsExist)
            .stubs()
            .will(returnValue(false));
    MOCKER(&MkdirRecusively)
            .stubs()
            .will(returnValue(false));
    ProfArgs args;
    args.argOutput = "testOutputPath";
    OpDeviceProf opDeviceProf(args);
    DeviceTask deviceTask{"DeviceProf", opDeviceProf};
    ASSERT_FALSE(deviceTask.PreProcess());
}

TEST(DeviceTask, PreProcess_output_not_directory)
{
    GlobalMockObject::verify();
    MOCKER(&IsExist)
            .stubs()
            .will(returnValue(true));
    MOCKER(&IsDir)
            .stubs()
            .will(returnValue(false));
    ProfArgs args;
    args.argOutput = "testOutputPath";
    OpDeviceProf opDeviceProf(args);
    DeviceTask deviceTask{"DeviceProf", opDeviceProf};
    ASSERT_FALSE(deviceTask.PreProcess());
}

TEST(DeviceTask, Check_events_for_910B)
{
    GlobalMockObject::verify();
    for (const auto &pair:AIC_EVENTS_FOR_910B) {
        for (uint16_t e: pair.second) {
            ASSERT_EQ(std::count(REPLAY_AIC_EVENTS_FOR_910B.begin(), REPLAY_AIC_EVENTS_FOR_910B.end(), e), 1);
        }
    }
    for (const auto &pair:AIV_EVENTS_FOR_910B) {
        for (uint16_t e: pair.second) {
            ASSERT_EQ(std::count(REPLAY_AIV_EVENTS_FOR_910B.begin(), REPLAY_AIV_EVENTS_FOR_910B.end(), e), 1);
        }
    }
}

TEST(DeviceDataParse, Execute_Success)
{
    std::string sourceDir = "test/ut/resources/op_profiling/device910B";
    std::string testDir = "test/ut/resources/op_profiling/OPPO";
    auto kerneldir = JoinPath({testDir, "device0/add/0"});
    auto tmpdumpDir = JoinPath({testDir, "dump"});
    MkdirRecusively(kerneldir);
    char destination[PATH_MAX + 1];
    realpath(kerneldir.c_str(), destination);
    char source[PATH_MAX + 1];
    realpath(sourceDir.c_str(), source);
    CopyFolder(source, destination, destination, nullptr);

    ProfArgs  config;
    ProfMetricsAbilityConfig metricsConfig;
    config.argOutput = testDir;
    PmuEventsId pmuEventsId;
    pmuEventsId.LoadPmuVec(metricsConfig, ChipType::ASCEND910B);
    DeviceDataParse deviceDataParse(ChipType::ASCEND910B, pmuEventsId, metricsConfig);
    ASSERT_TRUE(deviceDataParse.Execute(testDir));
    deviceDataParse.SingleKernelOutputReorganize(config.argOutput);

    std::vector<std::string> fileNames;
    GetFileNames(testDir, fileNames);
    for (const auto &file : fileNames) {
        std::string filePath = JoinPath({config.argOutput, file});
        if (file == "dump" || file == "visualize_data.bin" || file.find("bbb") != string::npos) {
            continue;
        }
        std::regex pattern("[A-Za-z0-9_]*.csv");
        ASSERT_TRUE(std::regex_match(file, pattern));
    }
    std::experimental::filesystem::remove_all(testDir);
}

TEST(DeviceDataParse, bean_test)
{
    GlobalMockObject::verify();
    vector<uint16_t> events = {1, 2, 3, 4, 5, 6, 7, 8};

    vector<char> fftsBin(FFTS_LENGTH * 2);
    ReadBinaryFile("test/ut/resources/op_profiling/device910B/dump/DeviceProf1.bin", fftsBin);
    vector<char> fftsBinData{&fftsBin[0], &fftsBin[0] + FFTS_LENGTH};
    FftsBlockBean fftsBean(Common::ChipProductType::ASCEND910B_SERIES, fftsBinData);
    SplitBlockPmuData fftsData = fftsBean.GetBlockData(events, events);
    ASSERT_EQ(fftsData.totalCycles, 10000);
    ASSERT_EQ(fftsData.blockType, "vector");
    ASSERT_EQ(fftsData.pmuEventValueMap.at(1), 1001);

    vector<char> acsqBin(ACSQ_LENGTH * 2);
    ReadBinaryFile("test/ut/resources/op_profiling/device910B/dump/duration.bin", acsqBin);
    for (size_t i = 0; i < HWTS_LENGTH * 2; i = i + HWTS_LENGTH) {
        vector<char> acsqBinData{&acsqBin[i], &acsqBin[i] + ACSQ_LENGTH};
        AcsqBean acsqBean(Common::ChipProductType::ASCEND910B_SERIES, acsqBinData);
        uint64_t time = acsqBean.GetSystemTime();
        Common::TimeType type = acsqBean.GetTimeType();
        if (i == 0) {
            ASSERT_EQ(type, Common::TimeType::START);
            ASSERT_EQ(time, 30000);
        } else {
            ASSERT_EQ(type, Common::TimeType::END);
            ASSERT_EQ(time, 40000);
        }
    }

    vector<char> aiCoreBin(AICORE_LENGTH);
    ReadBinaryFile("test/ut/resources/op_profiling/device310P/dump/DeviceProf1.bin", aiCoreBin);
    vector<char> aiCoreBinData{&aiCoreBin[0], &aiCoreBin[0] + AICORE_LENGTH};
    AiCoreBean aiCoreBean(aiCoreBinData);
    SplitBlockPmuData aiCoreInfo = aiCoreBean.GetAiCoreData(events);
    ASSERT_EQ(aiCoreInfo.totalCycles, 50000);
    ASSERT_EQ(aiCoreInfo.pmuEventValueMap.at(1), 1001);

    vector<char> hwtsBin(HWTS_LENGTH * 2);
    ReadBinaryFile("test/ut/resources/op_profiling/device310P/dump/duration.bin", hwtsBin);
    for (size_t i = 0; i < HWTS_LENGTH * 2; i = i + HWTS_LENGTH) {
        vector<char> hwtsBinData{&hwtsBin[i], &hwtsBin[i] + HWTS_LENGTH};
        HwtsBean hwtsBean(hwtsBinData);
        uint64_t time = hwtsBean.GetSystemTime();
        Common::TimeType type = hwtsBean.GetTimeType();
        if (i == 0) {
            ASSERT_EQ(type, Common::TimeType::START);
            ASSERT_EQ(time, 60000);
        } else {
            ASSERT_EQ(type, Common::TimeType::END);
            ASSERT_EQ(time, 70000);
        }
    }

    vector<char> l2CacheBin(L2_CACHE_LENGTH);
    ReadBinaryFile("test/ut/resources/op_profiling/device310P/dump/L2Cache.bin", l2CacheBin);
    vector<char> l2CacheBinData{&l2CacheBin[0], &l2CacheBin[0] + L2_CACHE_LENGTH};
    L2CacheBean l2CacheBean(l2CacheBinData);
    SplitBlockPmuData l2CacheInfo = l2CacheBean.GetL2CacheData(events);
    ASSERT_EQ(aiCoreInfo.pmuEventValueMap.at(1), 1001);
}

TEST(DeviceDataParse, pmu_calculate_test)
{
    map<uint16_t, uint64_t> pmuEventValueMap = {
        {11, 100}, {4, 123}, {73, 456}, {1, 1000}, {8, 500}, {76, 100}, {77, 200}, {174, 300}, {49, 400}, {50, 750},
        {74, 44}, {19, 600}, {518, 700}, {524, 800}, {106, 102}, {120, 100}, {121, 1}, {10, 500}
    };
    set<std::string> metricItems = {"aic_total_cycles", "aic_mte1_ratio", "aic_mte1_instructions", "aic_vec_fp16_ratio",
        "aic_l1_read_bw(GB/s)", "aic_cube_time(us)", "aic_write_cache_hit", "aic_l2_cache_hit_rate(%)", "unknown"};
    Calculate cal(pmuEventValueMap, 10000, {ChipType::ASCEND310P, 1150, 8, 24, "Ascend310P3"});
    std::map<std::string, uint64_t> dbiMap;
    map<string, string> metricValues = CalMetricItems(cal, metricItems, FormulaFor310P, dbiMap);
    ASSERT_EQ(metricValues.at("aic_total_cycles"), "10000");
    ASSERT_EQ(metricValues.at("aic_mte1_ratio"), "0.010000");
    ASSERT_EQ(metricValues.at("aic_mte1_instructions"), "123");
    ASSERT_EQ(metricValues.at("aic_vec_fp16_ratio"), "0.030000");
    ASSERT_EQ(metricValues.at("aic_l1_read_bw(GB/s)"), "175.476074");
    ASSERT_EQ(metricValues.at("aic_cube_time(us)"), "0.054348");
    ASSERT_EQ(metricValues.at("aic_write_cache_hit"), "NA");
    ASSERT_EQ(metricValues.at("aic_l2_cache_hit_rate(%)"), "NA");
    ASSERT_EQ(metricValues.at("unknown"), "NA");

    set<std::string> metricItems910B = {"L1_to_GM_bw_usage_rate(%)(estimate)", "GM_to_L1_datas(KB)",
        "L1_to_GM_datas(KB)(estimate)", "L0C_to_L1_datas(KB)", "L0C_to_GM_datas(KB)", "GM_to_L1_bw_usage_rate(%)",
        "L0C_to_L1_bw_usage_rate(%)", "L0C_to_GM_bw_usage_rate(%)"};
    Calculate cal2(pmuEventValueMap, 10000, {ChipType::ASCEND910B, 1650, 20, 24, "Ascend910B4"});
    map<string, string> metricValues2 = CalMetricItems(cal2, metricItems910B, FormulaFor910B, dbiMap);
    ASSERT_EQ(metricValues2.at("L1_to_GM_bw_usage_rate(%)(estimate)"), "23.824282");
    ASSERT_EQ(metricValues2.at("GM_to_L1_datas(KB)"), "12.500000");
    ASSERT_EQ(metricValues2.at("L1_to_GM_datas(KB)(estimate)"), "287.500000");
    ASSERT_EQ(metricValues2.at("L0C_to_L1_datas(KB)"), "87.500000");
    ASSERT_EQ(metricValues2.at("L0C_to_GM_datas(KB)"), "12.500000");
    ASSERT_EQ(metricValues2.at("GM_to_L1_bw_usage_rate(%)"), "0.886015");
    ASSERT_EQ(metricValues2.at("L0C_to_L1_bw_usage_rate(%)"), "7.220070");
    ASSERT_EQ(metricValues2.at("L0C_to_GM_bw_usage_rate(%)"), "1.031439");
}

TEST(DeviceDataParse, LoadOpBasicInfoTxtFile_expect_false)
{
    DataHandler dataHandler;
    std::string filePath = "test/ut/resources/op_profiling/device910B/dump/op_basic_info.txt";
    ASSERT_FALSE(dataHandler.LoadOpBasicInfoTxtFile(filePath));
}

TEST(DeviceDataParse, SaveOpBasicInfo_expect_false)
{
    DataHandler dataHandler;
    std::string filePath = "test/ut/resources/op_profiling/device910B/dump/op_basic_info.txt";
    ASSERT_FALSE(dataHandler.SaveOpBasicInfo(filePath));
}

/**
/* | 用例集 | DeviceDataParse
/* |测试函数| ParseTimeline::GenerateBiuTimeStamps()
/* | 用例名 | test_device_timeline_get_biu_time_stamps_and_expect_success
/* |用例描述| 执行测试函数，当输入一组数据时，结果正常
*/
TEST(DeviceDataParse, test_device_timeline_get_biu_time_stamps_and_expect_success)
{
    vector<char> totalBin(2097160, 0);
    InstrProfHeadInfo headInfo;
    headInfo.coreId = 0;
    headInfo.coreType = 1;
    headInfo.validLen = 8;
    BiuPerfInfo biuPerfInfo1;
    biuPerfInfo1.cycles = 100;
    biuPerfInfo1.biuInfo = 0xf001;
    BiuPerfInfo biuPerfInfo2;
    biuPerfInfo2.cycles = 50;
    biuPerfInfo2.biuInfo = 0xf000;
    ASSERT_TRUE(memcpy_s(&totalBin[0], sizeof(totalBin), &headInfo, sizeof(InstrProfHeadInfo)) == EOK);
    ASSERT_TRUE(memcpy_s(&totalBin[8], sizeof(totalBin) - 8, &biuPerfInfo1, sizeof(BiuPerfInfo)) == EOK);
    ASSERT_TRUE(memcpy_s(&totalBin[12], sizeof(totalBin) - 12, &biuPerfInfo2, sizeof(BiuPerfInfo)) == EOK);
    ParseTimeline timelineParser;
    MOCKER(&IsReadable)
        .stubs()
        .will(returnValue(true));
    MOCKER(&GetFileSize)
        .stubs()
        .will(returnValue(size_t(2097160)));
    MOCKER(&ReadBinaryFile)
        .stubs()
        .with(any(), outBound(totalBin))
        .will(returnValue(true));
    ASSERT_TRUE(timelineParser.GenerateBiuTimeStamps("test/ut/resources/"));
    std::vector<TimelineInfo> timelineVec = timelineParser.GetTimeline();
    ASSERT_EQ(timelineVec.size(), 1);
    ASSERT_EQ(timelineVec[0].pipeName, "SCALAR");
    ASSERT_EQ(timelineVec[0].coreName, "core0.veccore0");
    ASSERT_EQ(timelineVec[0].start, 100);
    ASSERT_EQ(timelineVec[0].duration, 50);
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, SaveOpBasicInfo_expect_true)
{
    DataHandler dataHandler;
    std::string filePath = "test/ut/resources/op_profiling/device910B/dump";
    ASSERT_TRUE(dataHandler.SaveOpBasicInfo(filePath));
}

TEST(DeviceDataParse, load2d_parse_success)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(Load2DRecord), 0);
    Load2DRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L0A;
    record.repeat = 3;
    if (memcpy_s(&buffer[0], sizeof(Load2DRecord), &record, sizeof(Load2DRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseLoad2dRecord(buffer, index, 0, 0));
}

TEST(DeviceDataParse, parse_dma_mov_record)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(DmaMovRecord), 0);
    DmaMovRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L0A;
    record.nBurst = 3;
    if (memcpy_s(&buffer[0], sizeof(DmaMovRecord), &record, sizeof(DmaMovRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseDmaMovRecord(buffer, index, 0, 0));
}

TEST(DeviceDataParse, parse_dma_mov_record_byte_mode)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(DmaMovRecord), 0);
    DmaMovRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L0A;
    record.nBurst = 3;
    record.byteMode = ByteMode::BM_ENABLE;
    if (memcpy_s(&buffer[0], sizeof(DmaMovRecord), &record, sizeof(DmaMovRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseDmaMovRecord(buffer, index, 0, 0));
}

TEST(DeviceDataParse, parse_dma_mov_record_pad_mode)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(DmaMovRecord), 0);
    DmaMovRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L0A;
    record.nBurst = 3;
    record.padMode = PadMode::PAD_MODE1;
    if (memcpy_s(&buffer[0], sizeof(DmaMovRecord), &record, sizeof(DmaMovRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseDmaMovRecord(buffer, index, 0, 0));
}

TEST(DeviceDataParse, parse_mov_align_record)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(MovAlignRecord), 0);
    MovAlignRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L0A;
    record.nBurst = 3;
    if (memcpy_s(&buffer[0], sizeof(MovAlignRecord), &record, sizeof(MovAlignRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseMovAlignRecord(buffer, index, 0, 0));
}

TEST(DeviceDataParse, parse_dma_mov_nd2nz_record)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(MovAlignRecord), 0);
    DmaMovNd2nzRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L0A;
    record.ndNum = 3;
    if (memcpy_s(&buffer[0], sizeof(DmaMovNd2nzRecord), &record, sizeof(DmaMovNd2nzRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseDmaMovNd2nzRecord(buffer, index, 0, 0));
}

TEST(DeviceDataParse, parse_mov_fp_record)
{
    DBIParser dbiParser("");
    std::string buffer(sizeof(MovFpRecord), 0);
    MovFpRecord record{};
    record.pc = 11188;
    record.dstStride = 1024.;
    record.srcStride = 128;
    record.nSize = 256;
    record.mSize = 128;
    record.ndNum = 1;
    record.quantPreBits = 32;
    record.enUnitFlag = true;
    record.enNZ2ND = true;
    if (memcpy_s(&buffer[0], sizeof(MovFpRecord), &record, sizeof(MovFpRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    std::size_t index = 0;
    ASSERT_TRUE(dbiParser.ParseMovFpRecord(buffer, index, 0, 0));
}

TEST(DeviceDataParse, parse_packet)
{
    DBIParser dbiParser("");
    ProfStub::DBIDataHeader dataHeader{1, sizeof(MovFpRecord), 1, 0, 0};
    std::string buffer(sizeof(ProfStub::DBIDataHeader) + 1 + sizeof(MovFpRecord), 0);
    MovFpRecord record{};
    record.pc = 11188;
    record.dstStride = 1024.;
    record.srcStride = 128;
    record.nSize = 256;
    record.mSize = 128;
    record.ndNum = 1;
    record.quantPreBits = 32;
    record.enUnitFlag = true;
    record.enNZ2ND = true;
    if (memcpy_s(&buffer[0], sizeof(ProfStub::DBIDataHeader), &dataHeader, sizeof(ProfStub::DBIDataHeader)) != EOK) {
        printf("memcpy_s failed\n");
    }
    buffer[sizeof(ProfStub::DBIDataHeader)] = '/';
    if (memcpy_s(&buffer[sizeof(ProfStub::DBIDataHeader) + 1],
                 sizeof(MovFpRecord), &record, sizeof(MovFpRecord)) != EOK) {
        printf("memcpy_s failed\n");
    }
    dbiParser.ParsePacket(0, std::move(buffer));
}

TEST(DeviceDataParse, ParseMemoryChart_one_invalid_data) {
    DBIParser dbiParser("");
    dbiParser.ParseMemoryChart(0, "", {});
    ProfStub::DBIDataHeader dbiDataHeader{};
    dbiDataHeader.count = 1;
    RecordHeader rh;
    rh.recordType = RecordType::INVALID;
    string msg = Communication::Serialize(dbiDataHeader, rh);
    dbiParser.ParseMemoryChart(0, msg, dbiDataHeader);
    ASSERT_TRUE(dbiParser.memoryChartMetrics_[0].invalidDataCount != 0);
}

TEST(DeviceDataParse, ParseMemoryChart_memcpy_failed) {
    DBIParser dbiParser("");
    MOCKER(memcpy_s).stubs().will(returnValue(EOVERFLOW));
    ProfStub::DBIDataHeader dbiDataHeader{};
    dbiDataHeader.count = 1;
    string msg = Communication::Serialize(dbiDataHeader) + "123";
    dbiParser.ParseMemoryChart(0, msg, dbiDataHeader);
    ASSERT_TRUE(dbiParser.memoryChartMetrics_[0].memoryCopyFailed != 0);
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, ParseMemoryChart_invalid_record_type) {
    DBIParser dbiParser("");
    MovFpRecord record{};
    ProfStub::DBIDataHeader dbiDataHeader{};
    dbiDataHeader.count = 1;
    RecordHeader rh;
    rh.recordType = RecordType::INVALID;
    string msg = Communication::Serialize(dbiDataHeader, rh, record);
    dbiParser.ParseMemoryChart(0, msg, dbiDataHeader);
    ASSERT_TRUE(dbiParser.memoryChartMetrics_[0].invalidRecordType != 0);
}

TEST(DeviceDataParse, ParseMemoryChart_one_record)
{
    DBIParser dbiParser("");
    MovFpRecord record{};
    record.pc = 11188;
    record.dstStride = 1024.;
    record.srcStride = 128;
    record.nSize = 256;
    record.mSize = 128;
    record.ndNum = 1;
    record.quantPreBits = 32;
    record.enUnitFlag = true;
    record.enNZ2ND = true;
    ProfStub::DBIDataHeader dbiDataHeader{};
    dbiDataHeader.count = 1;
    RecordHeader rh;
    rh.recordType = RecordType::MOV_FP;
    string msg = Communication::Serialize(dbiDataHeader, rh, record);
    dbiParser.ParseMemoryChart(0, msg, dbiDataHeader);
    ASSERT_TRUE(dbiParser.memoryChartMetrics_[0].typeProcessed[static_cast<uint32_t>(RecordType::MOV_FP)] != 0);
}

TEST(DeviceDataParse, L2CacheLoadAndStore)
{
    auto gm = std::make_shared<GM>("GM", 512);
    L2Cache cache{{"L2Cache", gm, gm, 4, 1, 512, CachePolicy::LRU, true, true}};
    CacheOpStat opStat{true};

    opStat = cache.Load({0, 1024});
    EXPECT_EQ(opStat.hit, 0);
    EXPECT_EQ(opStat.miss, 2);
    EXPECT_EQ(opStat.allocate, 2);
    EXPECT_EQ(opStat.evictAndWrite, 0);
    EXPECT_EQ(opStat.evictWithoutWrite, 0);

    opStat = cache.Store({1000, 32});
    EXPECT_EQ(opStat.hit, 1);
    EXPECT_EQ(opStat.miss, 1);
    EXPECT_EQ(opStat.allocate, 1);
    EXPECT_EQ(opStat.evictAndWrite, 0);
    EXPECT_EQ(opStat.evictWithoutWrite, 0);

    opStat = cache.Load({2000, 256});
    EXPECT_EQ(opStat.miss, 2);
    EXPECT_EQ(opStat.allocate, 2);

    opStat = cache.Load({5000, 256});
    EXPECT_EQ(opStat.miss, 2);
    EXPECT_EQ(opStat.allocate, 2);
    EXPECT_EQ(opStat.evictAndWrite, 2);

    opStat = cache.Store({2100, 32});
    EXPECT_EQ(opStat.hit, 1);

    opStat = cache.Store({2100, 32});
    EXPECT_EQ(opStat.hit, 1);

    opStat = cache.Load({1000, 3000});
    EXPECT_EQ(opStat.hit, 2);
    EXPECT_EQ(opStat.miss, 5);
    EXPECT_EQ(opStat.allocate, 5);
    EXPECT_EQ(opStat.evictAndWrite, 0);
    EXPECT_EQ(opStat.evictWithoutWrite, 5);
}

TEST(DataHandler, ParseDurationBin_910B)
{
    std::string outputPath = "test/ut/resources/op_profiling/device910B/dump";
    string timeFilePath = Utility::JoinPath({outputPath, "duration.bin"});
    size_t fileSize = GetFileSize(timeFilePath);
    vector<char> totalBin(fileSize);
    ReadBinaryFile(timeFilePath, totalBin);
    uint64_t startTime = 0;
    uint64_t endTime = 0;

    DataHandlerOf910B dataHandler;
    dataHandler.isMC2_ = true;
    dataHandler.ParseDurationBin(outputPath, totalBin, fileSize, startTime, endTime);
    EXPECT_EQ(startTime, 30000);
    EXPECT_EQ(endTime, 40000);
    EXPECT_EQ(dataHandler.minMc2TimeCyc_, 30000);
    auto taskInfo = dataHandler.acsqTimeMap_.at({0, 0});
    EXPECT_EQ(taskInfo.startTime, 30000);
    EXPECT_EQ(taskInfo.endTime, 40000);
    EXPECT_EQ(taskInfo.taskType, 0);
}

/**
 * |  用例集  | DataHandler
 * | 测试函数 | GetOperandRecordMap
 * |  用例名  | test_GetOperandRecordMap_when_mix_return_right
 * | 用例描述 | 测试mix算子获取插桩操作数顺序正确
 */
TEST(DataHandler, test_GetOperandRecordMap_when_mix_return_right)
{
    using namespace Visualize;
    DataHandler dataHandler;
    OperandRecord Record;
    Record.instructions = 10;
    OperandRecordMap map1 = {{Common::OperandType::DATA_B4, Record}};
    OperandRecordMap map2 = {{Common::OperandType::DATA_B8, Record}};
    OperandRecordMap map3 = {{Common::OperandType::DATA_B16, Record}};
    std::vector<TypeOperandRecord> operandRecords = {{map1, map1}, {map2, map2}, {map3, map3}};
    dataHandler.operandRecords_ = operandRecords;
    auto vector0 = dataHandler.GetOperandRecordMap(0, "vector0", Common::OpType::MIX);
    ASSERT_EQ(vector0.simdMap.at(Common::OperandType::DATA_B4).instructions, 10);
    auto vector1 = dataHandler.GetOperandRecordMap(0, "vector1", Common::OpType::MIX);
    ASSERT_EQ(vector1.simdMap.at(Common::OperandType::DATA_B8).instructions, 10);
    auto cube0 = dataHandler.GetOperandRecordMap(0, "cube0", Common::OpType::MIX);
    ASSERT_EQ(cube0.simdMap.at(Common::OperandType::DATA_B16).instructions, 10);
}

/**
 * |  用例集  | DataHandler
 * | 测试函数 | GetOperandRecordMap
 * |  用例名  | test_GetOperandRecordMap_when_not_mix_return_right
 * | 用例描述 | 测试非mix算子获取插桩操作数顺序正确
 */
TEST(DataHandler, test_GetOperandRecordMap_when_not_mix_return_right)
{
    using namespace Visualize;
    DataHandler dataHandler;
    OperandRecord Record;
    Record.instructions = 10;
    OperandRecordMap map1 = {{Common::OperandType::DATA_B4, Record}};
    OperandRecordMap map2 = {{Common::OperandType::DATA_B8, Record}};
    OperandRecordMap map3 = {{Common::OperandType::DATA_B16, Record}};
    OperandRecordMap map4 = {{Common::OperandType::DATA_S8, Record}};
    OperandRecordMap map5 = {{Common::OperandType::DATA_S16, Record}};
    OperandRecordMap map6 = {{Common::OperandType::DATA_S32, Record}};
    std::vector<TypeOperandRecord> operandRecords = {{map1, map1}, {map2, map2}, {map3, map3}, {map4, map4}, {map5, map5}, {map6, map6}};
    dataHandler.operandRecords_ = operandRecords;
    auto vector0 = dataHandler.GetOperandRecordMap(0, "vector0", Common::OpType::VECTOR);
    ASSERT_EQ(vector0.simdMap.at(Common::OperandType::DATA_B4).instructions, 10);
    auto vector1 = dataHandler.GetOperandRecordMap(1, "vector0", Common::OpType::VECTOR);
    ASSERT_EQ(vector1.simdMap.at(Common::OperandType::DATA_B8).instructions, 10);
    auto vector2 = dataHandler.GetOperandRecordMap(2, "vector0", Common::OpType::VECTOR);
    ASSERT_EQ(vector2.simdMap.at(Common::OperandType::DATA_S8).instructions, 10);
    auto vector3 = dataHandler.GetOperandRecordMap(3, "vector0", Common::OpType::VECTOR);
    ASSERT_EQ(vector3.simdMap.at(Common::OperandType::DATA_S16).instructions, 10);
    auto cube0 = dataHandler.GetOperandRecordMap(0, "cube0", Common::OpType::CUBE);
    ASSERT_EQ(cube0.simdMap.at(Common::OperandType::DATA_B16).instructions, 10);
    auto cube1 = dataHandler.GetOperandRecordMap(1, "cube0", Common::OpType::CUBE);
    ASSERT_EQ(cube1.simdMap.at(Common::OperandType::DATA_S32).instructions, 10);
}

TEST(DeviceDataParse, HotSpot_ProcessBBCount_expect_false)
{
    GlobalMockObject::verify();
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    ASSERT_FALSE(hotSpotFunctionGenerator.ProcessBBCount("not_exist_dir_path"));

    std::vector<std::string> filenames;
    MOCKER(&Utility::ListDir<decltype(std::back_inserter(filenames))>)
        .stubs()
        .will(returnValue(true));
    ASSERT_FALSE(hotSpotFunctionGenerator.ProcessBBCount("not_exist_dir_path"));
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, HotSpot_ProcessBBCount_expect_true)
{
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    ASSERT_FALSE(hotSpotFunctionGenerator.ProcessBBCount("not_exist_dir_path"));
}

TEST(DeviceDataParse, HotSpot_UpdateBBBMap_expect_true)
{
    string path = "test/ut/resources/op_profiling/device910B/dump/kernel0Stub.o.bbbmap.0";
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    ASSERT_TRUE(hotSpotFunctionGenerator.UpdateBBBMap(path));
}

TEST(DeviceDataParse, HotSpot_UpdateBBBMap_expect_false)
{
    GlobalMockObject::verify();
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    ASSERT_FALSE(hotSpotFunctionGenerator.UpdateBBBMap("not_exist_dir_path"));
    MOCKER(Utility::IsReadable)
           .stubs()
           .will(returnValue(true));
    ASSERT_FALSE(hotSpotFunctionGenerator.UpdateBBBMap("not_exist_dir_path"));
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, HotSpot_UpdateExtra_expect_true)
{
    string path = "test/ut/resources/op_profiling/device910B/dump/kernel0Stub.o.bbbmap.0";
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    ASSERT_TRUE(hotSpotFunctionGenerator.UpdateExtra(path));
}

TEST(DeviceDataParse, HotSpot_UpdateExtra_expect_false)
{
    GlobalMockObject::verify();
    string path = "test/ut/resources/op_profiling/device910B/dump/kernel0Stub.o.bbbmap.0";
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    ASSERT_FALSE(hotSpotFunctionGenerator.UpdateExtra("not_exist_dir_path"));

    MOCKER(Utility::IsWritable)
            .stubs()
            .will(returnValue(false));
    ASSERT_FALSE(hotSpotFunctionGenerator.UpdateExtra(path));
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, HotSpot_GenL2cacheStat_expect_empty)
{
    GlobalMockObject::verify();
    std::vector<Common::MemRecord> memoryRecords;
    auto l2Cache1 = GetDefaultL2Cache("invalid_soc_version");
    l2Cache1->Modeling(memoryRecords);
    ASSERT_TRUE(l2Cache1->GetPcBasedCacheData().empty());
    auto l2Cache2 = GetDefaultL2Cache("Ascend910B4");
    l2Cache2->Modeling(memoryRecords);
    ASSERT_TRUE(l2Cache2->GetPcBasedCacheData().empty());
    auto l2Cache3 = GetDefaultL2Cache("Ascend310P3");
    l2Cache3->Modeling(memoryRecords);
    ASSERT_TRUE(l2Cache3->GetPcBasedCacheData().empty());
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, HotSpot_GenFdata_expect_false)
{
    GlobalMockObject::verify();
    string path = "test/ut/resources/op_profiling/device910B/dump/kernel0Stub.o.bbbmap.0";
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    ASSERT_FALSE(hotSpotFunctionGenerator.GenFdata("", "", path));
    MOCKER(&Utility::CmdExecute)
            .stubs()
            .with(any())
            .will(returnValue(true));
    ASSERT_TRUE(hotSpotFunctionGenerator.GenFdata("", "", path));
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, HotSpot_GenBBCalls_expect_true)
{
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    hotSpotFunctionGenerator.kernelStartAddr_["matmul_custom_0_mix_aic"] = 0;
    vector<string> fdata{"1 matmul_custom_0_mix_aic 0 1",
                         "1 matmul_custom_0_mix_aic 4c 1",
                         "1 matmul_custom_0_mix_aic 50 1",
                         "1 matmul_custom_0_mix_aic 58 0",
                         "1 matmul_custom_0_mix_aic 68 1"};
    ASSERT_TRUE(hotSpotFunctionGenerator.GenBBCalls(fdata));
}

TEST(DeviceDataParse, HotSpot_GenVisualizeData)
{
    string path = "test/ut/resources/op_profiling/device910B/";
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    vector<CodeFile> codeFile(1);
    codeFile[0].file = "some_file";
    codeFile[0].lines.resize(1);
    vector<InstrInfo> instrInfo(1);
    ASSERT_NO_THROW(hotSpotFunctionGenerator.GenVisualizeData(path, codeFile, instrInfo));
}

TEST(DeviceDataParse, HotSpot_GenCodeFiles_expect_true)
{
    GlobalMockObject::verify();
    string path = "test/ut/resources/op_profiling/device910B/";
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    map<string, vector<Encoding>> line2Encodings = {
        {"matmul_custom.cpp:53", {{17840, "SCALAR", "ADD.s64", "matmul_custom.cpp:53", 2, 0, 0},
                                  {17844, "SCALAR", "SUB.s64", "matmul_custom.cpp:53", 2, 0, 1}}},
        {"matmul_custom.cpp:91", {{632, "SCALAR", "INSERT", "matmul_custom.cpp:91", 1, 0, 0}}},
        {"matmul_custom.cpp:92", {{404, "SCALAR", "ADD.s64", "matmul_custom.cpp:92", 1, 0, 0},
                                  {616, "SCALAR", "LD.b32", "matmul_custom.cpp:92", 1, 0, 0}}},
    };
    vector<CodeFile> codeFiles;
    ASSERT_TRUE(hotSpotFunctionGenerator.GenCodeFiles(path, line2Encodings, codeFiles));
    MOCKER(&Utility::Visualize::CodeWriter::Write)
            .stubs();
    MOCKER(&string::empty)
            .stubs()
            .will(returnValue(false));
    ASSERT_TRUE(hotSpotFunctionGenerator.GenCodeFiles(path, line2Encodings, codeFiles));
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, HotSpot_GenInstrInfos_expect_false)
{
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    vector<InstrInfo> instrInfos;
    ASSERT_FALSE(hotSpotFunctionGenerator.GenInstrInfos(instrInfos));
}

TEST(DeviceDataParse, HotSpot_GenInstrInfos_expect_true)
{
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    vector<InstrInfo> instrInfos;
    hotSpotFunctionGenerator.encodings_[632] = Encoding{632, "SCALAR", "INSERT", "", 1, 0, 0};
    ASSERT_TRUE(hotSpotFunctionGenerator.GenInstrInfos(instrInfos));
}

TEST(EquationsUtils, Ratio_expect_success)
{
    auto res1 = Ratio(100, 20);
    ASSERT_STREQ(res1.c_str(), "5.000000");
    auto res2 = Ratio(100, 0);
    ASSERT_STREQ(res2.c_str(), "NA");
}

TEST(EquationsUtils, BandWidth_expect_success)
{
    auto res1 = BandWidth(10, 10, 100);
    ASSERT_STREQ(res1.c_str(), "100000000.000000");
    auto res2 = BandWidth(10, 0, 1);
    ASSERT_STREQ(res2.c_str(), "NA");
}

TEST(EquationsUtils, BandWidthUsage_expect_success)
{
    auto res1 = BandWidthUsage(10, 0, TransportType::GM_TO_L1, ChipProductType::ASCEND950PR_9599);
    ASSERT_STREQ(res1.c_str(), "NA");
    auto res2 = BandWidthUsage(10, 1, TransportType::UNKNOWN, ChipProductType::ASCEND950PR_9599);
    ASSERT_STREQ(res2.c_str(), "NA");
    auto res3 = BandWidthUsage(0.0001, 1, TransportType::GM_TO_L1, ChipProductType::ASCEND950PR_9599);
    ASSERT_STREQ(res3.c_str(), "1.564328");
}

TEST(EquationsUtils, GetMaxBwBySocCal_expect_success)
{
    HalHelper::Instance().gmType_ = GmType::CJ;
    auto resCJ = GetMaxBwBySoc("Ascend910B1", ChipProductType::ASCEND910B1);
    EXPECT_FLOAT_EQ(resCJ[TransportType::GM_TO_L1], 296);
    EXPECT_FLOAT_EQ(resCJ[TransportType::MTE_TO_L0A], 439.32);
    HalHelper::Instance().gmType_ = GmType::DEFAULT;
    auto res = GetMaxBwBySoc("Ascend910B1", ChipProductType::ASCEND910B1);
    EXPECT_FLOAT_EQ(res[TransportType::GM_TO_L1], 264);
    EXPECT_FLOAT_EQ(res[TransportType::MTE_TO_L0A], 437.5);
}

// A5正式出来后该函数需要修改使用真实数据校验
TEST(CalMetricItems, CalMetricItems_A5_success)
{
    std::map<uint16_t, uint64_t> pmuEventValueMap;
    for (const auto &pmu : REPLAY_AIC_EVENTS_FOR_A5) {
        pmuEventValueMap[pmu] = 1;
    }
    for (const auto &pmu : REPLAY_AIV_EVENTS_FOR_A5) {
        pmuEventValueMap[pmu] = 1;
    }
    pmuEventValueMap.erase(pmuEventValueMap.begin());
    PmuMap pmuMap(pmuEventValueMap);

    CalculateParams params = { 1000, 1000, 1, "Ascend950PR_9599", pmuMap};
    set<std::string> metricItems;
    for (const auto &temp : MetricHeaderForA5) {
        metricItems.insert(temp.second.begin(), temp.second.end());
    }
    auto res = CalMetricItems(params, metricItems, FormulaForA5);
    ASSERT_STREQ(res["aic_total_cycles"].c_str(), "1000");
    ASSERT_STREQ(res["aic_cube_total_instr_number"].c_str(), "1");
}

/**
/* | 用例集 | DeviceDataParse
/* |测试函数| DataHandlerOf91095::ParseMemoryChartData
/* | 用例名 | a5_execute_func_when_pcStart_not_found_and_expect_no_throw
/* |用例描述| 执行测试函数，不抛异常
*/
TEST(DeviceDataParse, a5_execute_func_when_pcStart_not_found_and_expect_no_throw)
{
    DataHandlerOf91095 dataHandlerA5;
    dataHandlerA5.soc_ = "Ascend950PR_9599";
    string outputPath;
    ProfMetricsAbilityConfig metrics;
    metrics.pcSamplingEnable = true;
    vector<MemRecord> memoryRecords;
    ASSERT_NO_THROW(dataHandlerA5.ParseMemoryChartData(outputPath, metrics, memoryRecords));
}

/**
/* | 用例集 | DeviceDataParse
/* |测试函数| ProcessEncoding
/* | 用例名 | process_encoding_when_soc_is_a5_and_expect_success
/* |用例描述| 执行测试函数，soc 为a5时，返回预期结果
*/
TEST(DeviceDataParse, process_encoding_when_soc_is_a5_and_expect_success)
{
    GlobalMockObject::verify();
    string kernelPath;
    string soc = "Ascend950PR_9599";
    std::shared_ptr<L2Cache> l2CachePtr;
    std::vector<Encode::EncodingInfo> instrEncodingVec = {
        {{0, 0}, 0, EncodingType::BIT32, "PIPEA", "NAMEA"}, {{1, 0}, 1, EncodingType::BIT32, "PIPEB", "NAMEB"},
        {{2, 0}, 2, EncodingType::BIT32, "PIPEC", "NAMEC"}
    };
    HotSpotFunctionGenerator hotSpotFunctionGenerator({soc, "", 0, false, true, false});
    MOCKER(&Encode::InstrEncoding::GenerateEncoding)
            .stubs()
            .with(any(), outBound(instrEncodingVec))
            .will(returnValue(true));
    ASSERT_TRUE(hotSpotFunctionGenerator.ProcessEncoding(kernelPath, l2CachePtr));
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_.size(), 3);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0].addr, 0);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[1].pipe, "PIPEB");
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[2].source, "NAMEC");
    GlobalMockObject::verify();
}

/**
/* | 用例集 | DeviceDataParse
/* |测试函数| UpdatePcSampling
/* | 用例名 | process_update_pcsampling_given_bin_and_expect_success
/* |用例描述| 执行测试函数，soc 为a5时，返回预期结果
*/
TEST(DeviceDataParse, process_update_pcsampling_given_bin_and_expect_success)
{
    GlobalMockObject::verify();

    string soc = "Ascend950PR_9599";
    string dir = "test/ut/resources/op_profiling/instr_prof";
    HotSpotFunctionGenerator hotSpotFunctionGenerator({soc, "", 0, false, true, false});
    hotSpotFunctionGenerator.startPc_ = 0;
    hotSpotFunctionGenerator.encodings_[0x28] = {0x28, "PIPEA", "NAMEA", "", 0, 0, 0, 0};  // 0x05 * 8 = 0x28
    hotSpotFunctionGenerator.encodings_[0x40] = {0x40, "PIPEB", "NAMEB", "", 0, 0, 0, 0};  // 0x08 * 8 = 0x40
    hotSpotFunctionGenerator.encodings_[0x08] = {0x08, "PIPEC", "NAMEC", "", 0, 0, 0, 0};  // 0x01 * 8 = 0x08
    hotSpotFunctionGenerator.UpdatePcSampling(dir);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x28].pcSampling.size(), 9);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x40].pcSampling.size(), 9);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x08].pcSampling.size(), 9);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x28].pcSampling[0], 257);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x28].pcSampling[8], 257);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x40].pcSampling[0], 4);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x40].pcSampling[8], 4);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x08].pcSampling[0], 9);
    ASSERT_EQ(hotSpotFunctionGenerator.encodings_[0x08].pcSampling[8], 9);
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, HotSpot_UpdateProcessBytes_expect_true)
{
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    Common::MemRecord record1;
    record1.srcAddr = 10; record1.dstAddr = 10; record1.srcMemSize = 10; record1.dstMemSize = 10; record1.pc = 10;
    record1.blockId = 10; record1.src = MemType::GM; record1.dst = MemType::GM;
    Common::MemRecord record2 = record1; record2.src = MemType::L0A;
    std::vector<Common::MemRecord> memoryRecords = {record1, record2};
    Encoding encode {10, "vector", "aaa", "cce", 10, 10, 10, 0};
    hotSpotFunctionGenerator.encodings_[10] = encode;
    hotSpotFunctionGenerator.UpdateProcessBytes(memoryRecords);
    ASSERT_TRUE(hotSpotFunctionGenerator.encodings_[10].processBytes == 20);
}

TEST(DeviceDataParse, HotSpot_ProcessEncoding_expect_true)
{
    GlobalMockObject::verify();
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    auto gm = std::make_shared<GM>("GM", 512);
    L2Cache l2{{"L2Cache", gm, gm, 4, 1, 512, CachePolicy::LRU, true, true}};
    auto ptr = Utility::MakeShared<L2Cache>(l2);
    MOCKER(&InstrEncoding::GenerateEncoding)
            .stubs()
            .will(returnValue(true));
    ASSERT_TRUE(hotSpotFunctionGenerator.ProcessEncoding("", ptr));
    GlobalMockObject::verify();
}


TEST(DeviceDataParse, HotSpot_GenLine2Encodings_expect_true)
{
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    std::map<std::string, std::vector<Encoding>> line2Encodings;
    Encoding encode {10, "vector", "aaa", "cce", 10, 10, 10, 0};
    line2Encodings["a"] = {encode};
    hotSpotFunctionGenerator.encodings_[10] = encode;
    hotSpotFunctionGenerator.bbCalls_[10] = make_pair(10,10);
    MOCKER(&Utility::IsReadable)
            .stubs()
            .will(returnValue(true));
    MOCKER(&HotSpotFunctionGenerator::GenAddr2Lines)
            .stubs()
            .will(returnValue(true));
    ASSERT_TRUE(hotSpotFunctionGenerator.GenLine2Encodings("", line2Encodings));
    GlobalMockObject::verify();
}

TEST(DeviceDataParse, HotSpot_GenAddr2Lines_expect_true)
{
    HotSpotFunctionGenerator hotSpotFunctionGenerator({"Ascend910B4", "", 0, true, false, false});
    std::vector<std::string> addrVec;
    std::unordered_map<uint64_t, std::vector<std::string>> addr2Lines;
    MOCKER(&SymbolizerParser::Parse)
            .stubs()
            .will(returnValue(true));
    ASSERT_TRUE(hotSpotFunctionGenerator.GenAddr2Lines("", addrVec, addr2Lines));
    GlobalMockObject::verify();
}

/**
 * |  用例集  | DeviceCsvEquation
 * | 测试函数 | Entry
 * |  用例名  | test_CalAicMte3ActivateBw_when_input_is_valid_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(DeviceCsvEquation, test_CalAicMte3ActivateBw_when_input_is_valid_and_expect_return_true)
{
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::string mte3Bw = cal.CalAicMte3ActivateBw(128, 999, 111, 222, 999);
    ASSERT_STREQ(mte3Bw.c_str(), "764.926270");
}

/**
 * |  用例集  | DeviceCsvEquation
 * | 测试函数 | Entry
 * |  用例名  | test_CalAicMte3ActivateBw_when_input_is_invalid_and_expect_return_NA
 * | 用例描述 | 输入数据异常时，返回NA
 */
TEST(DeviceCsvEquation, test_CalAicMte3ActivateBw_when_input_is_invalid_and_expect_return_NA)
{
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::string mte3Bw = cal.CalAicMte3ActivateBw(156, 999, 111, 999999, 12);
    ASSERT_STREQ(mte3Bw.c_str(), "NA");
}

/**
 * |  用例集  | DeviceCsvEquation
 * | 测试函数 | Entry
 * |  用例名  | test_CalAivMteActivateBw_when_input_is_valid_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(DeviceCsvEquation, test_CalAivMteActivateBw_when_input_is_valid_and_expect_return_true)
{
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::string mte3Bw = cal.CalAivMteActivateBw(128, 111, 222);
    ASSERT_STREQ(mte3Bw.c_str(), "98.347656");
}

/**
 * |  用例集  | DeviceCsvEquation
 * | 测试函数 | Entry
 * |  用例名  | test_CalAivMteActivateBw_when_input_is_invalid_and_expect_return_NA
 * | 用例描述 | 输入数据异常时，返回NA
 */
TEST(DeviceCsvEquation, test_CalAivMteActivateBw_when_input_is_invalid_and_expect_return_NA)
{
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::string mte3Bw = cal.CalAivMteActivateBw(128, 999, 0);
    ASSERT_STREQ(mte3Bw.c_str(), "NA");
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | Entry
 * |  用例名  | test_CalCulateForVecOrCubeBw_when_input_is_valid_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(VisualizeDataAccuracy, test_CalCulateForVecOrCubeBw_when_input_is_valid_and_expect_return_true)
{
    using namespace Visualize;
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr; std::shared_ptr<BasicPmu> s2 = nullptr; unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    std::map<std::string, uint64_t> basicPmu = {{"L0A Read", 100}, {"L0B Read", 100}, {"MTE1 Cyc", 10},
        {"UB MTE Read", 100}, {"MTE2 Cyc", 10}, {"UB MTE Write", 100}, {"MTE3 Cyc", 10}, {"L1 GM Write", 10}, {"L0C READ", 1}, {"FIXP Cyc", 1}};
    std::map<std::string, uint64_t> dbiRequest;
    auto bw = st.CalCulateForVecOrCubeBw(basicPmu, "vector", cal, dbiRequest);
    ASSERT_STREQ(bw["MTE1"].c_str(), "NA");
    ASSERT_STREQ(bw["MTE2"].c_str(), "1966.953247");
    ASSERT_STREQ(bw["MTE3"].c_str(), "1966.953247");
    std::map<std::string, uint64_t> dbiRequest2 {{GM_TO_L0A_DATA, 1}, {GM_TO_L0B_DATA, 1}, {GM_TO_L1, 1}};
    auto bw2 = st.CalCulateForVecOrCubeBw(basicPmu, "cube", cal, dbiRequest2);
    ASSERT_STREQ(bw2["MTE1"].c_str(), "5900.552246");
    ASSERT_STREQ(bw2["MTE2"].c_str(), "0.307336");
    ASSERT_STREQ(bw2["MTE3"].c_str(), "196.695328");
    ASSERT_STREQ(bw2["FIXP"].c_str(), "196.695328");
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | Entry
 * |  用例名  | test_CalCulateForVecOrCubeBw_when_input_is_invalid_and_expect_return_NA
 * | 用例描述 | 输入数据异常时，返回NA
 */
TEST(DeviceCsvEquation, test_CalCulateForVecOrCubeBw_when_input_is_invalid_and_expect_return_NA)
{
    using namespace Visualize;
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr; std::shared_ptr<BasicPmu> s2 = nullptr; std::unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    std::map<std::string, uint64_t> basicPmu = {};
    std::map<std::string, uint64_t> dbiRequest;
    auto bw = st.CalCulateForVecOrCubeBw(basicPmu, "vector", cal, dbiRequest);
    ASSERT_STREQ(bw["MTE1"].c_str(), "NA");
    ASSERT_STREQ(bw["MTE2"].c_str(), "NA");
    ASSERT_STREQ(bw["MTE3"].c_str(), "NA");
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | Entry
 * |  用例名  | test_CalCulateForMixBw_when_input_is_valid_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(VisualizeDataAccuracy, test_CalCulateForMixBw_when_input_is_valid_and_expect_return_true)
{
    using namespace Visualize;
    std::vector<ComputeLoadBlockDetail> computeLoadBlockDetailVec = {};
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<std::string, uint64_t> basicPmu;
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr;
    auto handler = Utility::MakeUnique<Profiling::DataHandler>();
    auto s2 = Utility::MakeShared<BasicPmu>(handler);
    std::unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    MemMapDetail detail;
    detail.eventMap[28] = 10; detail.eventMap[34] = 10; detail.eventMap[50] = 1000; detail.eventMap[518] = 100; detail.eventMap[62] = 10;
    detail.eventMap[61] = 10; detail.eventMap[12] = 30; detail.eventMap[13] = 30; detail.eventMap[770] = 20;

    detail.eventMapVec0[28] = 0; detail.eventMapVec0[34] = 0; detail.eventMapVec0[50] = 1000; detail.eventMapVec0[518] = 100; detail.eventMapVec0[62] = 10;
    detail.eventMapVec0[61] = 10; detail.eventMapVec0[12] = 30; detail.eventMapVec0[13] = 30; detail.eventMapVec0[770] = 0;

    detail.eventMapVec1[28] = 0; detail.eventMapVec1[34] = 0; detail.eventMapVec1[50] = 1000; detail.eventMapVec1[518] = 100; detail.eventMapVec1[62] = 10;
    detail.eventMapVec1[61] = 10; detail.eventMapVec1[12] = 30; detail.eventMapVec1[13] = 30; detail.eventMapVec1[770] = 0;
    PmuCalculator pmuCalculator;
    basicPmu = pmuCalculator.GetBasicPmu(detail);
    st.AddBasicPmu910B("mix", detail, basicPmu);
    std::map<std::string, uint64_t> dbiRequest {{GM_TO_L1, 100}};
    st.memoryDetail_ = true;
    auto bw = st.CalCulateForMixBw(basicPmu, cal, dbiRequest);
    ASSERT_STREQ(bw["Cube MTE1"].c_str(), "295.042999");
    ASSERT_STREQ(bw["Cube MTE2"].c_str(), "0.000000");
    ASSERT_STREQ(bw["Cube MTE3"].c_str(), "655.651123");

    ASSERT_STREQ(bw["Vector0 MTE2"].c_str(), "65.565109");
    ASSERT_STREQ(bw["Vector0 MTE3"].c_str(), "65.565109");

    ASSERT_STREQ(bw["Vector1 MTE2"].c_str(), "65.565109");
    ASSERT_STREQ(bw["Vector1 MTE3"].c_str(), "65.565109");
    GlobalMockObject::verify();
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | Entry
 * |  用例名  | test_CalCulateForMixBw_when_input_is_invalid_and_expect_return_NA
 * | 用例描述 | 输入数据异常时，返回NA
 */
TEST(VisualizeDataAccuracy, test_CalMte1ActivateBw_when_input_is_invalid_and_expect_return_NA)
{
    using namespace Visualize;
    std::vector<ComputeLoadBlockDetail> computeLoadBlockDetailVec = {};
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    std::map<std::string, uint64_t> basicPmu;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr;
    auto handler = Utility::MakeUnique<Profiling::DataHandler>();
    auto s2 = Utility::MakeShared<BasicPmu>(handler);
    std::unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    MemMapDetail detail;
    PmuCalculator pmuCalculator;
    basicPmu = pmuCalculator.GetBasicPmu(detail);
    st.AddBasicPmu910B("mix", detail, basicPmu);
    std::map<std::string, uint64_t> dbiRequest;
    auto bw = st.CalCulateForMixBw(basicPmu, cal, dbiRequest);
    ASSERT_STREQ(bw["Cube MTE1"].c_str(),    "NA");
    ASSERT_STREQ(bw["Cube MTE2"].c_str(),    "NA");
    ASSERT_STREQ(bw["Cube MTE3"].c_str(),    "NA");

    ASSERT_STREQ(bw["Vector0 MTE2"].c_str(), "NA");
    ASSERT_STREQ(bw["Vector0 MTE3"].c_str(), "NA");

    ASSERT_STREQ(bw["Vector1 MTE2"].c_str(), "NA");
    ASSERT_STREQ(bw["Vector1 MTE3"].c_str(), "NA");
    GlobalMockObject::verify();
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | StorageAccess910B::SetScalarMemInfo
 * |  用例名  | test_SetScalarMemInfo_mix_operate_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(VisualizeDataAccuracy, test_SetScalarMemInfo_mix_operate_and_expect_return_true)
{
    using namespace Visualize;
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr; std::shared_ptr<BasicPmu> s2 = nullptr; unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    st.SetScalarMemInfo("mix", basicScalarPmu, cal);
    std::vector<std::string> cubeIndex = {"1.110303", "1.150909", "1.112121", "0.855758", "0.881818", "1.076970", "0.080606", "1.117576", "1.137576"};
    std::vector<uint64_t> vec0Index = {1148UL, 4318UL, 7158UL, 1388UL, 8158UL, 318UL, 148UL, 818UL, 118UL};
    std::vector<std::string> vec1Index = {"0.695758", "2.616970", "4.338182", "0.841212", "4.944242", "0.192727", "0.089697", "0.495758", "0.071515"};
    for (auto i = 0; i < 9; i++) {
        ASSERT_STREQ(st.memInfoScalarMap_["Scalar Cube"][i].time.c_str(), cubeIndex[i].c_str());
    }
    for (auto i = 0; i < 9; i++) {
        ASSERT_EQ(st.memInfoScalarMap_["Scalar Vector Core0"][i].cycle, vec0Index[i]);
    }
    for (auto i = 0; i < 9; i++) {
        ASSERT_STREQ(st.memInfoScalarMap_["Scalar Vector Core1"][i].time.c_str(), vec1Index[i].c_str());
    }
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | StorageAccess910B::SetScalarMemInfo
 * |  用例名  | test_SetScalarMemInfo_vector_operate_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(VisualizeDataAccuracy, test_SetScalarMemInfo_vector_operate_and_expect_return_true)
{
    using namespace Visualize;
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr; std::shared_ptr<BasicPmu> s2 = nullptr; unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    st.SetScalarMemInfo("vector", basicScalarPmu, cal);
    std::vector<std::string> vecIndex = {"1.110303", "1.150909", "1.112121", "0.881818", "1.076970", "0.080606", "1.117576"};
     for (auto i = 0; i < 7; i++) {
        ASSERT_STREQ(st.memInfoScalarMap_["Scalar"][i].time.c_str(), vecIndex[i].c_str());
    }
    ASSERT_EQ(st.memInfoScalarMap_["Scalar"][7].cycle, 1478UL);
    ASSERT_EQ(st.memInfoScalarMap_["Scalar"][8].cycle, 1428UL);
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | StorageAccess910B::SetScalarMemInfo
 * |  用例名  | test_SetScalarMemInfo_cube_operate_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(VisualizeDataAccuracy, test_SetScalarMemInfo_cube_operate_and_expect_return_true)
{
    using namespace Visualize;
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr; std::shared_ptr<BasicPmu> s2 = nullptr; unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    st.SetScalarMemInfo("cube", basicScalarPmu, cal);
    std::vector<std::string> cubeIndex = {"1.110303", "1.150909", "1.112121", "0.855758", "0.881818", "1.076970", "0.080606"};
     for (auto i = 0; i < 7; i++) {
        ASSERT_STREQ(st.memInfoScalarMap_["Scalar"][i].time.c_str(), cubeIndex[i].c_str());
    }
    ASSERT_EQ(st.memInfoScalarMap_["Scalar"][7].cycle, 1844UL);
    ASSERT_EQ(st.memInfoScalarMap_["Scalar"][8].cycle, 1877UL);
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | StorageAccess910B::AddInternuclearScalarIndex
 * |  用例名  | test_AddInternuclearScalarIndex_mix_operate_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(VisualizeDataAccuracy, test_AddInternuclearScalarIndex_mix_operate_and_expect_return_true)
{
    using namespace Visualize;
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr; std::shared_ptr<BasicPmu> s2 = nullptr; unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    st.AddInternuclearScalarIndex("mix", basicScalarPmu, cal);
    std::vector<uint64_t> indexValue = {1275UL, 3542UL, 4345UL, 6514UL, 3144UL, 5315UL, 6432UL, 3152UL, 5476UL, 2453UL, 653UL, 324UL, 424UL, 653UL, 536UL, 133UL};
    for (auto i = 0; i < 16; i++) {
        ASSERT_EQ(st.memInfoScalarMap_["Scalar Cube"][i].cycle, indexValue[i]);
        ASSERT_EQ(st.memInfoScalarMap_["Scalar Vector Core0"][i].cycle, indexValue[i]);
        ASSERT_EQ(st.memInfoScalarMap_["Scalar Vector Core1"][i].cycle, indexValue[i]);
    }
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | StorageAccess910B::AddInternuclearScalarIndex
 * |  用例名  | test_AddInternuclearScalarIndex_cube_operate_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(VisualizeDataAccuracy, test_AddInternuclearScalarIndex_cube_operate_and_expect_return_true)
{
    using namespace Visualize;
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr; std::shared_ptr<BasicPmu> s2 = nullptr; unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    st.AddInternuclearScalarIndex("cube", basicScalarPmu, cal);
    std::vector<uint64_t> indexValue = {1275UL, 3542UL, 4345UL, 6514UL, 3144UL, 5315UL, 6432UL, 3152UL, 5476UL, 2453UL, 653UL, 324UL, 424UL, 653UL, 536UL, 133UL};
    for (auto i = 0; i < 16; i++) {
        ASSERT_EQ(st.memInfoScalarMap_["Scalar"][i].cycle, indexValue[i]);
    }
}

/**
 * |  用例集  | VisualizeDataAccuracy
 * | 测试函数 | StorageAccess910B::AddInternuclearScalarIndex
 * |  用例名  | test_AddInternuclearScalarIndex_vector_operate_and_expect_return_true
 * | 用例描述 | 输入没有问题时，返回正确的计算结果
 */
TEST(VisualizeDataAccuracy, test_AddInternuclearScalarIndex_vector_operate_and_expect_return_true)
{
    using namespace Visualize;
    Profiling::CalDeviceInfo calDeviceInfo = {Common::ChipType::ASCEND910B, 1650, 24, 8, "Ascend910B4"};
    std::map<uint16_t, uint64_t> pmuMap;
    Profiling::Calculate cal(pmuMap, 111111, calDeviceInfo);
    std::shared_ptr<OpBasicInfo> s1 = nullptr; std::shared_ptr<BasicPmu> s2 = nullptr; unique_ptr<PmuCalculator> s3 = nullptr;
    StorageAccess910B st(s1, s2, s3);
    st.AddInternuclearScalarIndex("vector", basicScalarPmu, cal);
    std::vector<uint64_t> indexValue = {1275UL, 3542UL, 4345UL, 6514UL, 3144UL, 5315UL, 6432UL, 3152UL, 5476UL, 2453UL, 653UL, 324UL, 424UL, 653UL, 536UL, 133UL};
    for (auto i = 0; i < 16; i++) {
        ASSERT_EQ(st.memInfoScalarMap_["Scalar"][i].cycle, indexValue[i]);
    }
}

