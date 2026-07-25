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
* | 测试函数 | CollectInstrEvents
* |  用例名  | test_CollectInstrEvents_should_record_intra_block_instrs
* | 用例描述 | 测试CollectInstrEvents在解析时记录intra block指令到对应map
*/
TEST(CoreTimeLineVisualizer, test_CollectInstrEvents_should_record_intra_block_instrs)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    CoreTimeLineVisualizer core(dataCenter, config);

    MergeInfo setIntra;
    setIntra.icacheTick = UINT64_MAX;
    setIntra.pc = 0x10f86000;
    setIntra.startTick = 100;
    setIntra.endTick = 200;
    setIntra.pipe = "CUBE";
    setIntra.name = "SET_INTRA_BLOCK";
    setIntra.detail = "PIPE:CUBE,sync_id:5,";

    MergeInfo waitIntra;
    waitIntra.icacheTick = UINT64_MAX;
    waitIntra.pc = 0x10f86000;
    waitIntra.startTick = 200;
    waitIntra.endTick = 300;
    waitIntra.pipe = "VECTOR";
    waitIntra.name = "WAIT_INTRA_BLOCK";
    waitIntra.detail = "PIPE:VECTOR,sync_id:5,";

    MergeInfo normalInstr;
    normalInstr.icacheTick = UINT64_MAX;
    normalInstr.pc = 0x10f86000;
    normalInstr.startTick = 100;
    normalInstr.endTick = 110;
    normalInstr.pipe = "SCALAR";
    normalInstr.name = "scalar_mov_xd_imme16";
    normalInstr.detail = "x[0]=0x0";

    std::vector<MergeInfo> mergeVec {setIntra, waitIntra, normalInstr};
    std::vector<nlohmann::json> coreJson;
    core.CollectInstrEvents("core0.cubecore0", mergeVec, coreJson);

    ASSERT_EQ(core.recordIntraSetFlag_.size(), 1);
    ASSERT_EQ(core.recordIntraWaitFlag_.size(), 1);
    auto setInstrsSize = core.recordIntraSetFlag_[{"core0", "cubecore0"}][{"SET_INTRA_BLOCK", 5}].size();
    auto waitInstrSize = core.recordIntraWaitFlag_[{"core0", "cubecore0"}][{"WAIT_INTRA_BLOCK", 5}].size();
    ASSERT_EQ(setInstrsSize, 1);
    ASSERT_EQ(waitInstrSize, 1);
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

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | IsIntraBlockInstr
* |  用例名  | test_IsIntraBlockInstr_should_return_true_when_intra_block_instr
* | 用例描述 | 测试IsIntraBlockInstr对4种intra block指令返回true
*/
TEST(CoreTimeLineVisualizer, test_IsIntraBlockInstr_should_return_true_when_intra_block_instr)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    CoreTimeLineVisualizer core(dataCenter, config);
    ASSERT_TRUE(core.IsIntraBlockInstr("SET_INTRA_BLOCK"));
    ASSERT_TRUE(core.IsIntraBlockInstr("SET_INTRA_BLOCKI"));
    ASSERT_TRUE(core.IsIntraBlockInstr("WAIT_INTRA_BLOCK"));
    ASSERT_TRUE(core.IsIntraBlockInstr("WAIT_INTRA_BLOCKI"));
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | IsIntraBlockInstr
* |  用例名  | test_IsIntraBlockInstr_should_return_false_when_not_intra_block_instr
* | 用例描述 | 测试IsIntraBlockInstr对非intra block指令返回false
*/
TEST(CoreTimeLineVisualizer, test_IsIntraBlockInstr_should_return_false_when_not_intra_block_instr)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    CoreTimeLineVisualizer core(dataCenter, config);
    ASSERT_FALSE(core.IsIntraBlockInstr("SET_FLAG"));
    ASSERT_FALSE(core.IsIntraBlockInstr("WAIT_FLAG"));
    ASSERT_FALSE(core.IsIntraBlockInstr("scalar_mov_xd_imme16"));
    ASSERT_FALSE(core.IsIntraBlockInstr(""));
}

