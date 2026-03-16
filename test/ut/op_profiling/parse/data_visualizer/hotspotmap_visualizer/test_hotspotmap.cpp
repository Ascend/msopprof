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
#define private public
#include "parse/data_visualizer/hotspotmap_visualizer/hotspotmap_visualizer.h"
#include "parse/data_visualizer/hotspotmap_visualizer/sim_code_to_pc.h"
#include "parse/data_visualizer/hotspotmap_visualizer/sim_pc_to_code.h"
#undef private
#include "parse/data_visualizer/sim_visualizer_config.h"
#include "profiling/simulator/data_parse/sim_defs.h"
#include "filesystem.h"
#include "../test_data.h"

using namespace Profiling::Parse;
using namespace Utility;
using namespace Profiling;
using namespace std;

namespace Visualize {
/**
* |  用例集 | HotSpot
* | 测试函数 | Entry
* |  用例名  | test_HotSpot_910B_should_return_ture_when_parse_ok
* | 用例描述 | HotSpot的st，检查解析正常的全部功能
*/
TEST(HotSpot, test_HotSpot_910B_should_return_ture_when_parse_ok) {
    const std::string output = "test/ut/resources/dump/output";
    const std::string fileName = "test/ut/resources/dump/output/visualize_data.bin";
    const std::string sepCoreOutput = JoinPath({output, "core0.veccore0"});
    std::vector<std::string> fileNames;
    Utility::MkdirRecusively(sepCoreOutput);
    DataCenter dataCenter;
    dataCenter.DataTableRegister<std::map<std::string, SimData>>(GetSimData());
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    HotSpotMapVisualizer hotSpot {dataCenter, config};

    ASSERT_EQ(hotSpot.Entry(), PluginErrorCode::SUCCESS);
    ASSERT_TRUE(Utility::IsExist(fileName));
    ASSERT_TRUE(GetFileNames(sepCoreOutput, fileNames));
    bool searchCode = false;
    bool searchInstr = false;
    std::string csv1 = "core0.veccore0_code_exe";
    std::string csv2 = "core0.veccore0_instr_ex";
    for (const auto &file : fileNames) {
        if (file.substr(0, 23) == csv1) {
            searchCode = true;
        }
        if (file.substr(0, 23) == csv2) {
            searchInstr = true;
        }
    }
    ASSERT_TRUE(searchCode && searchInstr);
    std::experimental::filesystem::remove_all(output);
}

/**
* |  用例集 | HotSpot
* | 测试函数 | Entry
* |  用例名  | test_Entry_should_return_error_when_db_not_register
* | 用例描述 | 测试DB未注册时Entry返回error
*/
TEST(HotSpot, test_Entry_should_return_error_when_db_not_register) {
    const std::string output = "test/ut/resources/dump/output";
    DataCenter dataCenter;
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    HotSpotMapVisualizer hotSpot {dataCenter, config};
    ASSERT_EQ(hotSpot.Entry(), PluginErrorCode::NONBLOCKING_ERROR);
}

/**
* |  用例集 | HotSpot
* | 测试函数 | ParseCoreInfo
* |  用例名  | test_ParseCoreInfo_expect_true_when_instrs_time_cal_right
* | 用例描述 | 测试正确计算当前核上指令的运行时间
*/
TEST(HotSpot, test_ParseCoreInfo_expect_true_when_instrs_time_cal_right) {
    const std::string output = "test/ut/resources/dump/output";
    std::vector<std::string> fileNames;
    DataCenter dataCenter;
    dataCenter.DataTableRegister<std::map<std::string, SimData>>(GetSimData());
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    HotSpotMapVisualizer hotSpot {dataCenter, config};

    MergeInfo m1; m1.startTick = 1; m1.endTick = 2; m1.tick = 1; m1.pc = 0x10cfa000; m1.pipe = "SCALAR"; m1.name = "MOV_XD_IMM"; m1.detail = "XD:X0=0, IMM:0x1,";
    MergeInfo m2; m2.startTick = 2; m2.endTick = 1851; m2.tick = 1849; m2.pc = 0x10cfa004; m1.pipe = "MTE2"; m1.name = "DMA_MOV"; m1.detail = "Src:OUT, Dst:UB, XD:X0=0";
    std::vector<MergeInfo> instrs = {m1, m2};
    std::string coreName = "core0.veccore0";
    ASSERT_TRUE(hotSpot.ParseCoreInfo(coreName, instrs));
    ASSERT_EQ(hotSpot.exeStatList_.size(), 1);
    ASSERT_EQ(hotSpot.exeStatList_[0].coreName, coreName);
    ASSERT_FLOAT_EQ(hotSpot.exeStatList_[0].durationTimeUs, 1);
    ASSERT_FLOAT_EQ(hotSpot.exeStatList_[0].runningTime, 1);
}

/**
* |  用例集 | HotSpot
* | 测试函数 | ParseCoreInfo
* |  用例名  | test_ParseCoreInfo_expect_false_when_instrs_tick_error
* | 用例描述 | 测试指令tick有误时返回false
*/
TEST(HotSpot, test_ParseCoreInfo_expect_false_when_instrs_tick_error) {
    const std::string output = "test/ut/resources/dump/output";
    std::vector<std::string> fileNames;
    DataCenter dataCenter;
    dataCenter.DataTableRegister<std::map<std::string, SimData>>(GetSimData());
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    HotSpotMapVisualizer hotSpot {dataCenter, config};

    MergeInfo m1; m1.startTick = 2; m1.endTick = 1; m1.tick = 1; m1.pc = 0x10cfa000; m1.pipe = "SCALAR"; m1.name = "MOV_XD_IMM"; m1.detail = "XD:X0=0, IMM:0x1,";
    std::vector<MergeInfo> instrs = {m1};
    std::string coreName = "core0.veccore0";
    ASSERT_FALSE(hotSpot.ParseCoreInfo(coreName, instrs));
    ASSERT_EQ(hotSpot.exeStatList_.size(), 0);
}

/**
* |  用例集 | HotSpot
* | 测试函数 | ParseCoreInfo
* |  用例名  | test_ParseCoreInfo_expect_skip_when_instrs_tick_error
* | 用例描述 | 测试指令tick有误时返回跳过,minStart < maxEnd 但存在info.startTick > info.endTick
*/
TEST(HotSpot, test_ParseCoreInfo_expect_skip_when_instrs_tick_error) {
    const std::string output = "test/ut/resources/dump/output";
    std::vector<std::string> fileNames;
    DataCenter dataCenter;
    dataCenter.DataTableRegister<std::map<std::string, SimData>>(GetSimData());
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    HotSpotMapVisualizer hotSpot {dataCenter, config};

    MergeInfo m1; m1.startTick = 2; m1.endTick = 1; m1.tick = 1; m1.pc = 0x10cfa000; m1.pipe = "SCALAR"; m1.name = "MOV_XD_IMM"; m1.detail = "XD:X0=0, IMM:0x1,";
    MergeInfo m2; m2.startTick = 2; m2.endTick = 1851; m2.tick = 1849; m2.pc = 0x10cfa004; m1.pipe = "MTE2"; m1.name = "DMA_MOV"; m1.detail = "Src:OUT, Dst:UB, XD:X0=0";
    std::vector<MergeInfo> instrs = {m1, m2};
    std::string coreName = "core0.veccore0";
    ASSERT_TRUE(hotSpot.ParseCoreInfo(coreName, instrs));
    ASSERT_EQ(hotSpot.exeStatList_.size(), 1);
    ASSERT_EQ(hotSpot.exeStatList_[0].coreName, coreName);
    ASSERT_FLOAT_EQ(hotSpot.exeStatList_[0].durationTimeUs, 1);
    ASSERT_FLOAT_EQ(hotSpot.exeStatList_[0].runningTime, 1);
}

/**
 * |  用例集  | DataVisualize
 * | 测试函数 | MergeInstr
 * |  用例名  | SimPcToCode_mergeInstr
 * | 用例描述 | merge instr
 */
TEST(DataVisualize, SimPcToCode_mergeInstr) {
    MergeInfo m1;
    m1.startTick = 1;
    m1.endTick = 2;
    m1.tick = 1;
    m1.pc = 0x10cfa000;
    m1.pipe = "SCALAR";
    m1.name = "MOV_XD_IMM";
    m1.detail = "XD:X0=0, IMM:0x1,";
    MergeInfo m2;
    m2.startTick = 2;
    m2.endTick = 3;
    m2.tick = 2;
    m2.pc = 0x10cfa004;
    m1.pipe = "MTE2";
    m1.name = "DMA_MOV";
    m1.detail = "Src:OUT, Dst:UB, XD:X0=0";
    std::vector<MergeInfo> mergeList = {m1, m2};
    std::vector<std::string> cores = {"core0"};
    const std::string output = "test/ut/resources/dump/output";
    DataCenter dataCenter;
    dataCenter.DataTableRegister<std::map<std::string, SimData>>(GetSimData());
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    SimPcToCode simPcToCode{config, dataCenter, cores};
    simPcToCode.MergeInstr(mergeList);
    ASSERT_EQ(simPcToCode.cores_.size(), 1);
}

/**
* |  用例集  | DataVisualize
* | 测试函数 | Statistic
* |  用例名  | SimPcToCode_statistic
* | 用例描述 | pc statistic
*/
TEST(DataVisualize, SimPcToCode_statistic) {
    const std::string output = "test/ut/resources/dump/output";
    const std::string coreName = "core0";
    std::vector<std::string> cores = {"core0"};

    std::shared_ptr<InstrDetailTable> instrPtr;
    std::shared_ptr<CacheDetailTable> cachePtr;
    std::shared_ptr<UserMarkStruct> userMarkPtr;
    MergeInfo m1;
    m1.pc = 0x10f86004;
    m1.startTick = 11035;
    m1.endTick = 11036;
    m1.pipe = "VECTOR";
    m1.name = SET_FLAG;
    m1.detail = "PIPE:VEC,TRIGGERPIPE:MTE3,FLAGID:0,";
    MergeInfo m2;
    m2.pc = 0x10f86000;
    m2.startTick = 11030;
    m2.endTick = 11036;
    m2.pipe = "MTE3";
    m2.name = WAIT_FLAG;
    m2.detail = "PIPE:VEC,TRIGGERPIPE:MTE3,FLAGID:0,";
    std::vector<MergeInfo> mergeVec {m1, m2};
    InstrDetailTable instr(mergeVec);

    MergeInfo cache;
    cache.pc = 0x10f86008;
    cache.startTick = 11038;
    cache.endTick = 11038;
    cache.name = "0x10f86008";
    cache.pipe = "CACHEMISS";
    std::vector<MergeInfo> cacheVec {cache};
    cachePtr = Utility::MakeShared<CacheDetailTable>(cacheVec);
    instrPtr = Utility::MakeShared<InstrDetailTable>(instr);

    std::map<std::string, std::vector<UserMarkInfo>> userMarkInfos;
    UserMarkInfo uu;
    uu.startTick = 11040;
    uu.endTick = 11050;
    uu.startPc = 0x10f86000;
    uu.endPc = 0x10f86004;
    MergeInfo userMark;
    userMark.pc = 0x10f86000;
    userMark.pipe = "USERMARK";
    userMark.name = "Mark 0x1";
    userMark.startTick = 11040;
    userMark.endTick = 11050;
    std::vector<MergeInfo> userMarkInstrs = {userMark};
    userMarkInfos["Mark 0x1"] = std::vector<UserMarkInfo> {uu};
    UserMarkStruct userMarkStruct {userMarkInfos, userMarkInstrs};
    userMarkPtr = Utility::MakeShared<UserMarkStruct>(userMarkStruct);
    SimData data = {instrPtr, cachePtr, userMarkPtr};

    DataCenter dataCenter;
    dataCenter.DataTableRegister<std::map<std::string, SimData>>(GetSimData());
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);

