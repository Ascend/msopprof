#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"
#define private public
#include "parse/data_visualizer/timeline_visualizer/core_timeline/core_timeline_visualizer.h"
#undef private
#include "parse/data_visualizer/sim_visualizer_config.h"
#include "parse/data_visualizer/utility.h"
#include "profiling/simulator/data_parse/sim_defs.h"
#include "filesystem.h"
#include "../test_data.h"

using namespace Profiling::Parse;
using namespace Utility;
using namespace Profiling;

namespace Visualize {
/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | Entry
* |  用例名  | test_CoreTimeLineVisualizer_910B_should_return_ture_when_parse_ok
* | 用例描述 | CoreTimeLineVisualizer的st，检查解析正常的全部功能
*/
TEST(CoreTimeLineVisualizer, test_CoreTimeLineVisualizer_910B_should_return_ture_when_parse_ok)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    const std::string fileName1 = "test/ut/resources/dump/output/trace.json";
    const std::string fileName2 = "test/ut/resources/dump/output/visualize_data.bin";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    std::vector<std::string> fileNames;
    MkdirRecusively(output);
    CoreTimeLineVisualizer core(dataCenter, config);
    ASSERT_TRUE(core.Entry() == PluginErrorCode::SUCCESS);
    ASSERT_TRUE(IsExist(fileName1));
    ASSERT_TRUE(IsExist(fileName2));
    std::experimental::filesystem::remove_all(output);
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | Entry
* |  用例名  | test_Entry_should_return_error_when_db_not_register
* | 用例描述 | 测试DB未注册时Entry返回error
*/
TEST(CoreTimeLineVisualizer, test_Entry_should_return_error_when_db_not_register)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    CoreTimeLineVisualizer core(dataCenter, config);
    ASSERT_EQ(core.Entry(), PluginErrorCode::NONBLOCKING_ERROR);
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | WriteFile
* |  用例名  | test_WriteFile_output_file_generate_success
* | 用例描述 | 测试生成交付件visualize_data.bin和trace.json成功
*/
TEST(CoreTimeLineVisualizer, test_WriteFile_output_file_generate_success)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    const std::string fileName1 = "test/ut/resources/dump/output/trace.json";
    const std::string fileName2 = "test/ut/resources/dump/output/visualize_data.bin";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    std::vector<std::string> fileNames;
    MkdirRecusively(output);
    CoreTimeLineVisualizer core(dataCenter, config);
    core.WriteFile(output);
    ASSERT_TRUE(IsExist(fileName1));
    ASSERT_TRUE(IsExist(fileName2));
    std::experimental::filesystem::remove_all(output);
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | ParseByCore
* |  用例名  | test_ParseByCore_should_return_ture_when_parse_success
* | 用例描述 | 测试解析单核仿真数据json生成正确
*/
TEST(CoreTimeLineVisualizer, test_ParseByCore_should_return_ture_when_parse_success)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    CoreTimeLineVisualizer core(dataCenter, config);
    std::string coreName = "core0.veccore0";
    ASSERT_TRUE(core.ParseByCore(coreName, simData->at(coreName)));
    ASSERT_EQ(core.coresJsonList_.size(), 33);
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | ParseByCore
* |  用例名  | test_ParseByCore_should_return_false_when_InstrDetailTable_nullptr
* | 用例描述 | 测试当InstrDetailTable未初始化时解析单核仿真数据失败
*/
TEST(CoreTimeLineVisualizer, test_ParseByCore_should_return_false_when_InstrDetailTable_nullptr)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    CoreTimeLineVisualizer core(dataCenter, config);
    std::string coreName = "core0.veccore0";
    SimData data;
    ASSERT_FALSE(core.ParseByCore(coreName, data));
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | CollectInstrEvents
* |  用例名  | test_CollectInstrEvents_collect_json_success_of_310P_set_wait_common_instr
* | 用例描述 | 测试310P芯片set、wait及普通指令生成正确json数据
*/
TEST(CoreTimeLineVisualizer, test_CollectInstrEvents_collect_json_success_of_310P_set_wait_common_instr)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    ChipProductType chipType = ChipProductType::ASCEND310P1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    CoreTimeLineVisualizer core(dataCenter, config);
    std::vector<nlohmann::json> coreJsonList;
    MergeInfo m1;
    m1.icacheTick = UINT64_MAX;
    m1.pc = 0x10cfa000;
    m1.startTick = 782;
    m1.endTick = 788;
    m1.pipe = "SCALAR";
    m1.name = "scalar_mov_xd_imme16";
    m1.detail = "x[0]=0x0,imme16:0x1";
    MergeInfo m2;
    m2.icacheTick = UINT64_MAX;
    m2.pc = 0x10cfa008;
    m2.startTick = 779;
    m2.endTick = 1112;
    m2.pipe = "SCALAR";
    m2.name = "wait_event";
    m2.detail = "pipe_type: L2,tigger_pipe: SCALAR,event_id: 0";
    MergeInfo m3;
    m3.icacheTick = UINT64_MAX;
    m3.pc = 0x10cfa012;
    m3.startTick = 787;
    m3.endTick = 1112;
    m3.pipe = "MTE2";
    m3.name = "set_event";
    m3.detail = "pipe_type: L2,tigger_pipe: SCALAR,event_id: 0";
    std::vector<MergeInfo> mergeVec {m1, m2, m3};
    core.CollectInstrEvents("core0", mergeVec, coreJsonList);