/**
 * |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | SplitCoreName
* |  用例名  | test_SplitCoreName_should_return_coreId_and_subcore_when_has_dot
* | 用例描述 | 测试SplitCoreName正确拆分含点号的coreName
*/
TEST(CoreTimeLineVisualizer, test_SplitCoreName_should_return_coreId_and_subcore_when_has_dot)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    CoreTimeLineVisualizer core(dataCenter, config);
    auto [coreId, subcore] = core.SplitCoreName("core0.veccore0");
    ASSERT_EQ(coreId, "core0");
    ASSERT_EQ(subcore, "veccore0");

    auto [coreId2, subcore2] = core.SplitCoreName("core1.cubecore0");
    ASSERT_EQ(coreId2, "core1");
    ASSERT_EQ(subcore2, "cubecore0");
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | SplitCoreName
* |  用例名  | test_SplitCoreName_should_return_coreId_and_empty_when_no_dot
* | 用例描述 | 测试SplitCoreName在无点号时返回coreId和空subcore
*/
TEST(CoreTimeLineVisualizer, test_SplitCoreName_should_return_coreId_and_empty_when_no_dot)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    CoreTimeLineVisualizer core(dataCenter, config);
    auto [coreId, subcore] = core.SplitCoreName("core0");
    ASSERT_EQ(coreId, "core0");
    ASSERT_EQ(subcore, "");
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | AddIntraBlockFlow
* |  用例名  | test_AddIntraBlockFlow_should_add_flow_events_to_coresJsonList
* | 用例描述 | 测试AddIntraBlockFlow添加flow事件到coresJsonList
*/
TEST(CoreTimeLineVisualizer, test_AddIntraBlockFlow_should_add_flow_events_to_coresJsonList)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    ChipProductType chipType = ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    CoreTimeLineVisualizer core(dataCenter, config);

    MergeInfo setInstr;
    setInstr.pc = 0x1000;
    setInstr.startTick = 1000;
    setInstr.endTick = 1100;
    setInstr.pipe = "CUBE";
    setInstr.name = "SET_INTRA_BLOCK";
    setInstr.detail = "PIPE:CUBE,TRIGGERPIPE:VECTOR,sync_id:0,";
    setInstr.icacheTick = UINT64_MAX;

    MergeInfo waitInstr;
    waitInstr.pc = 0x2000;
    waitInstr.startTick = 1200;
    waitInstr.endTick = 1300;
    waitInstr.pipe = "VECTOR";
    waitInstr.name = "WAIT_INTRA_BLOCK";
    waitInstr.detail = "PIPE:VECTOR,TRIGGERPIPE:CUBE,sync_id:0,";
    waitInstr.icacheTick = UINT64_MAX;

    size_t prevSize = core.coresJsonList_.size();
    core.AddIntraBlockFlow(setInstr, waitInstr, "core0.cubecore0", "core0.veccore0", 0);
    ASSERT_EQ(core.coresJsonList_.size(), prevSize + 6);

    auto &flagSetBegin = core.coresJsonList_[prevSize];
    auto &flagSetEnd = core.coresJsonList_[prevSize + 1];
    ASSERT_EQ(flagSetBegin.at("name"), "SET_INTRA_BLOCK");
    ASSERT_EQ(flagSetBegin.at("ph"), "B");
    ASSERT_EQ(flagSetEnd.at("name"), "SET_INTRA_BLOCK");
    ASSERT_EQ(flagSetEnd.at("ph"), "E");
    auto &flagWaitBegin = core.coresJsonList_[prevSize + 2];
    auto &flagWaitEnd = core.coresJsonList_[prevSize + 3];
    ASSERT_EQ(flagWaitBegin.at("name"), "WAIT_INTRA_BLOCK");
    ASSERT_EQ(flagWaitBegin.at("ph"), "B");
    ASSERT_EQ(flagWaitEnd.at("name"), "WAIT_INTRA_BLOCK");
    ASSERT_EQ(flagWaitEnd.at("ph"), "E");
    auto &flowBegin = core.coresJsonList_[prevSize + 4];
    auto &flowEnd = core.coresJsonList_[prevSize + 5];
    ASSERT_EQ(flowBegin.at("name"), "flow");
    ASSERT_EQ(flowBegin.at("ph"), "s");
    ASSERT_EQ(flowEnd.at("name"), "flow");
    ASSERT_EQ(flowEnd.at("ph"), "t");
    ASSERT_EQ(flowBegin.at("id"), "intra_0");
    ASSERT_EQ(flowEnd.at("id"), "intra_0");
    ASSERT_EQ(flowBegin.at("cat"), "CUBEToVECTOR");
    ASSERT_EQ(flowEnd.at("cat"), "CUBEToVECTOR");
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | GetIntraMatchCoreInfo
* |  用例名  | test_GetIntraMatchCoreInfo_should_match_cubecore0_small_sync_to_veccore0
* | 用例描述 | 测试GetIntraMatchCoreInfo: cubecore上syncId < 16的SET匹配到veccore0的WAIT
*/
TEST(CoreTimeLineVisualizer, test_GetIntraMatchCoreInfo_should_match_cubecore0_small_sync_to_veccore0)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    CoreTimeLineVisualizer core(dataCenter, config);

    MergeInfo waitInstr;
    waitInstr.pc = 0x2000;
    waitInstr.startTick = 1200;
    waitInstr.endTick = 1300;
    waitInstr.pipe = "VECTOR";
    waitInstr.name = "WAIT_INTRA_BLOCK";
    waitInstr.detail = "PIPE:VECTOR,sync_id:5,";
    core.recordIntraWaitFlag_[{"core0", "veccore0"}][{"WAIT_INTRA_BLOCK", 5}].emplace_back(
        std::pair<MergeInfo, bool>{waitInstr, false});

    IntraFlag flag = {"core0", "cubecore0", "SET_INTRA_BLOCK", "", 5, 0, nullptr};
    ASSERT_TRUE(core.GetIntraMatchCoreInfo(flag));
    ASSERT_EQ(flag.matchCoreName, "core0.veccore0");
    ASSERT_EQ(flag.intraFlag->first.name, "WAIT_INTRA_BLOCK");
    ASSERT_EQ(flag.intraFlag->first.pipe, "VECTOR");
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | GetIntraMatchCoreInfo
* |  用例名  | test_GetIntraMatchCoreInfo_should_match_cubecore0_large_sync_to_veccore1
* | 用例描述 | 测试GetIntraMatchCoreInfo: cubecore上syncId >= 16的SET匹配到veccore1的WAIT
*/
TEST(CoreTimeLineVisualizer, test_GetIntraMatchCoreInfo_should_match_cubecore0_large_sync_to_veccore1)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    CoreTimeLineVisualizer core(dataCenter, config);

    MergeInfo waitInstr;
    waitInstr.pc = 0x2000;
    waitInstr.startTick = 1200;
    waitInstr.endTick = 1300;
    waitInstr.pipe = "VECTOR";
    waitInstr.name = "WAIT_INTRA_BLOCK";
    waitInstr.detail = "PIPE:VECTOR,sync_id:0,";
    core.recordIntraWaitFlag_[{"core0", "veccore1"}][{"WAIT_INTRA_BLOCK", 0}].emplace_back(
        std::pair<MergeInfo, bool>{waitInstr, false});

    IntraFlag flag = {"core0", "cubecore0", "SET_INTRA_BLOCK", "", 16, 0, nullptr};
    ASSERT_TRUE(core.GetIntraMatchCoreInfo(flag));
    ASSERT_EQ(flag.matchCoreName, "core0.veccore1");
    ASSERT_EQ(flag.intraFlag->first.name, "WAIT_INTRA_BLOCK");
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | GetIntraMatchCoreInfo
* |  用例名  | test_GetIntraMatchCoreInfo_should_return_empty_when_no_match
* | 用例描述 | 测试GetIntraMatchCoreInfo在无匹配WAIT时返回空coreName
*/
TEST(CoreTimeLineVisualizer, test_GetIntraMatchCoreInfo_should_return_empty_when_no_match)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    CoreTimeLineVisualizer core(dataCenter, config);

    IntraFlag flag = {"core0", "cubecore0", "SET_INTRA_BLOCK", "", 5, 0, nullptr};
    ASSERT_FALSE(core.GetIntraMatchCoreInfo(flag));
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | GetIntraMatchCoreInfo
* |  用例名  | test_GetIntraMatchCoreInfo_should_match_SET_INTRA_BLOCKI_to_WAIT_INTRA_BLOCKI
* | 用例描述 | 测试GetIntraMatchCoreInfo: SET_INTRA_BLOCKI匹配WAIT_INTRA_BLOCKI
*/
TEST(CoreTimeLineVisualizer, test_GetIntraMatchCoreInfo_should_match_SET_INTRA_BLOCKI_to_WAIT_INTRA_BLOCKI)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    CoreTimeLineVisualizer core(dataCenter, config);

    MergeInfo waitInstr;
    waitInstr.pc = 0x3000;
    waitInstr.startTick = 2000;
    waitInstr.endTick = 2100;
    waitInstr.pipe = "VECTOR";
    waitInstr.name = "WAIT_INTRA_BLOCKI";
    waitInstr.detail = "PIPE:VECTOR,sync_id:3,";
    core.recordIntraWaitFlag_[{"core0", "veccore0"}][{"WAIT_INTRA_BLOCKI", 3}].emplace_back(
        std::pair<MergeInfo, bool>{waitInstr, false});

    IntraFlag flag = {"core0", "cubecore0", "SET_INTRA_BLOCKI", "", 3, 0, nullptr};
    ASSERT_TRUE(core.GetIntraMatchCoreInfo(flag));
    ASSERT_EQ(flag.matchCoreName, "core0.veccore0");
    ASSERT_EQ(flag.intraFlag->first.name, "WAIT_INTRA_BLOCKI");
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | CollectIntraBlockFlowEvents
* |  用例名  | test_CollectIntraBlockFlowEvents_should_generate_flow_events_when_set_and_wait_match
* | 用例描述 | 测试CollectIntraBlockFlowEvents在SET和WAIT匹配时生成flow事件
*/
TEST(CoreTimeLineVisualizer, test_CollectIntraBlockFlowEvents_should_generate_flow_events_when_set_and_wait_match)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    ChipProductType chipType = ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    CoreTimeLineVisualizer core(dataCenter, config);

    MergeInfo setInstr;
    setInstr.pc = 0x1000;
    setInstr.startTick = 100;
    setInstr.endTick = 200;
    setInstr.pipe = "CUBE";
    setInstr.name = "SET_INTRA_BLOCK";
    setInstr.detail = "PIPE:CUBE,TRIGGERPIPE:VECTOR,sync_id:0,";
    setInstr.icacheTick = UINT64_MAX;

    MergeInfo waitInstr;
    waitInstr.pc = 0x2000;
    waitInstr.startTick = 200;
    waitInstr.endTick = 300;
    waitInstr.pipe = "VECTOR";
    waitInstr.name = "WAIT_INTRA_BLOCK";
    waitInstr.detail = "PIPE:VECTOR,TRIGGERPIPE:CUBE,sync_id:0,";
    waitInstr.icacheTick = UINT64_MAX;

    core.recordIntraSetFlag_[{"core0", "cubecore0"}][{"SET_INTRA_BLOCK", 0}].emplace_back(
        std::pair<MergeInfo, bool>{setInstr, false});
    core.recordIntraWaitFlag_[{"core0", "veccore0"}][{"WAIT_INTRA_BLOCK", 0}].emplace_back(
        std::pair<MergeInfo, bool>{waitInstr, false});

    size_t prevSize = core.coresJsonList_.size();
    core.CollectIntraBlockFlowEvents();
    ASSERT_EQ(core.coresJsonList_.size(), prevSize + 6);

    auto &flowBegin = core.coresJsonList_[prevSize + 4];
    ASSERT_EQ(flowBegin.at("name"), "flow");
    ASSERT_EQ(flowBegin.at("ph"), "s");
    ASSERT_EQ(flowBegin.at("id"), "intra_0");
    ASSERT_EQ(flowBegin.at("cat"), "CUBEToVECTOR");
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | CollectIntraBlockFlowEvents
* |  用例名  | test_CollectIntraBlockFlowEvents_should_not_generate_when_only_set_no_wait
* | 用例描述 | 测试CollectIntraBlockFlowEvents在仅有SET无WAIT时不生成flow事件
*/
TEST(CoreTimeLineVisualizer, test_CollectIntraBlockFlowEvents_should_not_generate_when_only_set_no_wait)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    CoreTimeLineVisualizer core(dataCenter, config);

    MergeInfo setInstr;
    setInstr.pc = 0x1000;
    setInstr.startTick = 100;
    setInstr.endTick = 200;
    setInstr.pipe = "CUBE";
    setInstr.name = "SET_INTRA_BLOCK";
    setInstr.detail = "PIPE:CUBE,sync_id:0,";
    setInstr.icacheTick = UINT64_MAX;

    core.recordIntraSetFlag_[{"core0", "cubecore0"}][{"SET_INTRA_BLOCK", 0}].emplace_back(
        std::pair<MergeInfo, bool>{setInstr, false});

    size_t prevSize = core.coresJsonList_.size();
    core.CollectIntraBlockFlowEvents();
    ASSERT_EQ(core.coresJsonList_.size(), prevSize + 2);
}