    SimPcToCode simPcToCode {config, dataCenter, cores};
    simPcToCode.Statistic(coreName, data);
    simPcToCode.GetInstrInfo();
    simPcToCode.CalCulate();
    ASSERT_EQ(simPcToCode.cores_.size(), 1);
}

/**
* |  用例集  | DataVisualize
* | 测试函数 | UpdateInstr
* |  用例名  | SimPcToCode_updateInstr_waitevent_instr_expect_wait_event_l1
* | 用例描述 | update wait event
*/
TEST(DataVisualize, SimPcToCode_updateInstr_waitevent_instr_expect_wait_event_l1) {
    const string coreName = "core0";
    std::vector<string> cores = {"core0"};
    std::string socVersion = "Ascend310P1";
    std::string setFlagName = "set_event";
    std::string waitFlagName = "wait_event";
    std::regex waitSetPattern{"pipe_type:([A-Za-z0-9]+),tigger_pipe:([A-Za-z0-9]+),event_id:([A-Za-z0-9]+)"};
    std::string instr = "wait_event pipe_type:L1,tigger_pipe:L2,event_id:1";
    std::string expectInstr = "wait_event L1 ID:1";
    const std::string output = "test/ut/resources/dump/output";

    DataCenter dataCenter;
    dataCenter.DataTableRegister<std::map<std::string, SimData>>(GetSimData());
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    SimPcToCode simPcToCode {config, dataCenter, cores};
    std::string realInstr = simPcToCode.UpdateInstr(instr, waitSetPattern, waitFlagName, setFlagName);
    EXPECT_EQ(realInstr, expectInstr);
    ASSERT_EQ(simPcToCode.cores_.size(), 1);
}