    nlohmann::json scalar, waitBegin, waitEnd, setBegin, setEnd, flows, flowt;
    for (const auto &i: coreJsonList) {
        if (i.at("name") == "scalar_mov_xd_imme16") { scalar = i; }
        if (i.at("name") == "wait_event" && i.at("ph") == "B")
        { waitBegin = i; }
        if (i.at("name") == "wait_event" && i.at("ph") == "E" && i.at("tid") == "SCALAR") { waitEnd = i; }
        if (i.at("name") == "set_event" && i.at("ph") == "B")
        { setBegin = i; }
        if (i.at("name") == "set_event" && i.at("ph") == "E" && i.at("tid") == "MTE2") { setEnd = i; }
        if (i.at("name") == "flow" && i.at("ph") == "s" && i.at("cat") == "MTE2ToSCALAR") { flows = i; }
        if (i.at("name") == "flow" && i.at("ph") == "t" && i.at("cat") == "MTE2ToSCALAR") { flowt = i; }
    }
    ASSERT_EQ(coreJsonList.size(), 7);
    EXPECT_FLOAT_EQ(scalar.at("ts"), GetMicrosecond(chipType, 782, -1));
    EXPECT_FLOAT_EQ(scalar.at("dur"), GetMicrosecond(chipType, 6, -1));
    // test wait flag display optimization when overlapping, its start will be end of common instr
    EXPECT_FLOAT_EQ(waitBegin.at("ts"), GetMicrosecond(chipType, 788, -1));
    auto waitEndTime = GetMicrosecond(chipType, 1112, -1);
    EXPECT_FLOAT_EQ(waitEnd.at("ts"), waitEndTime);
    EXPECT_FLOAT_EQ(setBegin.at("ts"), GetMicrosecond(chipType, 1112 - 1, -1));
    auto setEndTime = GetMicrosecond(chipType, 1112, -1);
    EXPECT_FLOAT_EQ(setEnd.at("ts"), setEndTime);
    // flow start is end of set flag, flow end is end of wait flag
    EXPECT_FLOAT_EQ(flows.at("ts"), setEndTime);
    EXPECT_FLOAT_EQ(flowt.at("ts"), waitEndTime);
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | AddFlag
* |  用例名  | test_AddFlag_add_set_instr_begin_and_end_json
* | 用例描述 | 测试添加set指令生成正确的起、止json数据
*/
TEST(CoreTimeLineVisualizer, test_AddFlag_add_set_instr_begin_and_end_json) {
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    ChipProductType chipType = ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    CoreTimeLineVisualizer core(dataCenter, config);
    std::vector<nlohmann::json> coreJsonList;
    MergeInfo m1;
    m1.pc = 0x12345678;
    m1.startTick = 782;
    m1.endTick = 788;
    m1.pipe = "MTE1";
    m1.name = "SET_FLAG";
    m1.detail = "PIPE:MTE1,TRIGGERPIPE:MTE3,FLAGID:0,";
    EventArgs evtArgs;
    SetWaitFlag flag {m1, evtArgs, "core0.cube0"};
    core.AddFlag(flag, "core0_0", coreJsonList);

    ASSERT_EQ(coreJsonList.size(), 2);
    nlohmann::json setBegin, setEnd;
    for (const auto &i: coreJsonList) {
        if (i.at("name") == "SET_FLAG" && i.at("ph") == "B") { setBegin = i; }
        if (i.at("name") == "SET_FLAG" && i.at("ph") == "E") { setEnd = i; }
    }
    EXPECT_FLOAT_EQ(setBegin.at("ts"), GetMicrosecond(chipType, 788 - 1, -1));
    EXPECT_FLOAT_EQ(setEnd.at("ts"), GetMicrosecond(chipType, 788, -1));
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | CollectUserMarkEvents
* |  用例名  | test_CollectUserMarkEvents_add_usermark_json_success
* | 用例描述 | 测试添加usermark指令生成正确的json数据
*/
TEST(CoreTimeLineVisualizer, test_CollectUserMarkEvents_add_usermark_json_success) {
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    ChipProductType chipType = ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    CoreTimeLineVisualizer core(dataCenter, config);
    std::vector<nlohmann::json> coreJsonList;
    core.CollectUserMarkEvents("core0.veccore0", simData->at("core0.veccore0"), coreJsonList);
    ASSERT_EQ(coreJsonList.size(), 1);
    ASSERT_EQ(coreJsonList[0].at("name"), "Mark 0x1");
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | CollectUserMarkEvents
* |  用例名  | test_CollectUserMarkEvents_add_none_json_when_usermark_nullptr
* | 用例描述 | 测试添加usermark指令时simData中usermarkPtr为nullptr，结果json为空
*/
TEST(CoreTimeLineVisualizer, test_CollectUserMarkEvents_add_none_json_when_usermark_nullptr) {
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    ChipProductType chipType = ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    CoreTimeLineVisualizer core(dataCenter, config);
    std::vector<nlohmann::json> coreJsonList;
    SimData data;
    core.CollectUserMarkEvents("core0.cube0", data, coreJsonList);
    ASSERT_EQ(coreJsonList.size(), 0);
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | CollectUserMarkEvents
* |  用例名  | test_CollectUserMarkEvents_add_none_json_when_usermark_id_error
* | 用例描述 | 测试添加usermark指令时simData中usermarkPtr为nullptr，结果json为空
*/
TEST(CoreTimeLineVisualizer, test_CollectUserMarkEvents_add_none_json_when_usermark_id_error) {
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    ChipProductType chipType = ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    CoreTimeLineVisualizer core(dataCenter, config);
    std::vector<nlohmann::json> coreJsonList;

    MergeInfo userMark;
    userMark.pc = 0x10f86010;
    userMark.startTick = 11040;
    userMark.endTick = 11042;
    userMark.name = "Mark 0x1";
    userMark.pipe = "USERMARK";
    std::vector<MergeInfo> userMarkVec {userMark};
    std::map<std::string, std::vector<UserMarkInfo>> userMarkInfos = {};
    UserMarkStruct userMarkStruct = {userMarkInfos, userMarkVec};
    std::shared_ptr<UserMarkStruct> userMarkPtr;
    userMarkPtr = MakeShared<UserMarkStruct>(userMarkStruct);
    SimData data = {nullptr, nullptr, userMarkPtr};
    core.CollectUserMarkEvents("core0.cube0", data, coreJsonList);
    ASSERT_EQ(coreJsonList.size(), 0);
}
}