/**
 * |  用例集 | CoreTimeLineVisualizer
 * | 测试函数 | CollectIntraBlockFlowEvents
 * |  用例名  | test_CollectIntraBlockFlowEvents_should_generate_multiple_flow_events
* | 用例描述 | 测试CollectIntraBlockFlowEvents生成多条flow事件并分配递增id
*/
TEST(CoreTimeLineVisualizer, test_CollectIntraBlockFlowEvents_should_generate_multiple_flow_events)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    ChipProductType chipType = ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    CoreTimeLineVisualizer core(dataCenter, config);

    for (int i = 0; i < 3; i++) {
        MergeInfo setInstr;
        setInstr.icacheTick = UINT64_MAX;
        setInstr.pc = static_cast<uint64_t>(0x1000) + i;
        setInstr.startTick = 100 + i * 10;
        setInstr.endTick = 200 + i * 10;
        setInstr.pipe = "CUBE";
        setInstr.name = "SET_INTRA_BLOCK";
        setInstr.detail = "PIPE:CUBE,sync_id:" + std::to_string(i) + ",";
        core.recordIntraSetFlag_[{"core0", "cubecore0"}][{"SET_INTRA_BLOCK", i}].emplace_back(
            std::pair<MergeInfo, bool>{setInstr, false});

        MergeInfo waitInstr;
        waitInstr.icacheTick = UINT64_MAX;
        waitInstr.pc = static_cast<uint64_t>(0x2000) + i;
        waitInstr.startTick = 200 + i * 10;
        waitInstr.endTick = 300 + i * 10;
        waitInstr.pipe = "VECTOR";
        waitInstr.name = "WAIT_INTRA_BLOCK";
        waitInstr.detail = "PIPE:VECTOR,sync_id:" + std::to_string(i) + ",";
        core.recordIntraWaitFlag_[{"core0", "veccore0"}][{"WAIT_INTRA_BLOCK", i}].emplace_back(
            std::pair<MergeInfo, bool>{waitInstr, false});
    }

    core.CollectIntraBlockFlowEvents();
    ASSERT_EQ(core.coresJsonList_.size(), 18);

    for (int i = 0; i < 3; i++) {
        ASSERT_EQ(core.coresJsonList_[i * 6].at("id"), "intra_" + std::to_string(i));
        ASSERT_EQ(core.coresJsonList_[i * 6 + 1].at("id"), "intra_" + std::to_string(i));
    }
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | CollectIntraBlockFlowEvents
* |  用例名  | test_CollectIntraBlockFlowEvents_should_match_large_sync_to_veccore1
* | 用例描述 | 测试CollectIntraBlockFlowEvents: syncId >= 16的SET匹配到veccore1
*/
TEST(CoreTimeLineVisualizer, test_CollectIntraBlockFlowEvents_should_match_large_sync_to_veccore1)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    ChipProductType chipType = ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    CoreTimeLineVisualizer core(dataCenter, config);

    MergeInfo setInstr;
    setInstr.icacheTick = UINT64_MAX;
    setInstr.pc = 0x1000;
    setInstr.startTick = 100;
    setInstr.endTick = 200;
    setInstr.pipe = "CUBE";
    setInstr.name = "SET_INTRA_BLOCK";
    setInstr.detail = "PIPE:CUBE,sync_id:18,";
    core.recordIntraSetFlag_[{"core0", "cubecore0"}][{"SET_INTRA_BLOCK", 18}].emplace_back(
        std::pair<MergeInfo, bool>{setInstr, false});

    MergeInfo waitInstr;
    waitInstr.icacheTick = UINT64_MAX;
    waitInstr.pc = 0x2000;
    waitInstr.startTick = 200;
    waitInstr.endTick = 300;
    waitInstr.pipe = "VECTOR";
    waitInstr.name = "WAIT_INTRA_BLOCK";
    waitInstr.detail = "PIPE:VECTOR,sync_id:2,";
    core.recordIntraWaitFlag_[{"core0", "veccore1"}][{"WAIT_INTRA_BLOCK", 2}].emplace_back(
        std::pair<MergeInfo, bool>{waitInstr, false});

    size_t prevSize = core.coresJsonList_.size();
    core.CollectIntraBlockFlowEvents();
    ASSERT_EQ(core.coresJsonList_.size(), prevSize + 6);
    ASSERT_EQ(core.coresJsonList_[prevSize + 4].at("pid"), "core0.cubecore0");
    ASSERT_EQ(core.coresJsonList_[prevSize + 5].at("pid"), "core0.veccore1");
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | CollectIntraBlockFlowEvents
* |  用例名  | test_CollectIntraBlockFlowEvents_should_match_BLOCKI_variants
* | 用例描述 | 测试CollectIntraBlockFlowEvents正确匹配SET_INTRA_BLOCKI和WAIT_INTRA_BLOCKI
*/
TEST(CoreTimeLineVisualizer, test_CollectIntraBlockFlowEvents_should_match_BLOCKI_variants)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    ChipProductType chipType = ChipProductType::ASCEND910B1;
    SimVisualizerConfig config = GetVisualizeConfig(output, chipType);
    CoreTimeLineVisualizer core(dataCenter, config);

    MergeInfo setInstr;
    setInstr.icacheTick = UINT64_MAX;
    setInstr.pc = 0x1000;
    setInstr.startTick = 100;
    setInstr.endTick = 200;
    setInstr.pipe = "CUBE";
    setInstr.name = "SET_INTRA_BLOCKI";
    setInstr.detail = "PIPE:CUBE,sync_id:5,";
    core.recordIntraSetFlag_[{"core0", "cubecore0"}][{"SET_INTRA_BLOCKI", 5}].emplace_back(
        std::pair<MergeInfo, bool>{setInstr, false});

    MergeInfo waitInstr;
    waitInstr.icacheTick = UINT64_MAX;
    waitInstr.pc = 0x2000;
    waitInstr.startTick = 200;
    waitInstr.endTick = 300;
    waitInstr.pipe = "VECTOR";
    waitInstr.name = "WAIT_INTRA_BLOCKI";
    waitInstr.detail = "PIPE:VECTOR,sync_id:5,";
    core.recordIntraWaitFlag_[{"core0", "veccore0"}][{"WAIT_INTRA_BLOCKI", 5}].emplace_back(
        std::pair<MergeInfo, bool>{waitInstr, false});

    size_t prevSize = core.coresJsonList_.size();
    core.CollectIntraBlockFlowEvents();
    ASSERT_EQ(core.coresJsonList_.size(), prevSize + 6);
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | GetIntraMatchCoreInfo
* |  用例名  | test_GetIntraMatchCoreInfo_should_handle_idx_out_of_range
* | 用例描述 | 测试GetIntraMatchCoreInfo在idx超出WAIT数量时返回空
*/
TEST(CoreTimeLineVisualizer, test_GetIntraMatchCoreInfo_should_handle_idx_out_of_range)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    CoreTimeLineVisualizer core(dataCenter, config);

    MergeInfo waitInstr;
    waitInstr.pc = 0x2000;
    waitInstr.startTick = 1200;
    waitInstr.endTick = 1300;
    waitInstr.pipe = "VECTOR";
    waitInstr.name = "WAIT_INTRA_BLOCK";
    waitInstr.detail = "PIPE:VECTOR,sync_id:5,";
    core.recordIntraWaitFlag_[{"core0", "veccore0"}][{"WAIT_INTRA_BLOCK", 5}].emplace_back(
        std::pair<MergeInfo, bool>{waitInstr, false});

    IntraFlag flag = {"core0", "cubecore0", "SET_INTRA_BLOCK", "", 5, 999, nullptr};
    ASSERT_FALSE(core.GetIntraMatchCoreInfo(flag));
}