/**
* |  用例集  | DataVisualize
* | 测试函数 | UpdateInstr
* |  用例名  | SimPcToCode_updateInstr_other_instr_expect_no_modify
* | 用例描述 | update other instr
*/
TEST(Serialize, SimPcToCode_updateInstr_other_instr_expect_no_modify) {
    std::string coreName = "core0";
    std::vector<string> cores = {"core0"};
    std::string socVersion = "Ascend910B1";
    std::string setFlagName = "SET_FLAG";
    std:: string waitFlagName = "WAIT_FLAG";
    std::regex waitSetPattern{"PIPE:([A-Za-z0-9]+),TRIGGERPIPE:([A-Za-z0-9]+),FLAGID:([A-Za-z0-9]+)"};

    std::string instr = "ST_XD_XN_IMM dtype:B32,XD:X3,XN:X7,IMM";
    std::string expectInstr = "ST_XD_XN_IMM dtype:B32,XD:X3,XN:X7,IMM";
    const std::string output = "test/ut/resources/dump/output";

    DataCenter dataCenter;
    dataCenter.DataTableRegister<std::map<std::string, SimData>>(GetSimData());
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    SimPcToCode simPcToCode {config, dataCenter, cores};
    std::string realInstr = simPcToCode.UpdateInstr(instr,waitSetPattern, waitFlagName, setFlagName);

    EXPECT_EQ(realInstr, expectInstr);
    ASSERT_EQ(simPcToCode.cores_.size(), 1);
}

