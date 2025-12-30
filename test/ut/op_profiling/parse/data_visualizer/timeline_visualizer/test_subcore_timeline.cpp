#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"
#define private public
#include "parse/data_visualizer/timeline_visualizer/subcore_timeline/subcore_timeline_visualizer.h"
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
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | Entry
* |  用例名  | test_SubcoreTimelineVisualizer_910B_should_return_ture_when_parse_ok
* | 用例描述 | SubcoreTimelineVisualizer，检查解析正常的全部功能
*/
TEST(SubcoreTimelineVisualizer, test_SubcoreTimelineVisualizer_910B_should_return_ture_when_parse_ok)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    const std::string fileName = "test/ut/resources/dump/output/core0.veccore0/trace.json";
    SimVisualizerConfig config = GetVisualizeConfig(output, Common::ChipProductType::ASCEND910B1);
    const std::string sepCoreOutput = JoinPath({output, "core0.veccore0"});
    MkdirRecusively(sepCoreOutput);
    SubcoreTimelineVisualizer core(dataCenter, config);
    ASSERT_EQ(core.Entry(), PluginErrorCode::SUCCESS);
    ASSERT_TRUE(IsExist(fileName));
    std::experimental::filesystem::remove_all(output);
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | Entry
* |  用例名  | test_Entry_should_return_error_when_db_not_register
* | 用例描述 | 测试DB未注册时Entry返回error
*/
TEST(SubcoreTimelineVisualizer, test_Entry_should_return_error_when_db_not_register)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, Common::ChipProductType::ASCEND910B1);
    SubcoreTimelineVisualizer core(dataCenter, config);
    ASSERT_EQ(core.Entry(), PluginErrorCode::NONBLOCKING_ERROR);
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | ParseByCore
* |  用例名  | test_ParseByCore_should_return_ture_when_parse_success
* | 用例描述 | 测试解析分核仿真数据json生成正确
*/
TEST(SubcoreTimelineVisualizer, test_ParseByCore_should_return_ture_when_parse_success)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, Common::ChipProductType::ASCEND910B1);
    MOCKER(&SubcoreTimelineVisualizer::WriteSepJson).stubs().will(returnValue(true));
    SubcoreTimelineVisualizer core(dataCenter, config);
    std::string coreName = "core0.veccore0";
    ASSERT_TRUE(core.ParseByCore(coreName, simData->at(coreName)));
    GlobalMockObject::verify();
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | ParseByCore
* |  用例名  | test_ParseByCore_should_return_false_when_InstrDetailTable_nullptr
* | 用例描述 | 测试当InstrDetailTable未初始化时解析分核仿真数据失败
*/
TEST(SubcoreTimelineVisualizer, test_ParseByCore_should_return_false_when_InstrDetailTable_nullptr)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, Common::ChipProductType::ASCEND910B1);
    SubcoreTimelineVisualizer core(dataCenter, config);
    std::string coreName = "core0.veccore0";
    SimData data;
    ASSERT_FALSE(core.ParseByCore(coreName, data));
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | ParseByCore
* |  用例名  | test_ParseByCore_should_return_false_when_write_sep_json_failed
* | 用例描述 | 测试当写入json失败时解析分核仿真数据失败
*/
TEST(SubcoreTimelineVisualizer, test_ParseByCore_should_return_false_when_write_sep_json_failed)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, Common::ChipProductType::ASCEND910B1);
    MOCKER(&SubcoreTimelineVisualizer::WriteSepJson).stubs().will(returnValue(false));
    SubcoreTimelineVisualizer core(dataCenter, config);
    std::string coreName = "core0.veccore0";
    ASSERT_FALSE(core.ParseByCore(coreName, simData->at(coreName)));
    GlobalMockObject::verify();
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | CollectInstrEvents4SepTrace
* |  用例名  | test_CollectInstrEvents4SepTrace_collect_json_success_of_310P_set_wait_common_instr
* | 用例描述 | 测试310P芯片set、wait及普通指令生成正确json数据
*/
TEST(SubcoreTimelineVisualizer, test_CollectInstrEvents4SepTrace_collect_json_success_of_310P_set_wait_common_instr)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    Common::ChipProductType chipType = Common::ChipProductType::ASCEND310P1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    MergeInfo m1;
    m1.pc = 0x10cfa000;
    m1.startTick = 782;
    m1.endTick = 788;
    m1.pipe = "SCALAR";
    m1.name = "scalar_mov_xd_imme16";
    m1.detail = "x[0]=0x0,imme16:0x1";
    m1.warpId = DEFAULT_INT_VALUE;
    m1.schId = DEFAULT_INT_VALUE;
    MergeInfo m2;
    m2.pc = 0x10cfa008;
    m2.startTick = 779;
    m2.endTick = 1112;
    m2.pipe = "SCALAR";
    m2.name = "wait_event";
    m2.detail = "pipe_type: L2,tigger_pipe: SCALAR,event_id: 0";
    m2.warpId = DEFAULT_INT_VALUE;
    m2.schId = DEFAULT_INT_VALUE;
    MergeInfo m3;
    m3.pc = 0x10cfa012;
    m3.startTick = 787;
    m3.endTick = 1112;
    m3.pipe = "MTE2";
    m3.name = "set_event";
    m3.detail = "pipe_type: L2,tigger_pipe: SCALAR,event_id: 0";
    m3.warpId = DEFAULT_INT_VALUE;
    m3.schId = DEFAULT_INT_VALUE;
    SubcoreTimelineVisualizer core(dataCenter, config);
    std::vector<MergeInfo> mergeVec {m1, m2, m3};
    std::vector<nlohmann::json> coreJsonList;
    std::set<std::string> pipeSet;
    core.CollectInstrEvents4SepTrace(mergeVec, coreJsonList, pipeSet);

    nlohmann::json scalar, wait, set, flows, flowt;
    for (const auto &i: coreJsonList) {
        if (i.at("name") == "scalar_mov_xd_imme16" && i.at("args").at("pc_addr") == "0x10cfa000") { scalar = i; }
        if (i.at("name") == "wait_event") { wait = i; }
        if (i.at("name") == "set_event") { set = i; }
        if (i.at("name") == "flow" && i.at("ph") == "s") { flows = i; }
        if (i.at("name") == "flow" && i.at("ph") == "t") { flowt = i; }
    }
    ASSERT_EQ(coreJsonList.size(), 15);
    EXPECT_FLOAT_EQ(scalar.at("ts"), GetMicrosecond(chipType, 782));
    EXPECT_FLOAT_EQ(scalar.at("dur"), GetMicrosecond(chipType, 6));
    // test wait flag display optimization when overlapping, its start will be end of common instr
    EXPECT_FLOAT_EQ(wait.at("ts"), GetMicrosecond(chipType, 788, -1));
    EXPECT_FLOAT_EQ(wait.at("dur"), GetMicrosecond(chipType, 1112 - 788, -1));
    auto set1StartTime = GetMicrosecond(chipType, 1112 - 1, -1);
    auto set1Dur = GetMicrosecond(chipType, 1, -1);
    EXPECT_FLOAT_EQ(set.at("ts"), set1StartTime);
    EXPECT_FLOAT_EQ(set.at("dur"), set1Dur);
    // flow start is medium of set flag, flow end is end of wait flag
    EXPECT_FLOAT_EQ(flows.at("ts"), set1StartTime + set1Dur/2.0);
    EXPECT_FLOAT_EQ(flowt.at("ts"), GetMicrosecond(chipType, 1112, -1));
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | CollectEvents
* |  用例名  | test_CollectEvents_collect_json_success_of_910B_set_wait_bar_instr
* | 用例描述 | 测试910B芯片set、wait及bar指令生成正确json数据
*/
TEST(SubcoreTimelineVisualizer, test_CollectEvents_collect_json_success_of_910B_set_wait_bar_instr)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    Common::ChipProductType chipType = Common::ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    MergeInfo m1;
    m1.pc = 0x10cfa000;
    m1.startTick = 782;
    m1.endTick = 788;
    m1.pipe = "SCALAR";
    m1.name = "BAR";
    m1.detail = "THIS IS A NORMAL SCALAR INSTRUCTION,";
    MergeInfo m2;
    m2.pc = 0x10cfa008;
    m2.startTick = 779;
    m2.endTick = 1112;
    m2.pipe = "MTE3";
    m2.name = "WAIT_FLAG";
    m2.detail = "PIPE:MTE1,TRIGGERPIPE:MTE3,FLAGID:0,";
    MergeInfo m3;
    m3.pc = 0x10cfa012;
    m3.startTick = 787;
    m3.endTick = 1112;
    m3.pipe = "MTE1";
    m3.name = "SET_FLAG";
    m3.detail = "PIPE:MTE1,TRIGGERPIPE:MTE3,FLAGID:0,";
    SubcoreTimelineVisualizer core(dataCenter, config);
    std::vector<MergeInfo> mergeVec {m1, m2, m3};
    std::map<int, std::vector<MergeInfo>> instrsGroup = {{0, mergeVec}};
    std::vector<nlohmann::json> coreJsonList;
    std::map<std::string, std::vector<XEvent>> setFlagRecord;
    std::map<std::string, std::vector<XEvent>> waitFlagRecord;

    core.CollectEvents(instrsGroup, coreJsonList, setFlagRecord, waitFlagRecord);
    ASSERT_EQ(coreJsonList.size(), 9);
    ASSERT_EQ(setFlagRecord.at(m3.detail).size(), 1);
    ASSERT_EQ(waitFlagRecord.at(m2.detail).size(), 1);
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | GetUserMarkTid
* |  用例名  | test_GetUserMarkTid_add_new_thread_metadata
* | 用例描述 | 测试获取新的tid并添加metadata json数据
*/
TEST(SubcoreTimelineVisualizer, test_GetUserMarkTid_add_new_thread_metadata)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    Common::ChipProductType chipType = Common::ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    SubcoreTimelineVisualizer core(dataCenter, config);
    MergeInfo instr;
    instr.name = "Mark 0x1";
    std::map<std::string, bool> userMarkTidMap;
    std::vector<nlohmann::json> coreJsonList;
    ASSERT_EQ(core.GetUserMarkTid(instr, userMarkTidMap, coreJsonList), 1);
    ASSERT_EQ(coreJsonList.size(), 2);
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | GetUserMarkTid
* |  用例名  | test_GetUserMarkTid_get_existed_tid
* | 用例描述 | 测试获取已有tid
*/
TEST(SubcoreTimelineVisualizer, test_GetUserMarkTid_get_existed_tid)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    Common::ChipProductType chipType = Common::ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    SubcoreTimelineVisualizer core(dataCenter, config);
    MergeInfo instr;
    instr.name = "Mark 0x1";
    std::map<std::string, bool> userMarkTidMap = {{instr.name, true}};
    std::vector<nlohmann::json> coreJsonList;
    ASSERT_EQ(core.GetUserMarkTid(instr, userMarkTidMap, coreJsonList), 1);
    ASSERT_EQ(coreJsonList.size(), 0);
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | CollectUserMarkEvents
* |  用例名  | test_CollectUserMarkEvents_add_usermark_json_success
* | 用例描述 | 测试添加usermark指令生成正确的json数据
*/
TEST(SubcoreTimelineVisualizer, test_CollectUserMarkEvents_add_usermark_json_success) {
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    Common::ChipProductType chipType = Common::ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    MOCKER(&SubcoreTimelineVisualizer::GetUserMarkTid).stubs().will(returnValue(1));
    SubcoreTimelineVisualizer core(dataCenter, config);
    std::vector<nlohmann::json> coreJsonList;
    core.CollectUserMarkEvents(simData->at("core0.veccore0"), coreJsonList);
    ASSERT_EQ(coreJsonList.size(), 3);
    nlohmann::json mark;
    for (const auto &i: coreJsonList) {
        if (i.at("name") == "Mark 0x1") { mark = i; }
    }
    ASSERT_EQ(mark.at("tid"), 1);
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | CollectUserMarkEvents
* |  用例名  | test_CollectUserMarkEvents_add_none_json_when_usermark_nullptr
* | 用例描述 | 测试添加usermark指令时simData中usermarkPtr为nullptr，结果json为空
*/
TEST(SubcoreTimelineVisualizer, test_CollectUserMarkEvents_add_none_json_when_usermark_nullptr) {
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    Common::ChipProductType chipType = Common::ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    SubcoreTimelineVisualizer core(dataCenter, config);
    std::vector<nlohmann::json> coreJsonList;
    SimData data;
    core.CollectUserMarkEvents(data, coreJsonList);
    ASSERT_EQ(coreJsonList.size(), 0);
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | CollectUserMarkEvents
* |  用例名  | test_CollectUserMarkEvents_add_none_json_when_usermark_id_error
* | 用例描述 | 测试添加usermark指令时simData中usermarkPtr为nullptr，结果json为空
*/
TEST(SubcoreTimelineVisualizer, test_CollectUserMarkEvents_add_none_json_when_usermark_id_error) {
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    Common::ChipProductType chipType = Common::ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    SubcoreTimelineVisualizer core(dataCenter, config);
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
    core.CollectUserMarkEvents(data, coreJsonList);
    ASSERT_EQ(coreJsonList.size(), 2);
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | GetTid
* |  用例名  | test_GetTid_add_new_thread
* | 用例描述 | 测试获取tid时分配新的线程
*/
TEST(SubcoreTimelineVisualizer, test_GetTid_add_new_thread)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    Common::ChipProductType chipType = Common::ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    MOCKER(&SubcoreTimelineVisualizer::FindAvailableThread).stubs().will(returnValue(false));
    SubcoreTimelineVisualizer core(dataCenter, config);
    MergeInfo instr;
    std::vector<std::vector<int>> pipeOccupy;
    std::vector<nlohmann::json> coreJsonList;
    uint64_t maxCycle = 0;
    ASSERT_EQ(core.GetTid(instr, maxCycle, pipeOccupy, coreJsonList, core.pidMap_["VECTOR"]), 1);
    ASSERT_EQ(coreJsonList.size(), 2);
    ASSERT_EQ(pipeOccupy.size(), 1);
    GlobalMockObject::verify();
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | GetTid
* |  用例名  | test_GetTid_find_available_thread
* | 用例描述 | 测试获取tid时使用可用线程
*/
TEST(SubcoreTimelineVisualizer, test_GetTid_find_available_thread)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    Common::ChipProductType chipType = Common::ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    MOCKER(&SubcoreTimelineVisualizer::FindAvailableThread).stubs().will(returnValue(true));
    SubcoreTimelineVisualizer core(dataCenter, config);
    MergeInfo instr;
    std::vector<std::vector<int>> pipeOccupy;
    std::vector<nlohmann::json> coreJsonList;
    uint64_t maxCycle = 0;
    ASSERT_EQ(core.GetTid(instr, maxCycle, pipeOccupy, coreJsonList, core.pidMap_["VECTOR"]), 0);
    ASSERT_EQ(coreJsonList.size(), 0);
    ASSERT_EQ(pipeOccupy.size(), 0);
    GlobalMockObject::verify();
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | FindAvailableThread
* |  用例名  | test_FindAvailableThread_should_return_true_when_find_thread_success
* | 用例描述 | 测试正确获取可用线程
*/
TEST(SubcoreTimelineVisualizer, test_FindAvailableThread_should_return_true_when_find_thread_success)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    Common::ChipProductType chipType = Common::ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    SubcoreTimelineVisualizer core(dataCenter, config);
    int curThreadNum = 1;
    MergeInfo instr;
    instr.startTick = 0;
    instr.endTick = 100;
    std::vector<std::vector<int>> pipeOccupy = {{}};
    pipeOccupy.resize(1);
    pipeOccupy[0].resize(100000);
    int tid = 0;
    ASSERT_TRUE(core.FindAvailableThread(curThreadNum, instr, pipeOccupy, tid));
    ASSERT_EQ(tid, 1);
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | FindAvailableThread
* |  用例名  | test_FindAvailableThread_should_return_false_when_current_thread_num_is_zero
* | 用例描述 | 测试当前线程数为0时获取可用线程失败
*/
TEST(SubcoreTimelineVisualizer, test_FindAvailableThread_should_return_false_when_current_thread_num_is_zero)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    Common::ChipProductType chipType = Common::ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    SubcoreTimelineVisualizer core(dataCenter, config);
    int curThreadNum = 0;
    MergeInfo instr;
    std::vector<std::vector<int>> pipeOccupy;
    int tid = 0;
    ASSERT_FALSE(core.FindAvailableThread(curThreadNum, instr, pipeOccupy, tid));
    ASSERT_EQ(tid, 0);
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | AddProcessMetaData
* |  用例名  | test_AddProcessMetaData_add_json_success
* | 用例描述 | 测试正确添加process metaData json数据
*/
TEST(SubcoreTimelineVisualizer, test_AddProcessMetaData_add_json_success)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    Common::ChipProductType chipType = Common::ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    SubcoreTimelineVisualizer core(dataCenter, config);
    std::vector<nlohmann::json> coreJsonList;
    core.AddProcessMetaData(coreJsonList, "MTE2", core.pidMap_["MTE2"]);
    ASSERT_EQ(coreJsonList.size(), 2);
    nlohmann::json processName, processIndex;
    for (const auto &i: coreJsonList) {
        if (i.at("name") == "process_name") { processName = i; }
        if (i.at("name") == "process_sort_index") { processIndex = i; }
    }
    ASSERT_EQ(processName.at("pid"), 60);
    ASSERT_EQ(processName.at("args").at("name"), "MTE2");
    ASSERT_EQ(processIndex.at("args").at("sort_index"), 60);
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | AddThreadMetaData
* |  用例名  | test_AddThreadMetaData_add_json_success
* | 用例描述 | 测试正确添加thread metaData json数据
*/
TEST(SubcoreTimelineVisualizer, test_AddThreadMetaData_add_json_success)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    Common::ChipProductType chipType = Common::ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    SubcoreTimelineVisualizer core(dataCenter, config);
    std::vector<nlohmann::json> coreJsonList;
    core.AddThreadMetaData(coreJsonList, 0, "MTE2_0", core.pidMap_["MTE2"]);
    ASSERT_EQ(coreJsonList.size(), 2);
    nlohmann::json threadName, threadIndex;
    for (const auto &i: coreJsonList) {
        if (i.at("name") == "thread_name") { threadName = i; }
        if (i.at("name") == "thread_sort_index") { threadIndex = i; }
    }
    ASSERT_EQ(threadName.at("pid"), 60);
    ASSERT_EQ(threadName.at("args").at("name"), "MTE2_0");
    ASSERT_EQ(threadIndex.at("args").at("sort_index"), 0);
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | WriteSepJson
* |  用例名  | test_WriteSepJson_should_return_ture_when_write_json_success
* | 用例描述 | 测试写入json成功
*/
TEST(SubcoreTimelineVisualizer, test_WriteSepJson_should_return_ture_when_write_json_success)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    const std::string fileName = "test/ut/resources/dump/output/core0.veccore0/trace.json";
    SimVisualizerConfig config = GetVisualizeConfig(output, Common::ChipProductType::ASCEND910B1);
    const std::string sepCoreOutput = JoinPath({output, "core0.veccore0"});
    MkdirRecusively(sepCoreOutput);
    SubcoreTimelineVisualizer core(dataCenter, config);
    std::vector<nlohmann::json> coreJsonList;
    ASSERT_TRUE(core.WriteSepJson(fileName, coreJsonList));
    ASSERT_TRUE(IsExist(fileName));
    std::experimental::filesystem::remove_all(output);
}

/**
* |  用例集 | SubcoreTimelineVisualizer
* | 测试函数 | WriteSepJson
* |  用例名  | test_WriteSepJson_should_return_false_when_write_json_failed
* | 用例描述 | 测试写入json失败
*/
TEST(SubcoreTimelineVisualizer, test_WriteSepJson_should_return_false_when_write_json_failed)
{
    DataCenter dataCenter;
    auto simData = GetSimData();
    dataCenter.DataTableRegister(simData);
    std::string output = "test/ut/resources/dump/output";
    const std::string fileName = "test/ut/resources/dump/output/core0.veccore0/trace.json";
    SimVisualizerConfig config = GetVisualizeConfig(output, Common::ChipProductType::ASCEND910B1);
    const std::string sepCoreOutput = JoinPath({output, "core0.veccore0"});
    SubcoreTimelineVisualizer core(dataCenter, config);
    std::vector<nlohmann::json> coreJsonList;
    ASSERT_FALSE(core.WriteSepJson(fileName, coreJsonList));
    ASSERT_FALSE(IsExist(fileName));
}
}