/**
* |  用例集 | CoreTimeLineVisualizer
* | 测试函数 | CollectIntraBlockFlowEvents
* |  用例名  | test_CollectIntraBlockFlowEvents_should_not_generate_when_wait_in_wrong_subcore
* | 用例描述 | 测试CollectIntraBlockFlowEvents当WAIT在错误的subcore时无匹配
*/
TEST(CoreTimeLineVisualizer, test_CollectIntraBlockFlowEvents_should_not_generate_when_wait_in_wrong_subcore)
{
    DataCenter dataCenter;
    std::string output = "test/ut/resources/dump/output";
    SimVisualizerConfig config = GetVisualizeConfig(output, ChipProductType::ASCEND910B1);
    CoreTimeLineVisualizer core(dataCenter, config);

    MergeInfo setInstr;
    setInstr.icacheTick = UINT64_MAX;
    setInstr.pc = 0x1000;
    setInstr.startTick = 100;
    setInstr.endTick = 200;
    setInstr.pipe = "CUBE";
    setInstr.name = "SET_INTRA_BLOCK";
    setInstr.detail = "PIPE:CUBE,sync_id:5,";
    core.recordIntraSetFlag_[{"core0", "cubecore0"}][{"SET_INTRA_BLOCK", 5}].emplace_back(
        std::pair<MergeInfo, bool>{setInstr, false});

    // WAIT in wrong subcore: veccore1 instead of veccore0 for syncId < 16
    MergeInfo waitInstr;
    waitInstr.icacheTick = UINT64_MAX;
    waitInstr.pc = 0x2000;
    waitInstr.startTick = 200;
    waitInstr.endTick = 300;
    waitInstr.pipe = "VECTOR";
    waitInstr.name = "WAIT_INTRA_BLOCK";
    waitInstr.detail = "PIPE:VECTOR,sync_id:5,";
    core.recordIntraWaitFlag_[{"core0", "veccore1"}][{"WAIT_INTRA_BLOCK", 5}].emplace_back(
        std::pair<MergeInfo, bool>{waitInstr, false});

    size_t prevSize = core.coresJsonList_.size();
    core.CollectIntraBlockFlowEvents();
    ASSERT_EQ(core.coresJsonList_.size(), prevSize + 4);
}

}