/**
* |  用例集  | DataVisualize
* | 测试函数 | UpdateInstr
* |  用例名  | SimPcToCode_updateInstr_error_format_instr_expect_no_modify
* | 用例描述 | update error instr
*/
TEST(Serialize, SimPcToCode_updateInstr_error_format_instr_expect_no_modify) {
    vector<string> cores = {"core0"};
    std::string socVersion = "Ascend910B1";
    std::string setFlagName = "SET_FLAG";
    std::string waitFlagName = "WAIT_FLAG";
    std::regex waitSetPattern{"PIPE:([A-Za-z0-9]+),TRIGGERPIPE:([A-Za-z0-9]+),FLAGID:([A-Za-z0-9]+)"};

    std::string instr = "ST_XD_XN_IMMdtype:B32,XD:X3,XN:X7,IMM";
    std::string expectInstr = "ST_XD_XN_IMMdtype:B32,XD:X3,XN:X7,IMM";
    const std::string output = "test/ut/resources/dump/output";

    DataCenter dataCenter;
    dataCenter.DataTableRegister<std::map<std::string, SimData>>(GetSimData());
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    SimPcToCode simPcToCode {config, dataCenter, cores};
    std::string realInstr = simPcToCode.UpdateInstr(instr, waitSetPattern, waitFlagName, setFlagName);

    EXPECT_EQ(realInstr, expectInstr);
    ASSERT_EQ(simPcToCode.cores_.size(), 1);
}

/**
* |  用例集  | DataVisualize
* | 测试函数 | UpdateInstr
* |  用例名  | SimPcToCode_updateInstr_waitflag_instr_not_complete_expect_no_modify
* | 用例描述 | update wait flag instr
*/
TEST(Serialize, SimPcToCode_updateInstr_waitflag_instr_not_complete_expect_no_modify) {
    vector<string> cores = {"core0"};
    std::string socVersion = "Ascend910B1";
    std::string setFlagName = "SET_FLAG";
    std::string waitFlagName = "WAIT_FLAG";
    std::regex waitSetPattern{"PIPE:([A-Za-z0-9]+),TRIGGERPIPE:([A-Za-z0-9]+),FLAGID:([A-Za-z0-9]+)"};

    std::string instr = "WAIT_FLAG PIPE:MTE3,TRIGGERPIPE";
    std::string expectInstr = "WAIT_FLAG PIPE:MTE3,TRIGGERPIPE";
    const std::string output = "test/ut/resources/dump/output";

    DataCenter dataCenter;
    dataCenter.DataTableRegister<std::map<std::string, SimData>>(GetSimData());
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    SimPcToCode simPcToCode {config, dataCenter, cores};
    std::string realInstr = simPcToCode.UpdateInstr(instr, waitSetPattern, waitFlagName, setFlagName);

    EXPECT_EQ(realInstr, expectInstr);
    ASSERT_EQ(simPcToCode.cores_.size(), 1);
}

/**
* |  用例集  | DataVisualize
* | 测试函数 | UpdateInstr
* |  用例名  | SimPcToCode_updateInstr_waitevent_instr_not_complete_expect_no_modify
* | 用例描述 | update wait event not complete instr
*/
TEST(Serialize, SimPcToCode_updateInstr_waitevent_instr_not_complete_expect_no_modify) {
    vector<string> cores = {"core0"};
    std::string socVersion = "Ascend310P1";
    std::string setFlagName = "set_event";
    std::string waitFlagName = "wait_event";
    std::regex waitSetPattern{"pipe_type:([A-Za-z0-9]+),tigger_pipe:([A-Za-z0-9]+),event_id:([A-Za-z0-9]+)"};

    std::string instr = "wait_event pipe_type:L1,tigger_pipe";
    std::string expectInstr = "wait_event pipe_type:L1,tigger_pipe";
    const std::string output = "test/ut/resources/dump/output";

    DataCenter dataCenter;
    dataCenter.DataTableRegister<std::map<std::string, SimData>>(GetSimData());
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    SimPcToCode simPcToCode {config, dataCenter, cores};

    std::string realInstr = simPcToCode.UpdateInstr(instr, waitSetPattern, waitFlagName, setFlagName);

    EXPECT_EQ(realInstr, expectInstr);
    ASSERT_EQ(simPcToCode.cores_.size(), 1);
}

/**
* |  用例集  | DataVisualize
* | 测试函数 | UpdateInstr
* |  用例名  | SimPcToCode_updateInstr_setflag_instr_expect_set_flag_vec
* | 用例描述 | update set flag instr instr
*/
TEST(Serialize,SimPcToCode_updateInstr_setflag_instr_expect_set_flag_vec) {
    vector<string> cores = {"core0"};
    std::string socVersion = "Ascend910B1";
    std::string setFlagName = "SET_FLAG";
    std::string waitFlagName = "WAIT_FLAG";
    std::regex waitSetPattern{"PIPE:([A-Za-z0-9]+),TRIGGERPIPE:([A-Za-z0-9]+),FLAGID:([A-Za-z0-9]+)"};

    std::string instr = "SET_FLAG PIPE:MTE3,TRIGGERPIPE:VEC,FLAGID:0,";
    std::string expectInstr = "SET_FLAG VEC ID:0";
    const std::string output = "test/ut/resources/dump/output";

    DataCenter dataCenter;
    dataCenter.DataTableRegister<std::map<std::string, SimData>>(GetSimData());
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    SimPcToCode simPcToCode {config, dataCenter, cores};
    std::string realInstr = simPcToCode.UpdateInstr(instr, waitSetPattern, waitFlagName, setFlagName);

    EXPECT_EQ(realInstr, expectInstr);
    ASSERT_EQ(simPcToCode.cores_.size(), 1);
}

/**
* |  用例集  | DataVisualize
* | 测试函数 | UpdateInstr
* |  用例名  | SimPcToCode_updateInstr_setevent_instr_expect_set_event_l2
* | 用例描述 | update set event
*/
TEST(Serialize, SimPcToCode_updateInstr_setevent_instr_expect_set_event_l2) {
    vector<string> cores = {"core0"};
    const string socVersion = "Ascend310P1";
    const string setFlagName = "set_event";
    const string waitFlagName = "wait_event";
    std::regex waitSetPattern{"pipe_type:([A-Za-z0-9]+),tigger_pipe:([A-Za-z0-9]+),event_id:([A-Za-z0-9]+)"};

    const string instr = "set_event pipe_type:L1,tigger_pipe:L2,event_id:1";
    const std::string expectInstr = "set_event L2 ID:1";
    const std::string output = "test/ut/resources/dump/output";

    DataCenter dataCenter;
    dataCenter.DataTableRegister<std::map<std::string, SimData>>(GetSimData());
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    SimPcToCode simPcToCode {config, dataCenter, cores};
    std::string realInstr = simPcToCode.UpdateInstr(instr, waitSetPattern, waitFlagName, setFlagName);

    EXPECT_EQ(realInstr, expectInstr);
    ASSERT_EQ(simPcToCode.cores_.size(), 1);
}

/**
* |  用例集  | DataVisualize
* | 测试函数 | UpdateInstr
* |  用例名  | SimPcToCode_updateInstr_waitflag_instr_expect_wait_flag_mte3
* | 用例描述 | update wait event instr
*/
TEST(Serialize, SimPcToCode_updateInstr_waitflag_instr_expect_wait_flag_mte3) {
    vector<string> cores = {"core0"};
    std::string socVersion = "Ascend910B1";
    std::string setFlagName = "SET_FLAG";
    std::string waitFlagName = "WAIT_FLAG";
    std::regex waitSetPattern{"PIPE:([A-Za-z0-9]+),TRIGGERPIPE:([A-Za-z0-9]+),FLAGID:([A-Za-z0-9]+)"};

    std::string instr = "WAIT_FLAG PIPE:MTE3,TRIGGERPIPE:VEC,FLAGID:0,";
    std::string expectInstr = "WAIT_FLAG MTE3 ID:0";
    const std::string output = "test/ut/resources/dump/output";

    DataCenter dataCenter;
    dataCenter.DataTableRegister<std::map<std::string, SimData>>(GetSimData());
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    SimPcToCode simPcToCode {config, dataCenter, cores};
    std::string realInstr = simPcToCode.UpdateInstr(instr, waitSetPattern, waitFlagName, setFlagName);

    EXPECT_EQ(realInstr, expectInstr);
    ASSERT_EQ(simPcToCode.cores_.size(), 1);
}

/**
* |  用例集  | DataVisualize
* | 测试函数 | Statistic
* |  用例名  | SimCodeToPc_statistic_should_run_no_error
* | 用例描述 | code statistic
*/
TEST(DataVisualize, SimCodeToPc_statistic_should_run_no_error) {
    const std::string coreName = "core0";
    std::vector<std::string> cores = {"core0"};

    std::shared_ptr<InstrDetailTable> instrPtr;
    std::shared_ptr<CacheDetailTable> cachePtr;
    std::shared_ptr<UserMarkStruct> userMarkPtr;
    MergeInfo m1;
    m1.pc = 0x10f86004;
    m1.startTick = 11035;
    m1.endTick = 11036;
    m1.pipe = "VECTOR";
    m1.name = SET_FLAG;
    m1.detail = "PIPE:VEC,TRIGGERPIPE:MTE3,FLAGID:0,";
    MergeInfo m2;
    m2.pc = 0x10f86000;
    m2.startTick = 11030;
    m2.endTick = 11036;
    m2.pipe = "MTE3";
    m2.name = WAIT_FLAG;
    m2.detail = "PIPE:VEC,TRIGGERPIPE:MTE3,FLAGID:0,";
    std::vector<MergeInfo> mergeVec {m1, m2};
    InstrDetailTable instr(mergeVec);

    MergeInfo cache;
    cache.pc = 0x10f86008;
    cache.startTick = 11038;
    cache.endTick = 11038;
    cache.name = "0x10f86008";
    cache.pipe = "CACHEMISS";
    std::vector<MergeInfo> cacheVec {cache};
    cachePtr = Utility::MakeShared<CacheDetailTable>(cacheVec);
    instrPtr = Utility::MakeShared<InstrDetailTable>(instr);

    std::map<std::string, std::vector<UserMarkInfo>> userMarkInfos;
    UserMarkInfo uu;
    uu.startTick = 11040;
    uu.endTick = 11050;
    uu.startPc = 0x10f86000;
    uu.endPc = 0x10f86004;
    MergeInfo userMark;
    userMark.pc = 0x10f86000;
    userMark.pipe = "USERMARK";
    userMark.name = "Mark 0x1";
    userMark.startTick = 11040;
    userMark.endTick = 11050;
    std::vector<MergeInfo> userMarkInstrs = {userMark};
    userMarkInfos["Mark 0x1"] = std::vector<UserMarkInfo> {uu};
    UserMarkStruct userMarkStruct {userMarkInfos, userMarkInstrs};
    userMarkPtr = Utility::MakeShared<UserMarkStruct>(userMarkStruct);
    SimData data = {instrPtr, cachePtr, userMarkPtr};

    const std::string output = "test/ut/resources/dump/output";
    DataCenter dataCenter;
    dataCenter.DataTableRegister<std::map<std::string, SimData>>(GetSimData());
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    SimCodeToPc simCodeToPc {config, dataCenter, cores};
    simCodeToPc.Statistic(coreName, data);
    simCodeToPc.CalCulate();
    ASSERT_EQ(simCodeToPc.cores_.size(), 1);
}
}
