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

#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "smart_pointer.h"
#include "filesystem.h"
#define private public
#define protected public
#include "profiling/device/data_visualize/data_visualize.h"
#include "profiling/device/data_visualize/timeline_parser/mc2_timeline_parser.h"
#include "common/hal_helper.h"
#undef private
#undef protected

using namespace Visualize;
using namespace Profiling;
using namespace Common;
using namespace Utility;
using namespace std;

// mc2_timeline_parser.cpp中的自由函数未在头文件声明，此处补充声明以便直接测试
namespace Visualize {
void GetStreamIdAndTaskId(uint32_t &taskId, uint16_t &streamId);
void ProcessOperationStart(const MsprofAicTimeStampInfoUpdate &item, const std::string &operationType,
                           const std::string &type, std::unordered_map<std::string, OperationInfo> &operationMap);
void ProcessOperationEnd(const MsprofAicTimeStampInfoUpdate &item, const std::string &operationType,
                         std::unordered_map<std::string, OperationInfo> &operationMap);
}

namespace {
constexpr uint16_t MSPROF_AICPU_LEVEL_TEST = 6000;
constexpr uint16_t MSPROF_AICPU_COMM_TIME_TYPE_TEST = 4;   // communication time
constexpr uint16_t MSPROF_AICPU_HCCL_INFO_TYPE_TEST = 13;  // hccl task info
constexpr int64_t TEST_TASK_SCHED_FREQ = 1000;

// 构造aicpu.bin解析所需的 MsprofAicTimeStampInfoUpdate 数据
MsprofAicTimeStampInfoUpdate MakeTimeStamp(uint64_t syscyc, uint32_t blockId, uint32_t descId, uint64_t curPc)
{
    MsprofAicTimeStampInfoUpdate item {};
    item.syscyc = syscyc;
    item.blockId = blockId;
    item.descId = descId;
    item.curPc = curPc;
    item.type = AIC_BLOCK;
    return item;
}

// 构造一条 MsprofAdditionalInfo 记录并写入bin缓存
void FillAdditionalInfo(std::vector<char> &binData, size_t index, uint16_t level, uint32_t type)
{
    MsprofAdditionalInfo info{};
    info.level = level;
    info.type = type;
    ASSERT_TRUE(memcpy_s(&binData[index * sizeof(MsprofAdditionalInfo)], sizeof(MsprofAdditionalInfo),
                         &info, sizeof(MsprofAdditionalInfo)) == EOK);
}

class ScopedHalGetDeviceInfoMock {
public:
    explicit ScopedHalGetDeviceInfoMock(halGetDeviceInfoFunc func)
        : oldFunc_(Common::HalHelper::Instance().halGetDeviceInfo_)
    {
        Common::HalHelper::Instance().halGetDeviceInfo_ = func;
    }
    ~ScopedHalGetDeviceInfoMock() { Common::HalHelper::Instance().halGetDeviceInfo_ = oldFunc_; }
private:
    halGetDeviceInfoFunc oldFunc_;
};
}

class Mc2TimelineParserTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override
    {
        GlobalMockObject::verify();
    }

    void InitParser(const AcsqTimeMapType &acsqTimeMap, uint64_t minMc2TimeCyc, const std::string &soc = "")
    {
        handler_ = Utility::MakeUnique<DataHandlerOf910B>();
        opBasicInfoPtr_ = Utility::MakeShared<Visualize::OpBasicInfo>(handler_);
        if (!soc.empty()) {
            opBasicInfoPtr_->soc_ = soc;
        }
        basicPmuPtr_ = Utility::MakeShared<Visualize::BasicPmu>(handler_);
        parser_.reset(new (std::nothrow) MC2TimelineParser(acsqTimeMap, minMc2TimeCyc, opBasicInfoPtr_,
                                                           basicPmuPtr_));
        ASSERT_NE(parser_, nullptr);
    }

    std::unique_ptr<DataHandler> handler_;
    std::shared_ptr<Visualize::OpBasicInfo> opBasicInfoPtr_;
    std::shared_ptr<Visualize::BasicPmu> basicPmuPtr_;
    std::unique_ptr<MC2TimelineParser> parser_;
};

// 测试GetStreamIdAndTaskId，streamId第13位置位时taskId与streamId高位交换
TEST_F(Mc2TimelineParserTest, get_stream_id_and_task_id_when_bit13_set_expect_exchange)
{
    uint32_t taskId = 0xABCD;
    uint16_t streamId = 0x9005;
    GetStreamIdAndTaskId(taskId, streamId);
    EXPECT_EQ(taskId, (0xABCD & 0x1FFF) | (0x9005 & 0xE000));
    EXPECT_EQ(streamId, 5);
}

// 测试GetStreamIdAndTaskId，streamId第14位置位时低12位交换
TEST_F(Mc2TimelineParserTest, get_stream_id_and_task_id_when_bit14_set_expect_low12_exchange)
{
    uint32_t taskId = 0x5ABC;
    uint16_t streamId = 0x2003;
    GetStreamIdAndTaskId(taskId, streamId);
    EXPECT_EQ(taskId, (0x5ABC & 0xF000) | (0x2003 & 0x0FFF));
    EXPECT_EQ(streamId, 0x5ABC & 0x0FFF);
}

// 测试GetStreamIdAndTaskId，标志位均未置位时streamId取模、taskId保持不变
TEST_F(Mc2TimelineParserTest, get_stream_id_and_task_id_when_no_flag_expect_modulo)
{
    uint32_t taskId = 100;
    uint16_t streamId = 0x0805;
    GetStreamIdAndTaskId(taskId, streamId);
    EXPECT_EQ(taskId, 100u);
    EXPECT_EQ(streamId, 5);
}

// 测试ProcessOperationStart，首次start打点记录成功且重复start不覆盖
TEST_F(Mc2TimelineParserTest, process_operation_start_when_first_start_expect_recorded)
{
    MsprofAicTimeStampInfoUpdate item = MakeTimeStamp(100, 2, 0x1000, 0x1234);
    std::unordered_map<std::string, OperationInfo> operationMap;
    ProcessOperationStart(item, "INIT_OP", AIC_BLOCK, operationMap);
    ASSERT_TRUE(operationMap["INIT_OP"].startFound);
    EXPECT_EQ(operationMap["INIT_OP"].startSyscyc, 100u);
    EXPECT_EQ(operationMap["INIT_OP"].blockId, 2u);
    EXPECT_EQ(operationMap["INIT_OP"].startCurPc, 0x1234u);
    EXPECT_EQ(operationMap["INIT_OP"].type, AIC_BLOCK);

    // 重复start不覆盖首次记录
    MsprofAicTimeStampInfoUpdate itemDup = MakeTimeStamp(200, 3, 0x1000, 0x1234);
    ProcessOperationStart(itemDup, "INIT_OP", AIC_BLOCK, operationMap);
    EXPECT_EQ(operationMap["INIT_OP"].startSyscyc, 100u);
    EXPECT_EQ(operationMap["INIT_OP"].blockId, 2u);

    // 奇数descId是end打点，不记录start
    MsprofAicTimeStampInfoUpdate itemEnd = MakeTimeStamp(300, 2, 0x1001, 0x1234);
    ProcessOperationStart(itemEnd, "END_OP", AIC_BLOCK, operationMap);
    EXPECT_FALSE(operationMap["END_OP"].startFound);
}

// 测试ProcessOperationEnd，start已记录时end打点记录成功，其余情况忽略
TEST_F(Mc2TimelineParserTest, process_operation_end_when_start_found_expect_end_recorded)
{
    std::unordered_map<std::string, OperationInfo> operationMap;
    ProcessOperationStart(MakeTimeStamp(100, 2, 0x1000, 0x1234), "INIT_OP", AIC_BLOCK, operationMap);
    ProcessOperationEnd(MakeTimeStamp(200, 2, 0x1001, 0x1238), "INIT_OP", operationMap);
    ASSERT_TRUE(operationMap["INIT_OP"].endFound);
    EXPECT_EQ(operationMap["INIT_OP"].endSyscyc, 200u);

    // 偶数descId不是end打点，不记录end
    std::unordered_map<std::string, OperationInfo> operationMap2;
    ProcessOperationEnd(MakeTimeStamp(100, 2, 0x1000, 0x1234), "EVEN_OP", operationMap2);
    EXPECT_FALSE(operationMap2["EVEN_OP"].endFound);

    // 无start记录时end打点被忽略
    std::unordered_map<std::string, OperationInfo> operationMap3;
    ProcessOperationEnd(MakeTimeStamp(200, 2, 0x1001, 0x1238), "NO_START_OP", operationMap3);
    EXPECT_FALSE(operationMap3["NO_START_OP"].endFound);
}

// 测试BuildAicoreDot，频率为0时返回空json
TEST_F(Mc2TimelineParserTest, build_aicore_dot_when_freq_zero_expect_empty_json)
{
    InitParser(AcsqTimeMapType{}, 1000);
    parser_->aicpuFreq_ = 0;
    OperationInfo info{};
    info.startSyscyc = 2000;
    info.endSyscyc = 3000;
    info.blockId = 0;
    info.startCurPc = 0x1000;
    info.startFound = true;
    info.endFound = true;
    info.type = AIC_BLOCK;
    nlohmann::json result = parser_->BuildAicoreDot(info, "INIT");
    EXPECT_TRUE(result.is_null());
}

// 测试BuildAicoreDot，存在pc代码映射时生成带代码信息的打点事件
TEST_F(Mc2TimelineParserTest, build_aicore_dot_with_pc_code_expect_code_and_metrics)
{
    InitParser(AcsqTimeMapType{}, 1000);
    parser_->minSysCyc_ = 1000;
    parser_->aicpuFreq_ = 50000;
    parser_->pc2code_[0x1000] = std::vector<std::string>{"code line1", "code line2"};
    OperationInfo info{};
    info.startSyscyc = 2000;
    info.endSyscyc = 3000;
    info.blockId = 0;
    info.startCurPc = 0x1000;
    info.startFound = true;
    info.endFound = true;
    info.type = AIC_BLOCK;
    nlohmann::json result = parser_->BuildAicoreDot(info, "INIT");
    EXPECT_EQ(result["name"], "INIT");
    EXPECT_EQ(result["cname"], std::string(VISUALIZE_COLOR_NAME::PINK));
    EXPECT_EQ(result["ph"], "X");
    EXPECT_EQ(result["pid"], BLOCK);
    EXPECT_FLOAT_EQ(result["dur"].get<float>(), 20.0f);
    EXPECT_FLOAT_EQ(result["ts"].get<float>(), 20.0f);
    EXPECT_EQ(result["args"]["code"], "code line1\ncode line2");
    EXPECT_FALSE(result["args"]["pc_addr"].get<std::string>().empty());
}

// 测试BuildAicoreDot，无pc代码映射且操作类型无颜色配置时code为空、cname缺省
TEST_F(Mc2TimelineParserTest, build_aicore_dot_without_pc_code_expect_empty_code)
{
    InitParser(AcsqTimeMapType{}, 1000);
    parser_->minSysCyc_ = 1000;
    parser_->aicpuFreq_ = 50000;
    OperationInfo info{};
    info.startSyscyc = 2000;
    info.endSyscyc = 3000;
    info.blockId = 0;
    info.startCurPc = 0x2000;
    info.startFound = true;
    info.endFound = true;
    info.type = AIC_BLOCK;
    nlohmann::json result = parser_->BuildAicoreDot(info, "UNKNOWN_OP");
    EXPECT_EQ(result["name"], "UNKNOWN_OP");
    EXPECT_FALSE(result.contains("cname"));
    EXPECT_EQ(result["args"]["code"], "");
}

// 测试GetAicpuDatas，aicpu.bin文件不存在时不解析出任何数据
TEST_F(Mc2TimelineParserTest, get_aicpu_datas_when_file_missing_expect_empty)
{
    InitParser(AcsqTimeMapType{}, 1000);
    parser_->outputPath_ = "test/ut/resources/mc2_timeline_not_exist";
    std::vector<AicpuKfcProfCommTurn> aicpuTurns;
    std::vector<MsprofAicpuHcclTaskInfo> aicpuHcclTasks;
    parser_->GetAicpuDatas(aicpuTurns, aicpuHcclTasks);
    EXPECT_TRUE(aicpuTurns.empty());
    EXPECT_TRUE(aicpuHcclTasks.empty());
}

// 测试GetAicpuDatas，读取aicpu.bin文件失败时不解析出任何数据
TEST_F(Mc2TimelineParserTest, get_aicpu_datas_when_read_fail_expect_empty)
{
    InitParser(AcsqTimeMapType{}, 1000);
    parser_->outputPath_ = "test/ut/resources/mc2_timeline_not_exist";
    MOCKER(&GetFileSize)
        .stubs()
        .will(returnValue(static_cast<size_t>(sizeof(MsprofAdditionalInfo))));
    MOCKER(&ReadBinFileByMultiStruct)
        .stubs()
        .will(returnValue(false));
    std::vector<AicpuKfcProfCommTurn> aicpuTurns;
    std::vector<MsprofAicpuHcclTaskInfo> aicpuHcclTasks;
    parser_->GetAicpuDatas(aicpuTurns, aicpuHcclTasks);
    EXPECT_TRUE(aicpuTurns.empty());
    EXPECT_TRUE(aicpuHcclTasks.empty());
}

// 测试GetAicpuDatas，构造合法的aicpu.bin数据后turn与hccl task解析正确
TEST_F(Mc2TimelineParserTest, get_aicpu_datas_with_valid_bin_expect_turn_and_hccl_parsed)
{
    InitParser(AcsqTimeMapType{}, 1000);
    parser_->outputPath_ = "test/ut/resources/mc2_timeline_not_exist";

    std::vector<char> binData(sizeof(MsprofAdditionalInfo) * 4, 0);
    // 记录0：level=6000, type=4，携带一条turn数据
    FillAdditionalInfo(binData, 0, MSPROF_AICPU_LEVEL_TEST, MSPROF_AICPU_COMM_TIME_TYPE_TEST);
    AicpuKfcProfCommTurn turn{};
    turn.currentTurn = 3;
    turn.streamId = 60;
    turn.taskId = 100;
    MsprofAdditionalInfo *info0 = reinterpret_cast<MsprofAdditionalInfo *>(&binData[0]);
    ASSERT_TRUE(memcpy_s(info0->data, sizeof(info0->data), &turn, sizeof(turn)) == EOK);
    // 记录1：level不匹配，被跳过
    FillAdditionalInfo(binData, 1, 1234, MSPROF_AICPU_COMM_TIME_TYPE_TEST);
    // 记录2：level匹配但type不是4或13，被跳过
    FillAdditionalInfo(binData, 2, MSPROF_AICPU_LEVEL_TEST, 99);
    // 记录3：level=6000, type=13，携带2条hccl task数据，第2条groupName为0被跳过
    // （注意：源码循环中beginIndex仅在groupName非0时递增，因此第1条必须有效才能解析到第2条）
    FillAdditionalInfo(binData, 3, MSPROF_AICPU_LEVEL_TEST, MSPROF_AICPU_HCCL_INFO_TYPE_TEST);
    MsprofAicpuHcclTaskInfo hccl0{};
    hccl0.groupName = 123;
    hccl0.streamId = 0x1005;
    hccl0.taskId = 0xABCD;
    hccl0.planeID = 2;
    MsprofAicpuHcclTaskInfo hccl1{};
    MsprofAdditionalInfo *info3 = reinterpret_cast<MsprofAdditionalInfo *>(&binData[3 * sizeof(MsprofAdditionalInfo)]);
    ASSERT_TRUE(memcpy_s(info3->data, sizeof(info3->data), &hccl0, sizeof(hccl0)) == EOK);
    ASSERT_TRUE(memcpy_s(info3->data + sizeof(hccl0), sizeof(info3->data) - sizeof(hccl0),
                         &hccl1, sizeof(hccl1)) == EOK);

    MOCKER(&GetFileSize)
        .stubs()
        .will(returnValue(static_cast<size_t>(binData.size())));
    MOCKER(&ReadBinFileByMultiStruct)
        .stubs()
        .with(any(), any(), any(), outBound(binData))
        .will(returnValue(true));

    std::vector<AicpuKfcProfCommTurn> aicpuTurns;
    std::vector<MsprofAicpuHcclTaskInfo> aicpuHcclTasks;
    parser_->GetAicpuDatas(aicpuTurns, aicpuHcclTasks);

    ASSERT_EQ(aicpuTurns.size(), 1);
    EXPECT_EQ(aicpuTurns[0].currentTurn, 3);
    EXPECT_EQ(aicpuTurns[0].streamId, 60);
    EXPECT_EQ(aicpuTurns[0].taskId, 100);

    ASSERT_EQ(aicpuHcclTasks.size(), 1);
    EXPECT_EQ(aicpuHcclTasks[0].groupName, 123u);
    EXPECT_EQ(aicpuHcclTasks[0].planeID, 2);
    // streamId第13位置位，taskId与streamId按规则交换
    EXPECT_EQ(aicpuHcclTasks[0].taskId, 0xABCD & 0x1FFF);
    EXPECT_EQ(aicpuHcclTasks[0].streamId, 5);
}

// 测试PreProcessData，aicpu.bin与aic_timestamp.bin均缺失时输出为空
TEST_F(Mc2TimelineParserTest, pre_process_data_when_bin_files_missing_expect_empty)
{
    InitParser(AcsqTimeMapType{}, 1000);
    parser_->outputPath_ = "test/ut/resources/mc2_timeline_not_exist";
    std::vector<AicpuKfcProfCommTurn> aicpuTurns;
    std::vector<MsprofAicpuHcclTaskInfo> aicpuHcclTasks;
    std::vector<MsprofAicTimeStampInfoUpdate> aicoreTimeStamps;
    parser_->PreProcessData(aicpuTurns, aicpuHcclTasks, aicoreTimeStamps);
    EXPECT_TRUE(aicpuTurns.empty());
    EXPECT_TRUE(aicpuHcclTasks.empty());
    EXPECT_TRUE(aicoreTimeStamps.empty());
}

// 测试ProcessMC2AicoreData，配对打点生成dot事件，非法与未配对打点被过滤
TEST_F(Mc2TimelineParserTest, process_mc2_aicore_data_with_paired_dots_expect_dot_event)
{
    InitParser(AcsqTimeMapType{}, 1000);
    parser_->minSysCyc_ = 1000;
    parser_->aicpuFreq_ = 50000;
    parser_->blockDuration_[std::make_pair(std::string(AIC_BLOCK), 0u)] = std::make_pair(1500UL, 5000UL);

    std::vector<MsprofAicTimeStampInfoUpdate> aicoreTimeStamps;
    // INIT配对打点，在block时长范围内，生成dot事件
    aicoreTimeStamps.push_back(MakeTimeStamp(2000, 0, 0x1000, 0x1000));
    aicoreTimeStamps.push_back(MakeTimeStamp(3000, 0, 0x1001, 0x1004));
    // 未注册的descId，被跳过
    aicoreTimeStamps.push_back(MakeTimeStamp(2100, 0, 0x9999, 0x1008));
    // COMMIT只有start没有end，被跳过
    aicoreTimeStamps.push_back(MakeTimeStamp(2200, 1, 0x1010, 0x1010));
    // WAIT配对成功但超出block时长范围，被跳过
    aicoreTimeStamps.push_back(MakeTimeStamp(2000, 2, 0x1020, 0x1020));
    aicoreTimeStamps.push_back(MakeTimeStamp(3000, 2, 0x1021, 0x1024));

    parser_->ProcessMC2AicoreData(aicoreTimeStamps);

    ASSERT_EQ(parser_->timelineJson_.size(), 1);
    nlohmann::json event = parser_->timelineJson_[0];
    EXPECT_EQ(event["name"], "INIT");
    EXPECT_EQ(event["cname"], std::string(VISUALIZE_COLOR_NAME::PINK));
    EXPECT_EQ(event["ph"], "X");
    EXPECT_EQ(event["pid"], BLOCK);
    EXPECT_FLOAT_EQ(event["dur"].get<float>(), 20.0f);
    EXPECT_FLOAT_EQ(event["ts"].get<float>(), 20.0f);
}

// 测试ProcessHcclTaskData，频率为0时直接返回不生成事件
TEST_F(Mc2TimelineParserTest, process_hccl_task_data_when_freq_zero_expect_return)
{
    InitParser(AcsqTimeMapType{}, 1000);
    parser_->aicpuFreq_ = 0;
    MsprofAicpuHcclTaskInfo info{};
    parser_->ProcessHcclTaskData({info});
    EXPECT_TRUE(parser_->timelineJson_.empty());
}

// 测试AddAicpuTurnJson，时间范围非法时不生成turn事件
TEST_F(Mc2TimelineParserTest, add_aicpu_turn_json_when_time_invalid_expect_skip)
{
    InitParser(AcsqTimeMapType{}, 1000);
    parser_->minSysCyc_ = 1000;
    parser_->aicpuFreq_ = 50000;
    AicpuKfcProfCommTurn turn{};
    turn.currentTurn = 0;
    turn.streamId = 60;
    turn.taskId = 100;
    // startTime早于minSysCyc_，被跳过
    parser_->AddAicpuTurnJson(turn, "StartServer", 500, 600);
    // endTime不大于startTime，被跳过
    parser_->AddAicpuTurnJson(turn, "StartServer", 2000, 1500);
    EXPECT_TRUE(parser_->timelineJson_.empty());
}

// 测试AddAicpuTurnJson，频率为0时不生成turn事件
TEST_F(Mc2TimelineParserTest, add_aicpu_turn_json_when_freq_zero_expect_skip)
{
    InitParser(AcsqTimeMapType{}, 1000);
    parser_->minSysCyc_ = 1000;
    parser_->aicpuFreq_ = 0;
    AicpuKfcProfCommTurn turn{};
    turn.currentTurn = 0;
    turn.streamId = 60;
    turn.taskId = 100;
    parser_->AddAicpuTurnJson(turn, "StartServer", 2000, 3000);
    EXPECT_TRUE(parser_->timelineJson_.empty());
}

// 测试ProcessHcclData，频率为0时遍历到有效任务后直接返回
TEST_F(Mc2TimelineParserTest, process_hccl_data_when_freq_zero_expect_return)
{
    uint64_t minCyc = 100000;
    AcsqTimeMapType acsqTimeMap;
    acsqTimeMap[std::make_pair(static_cast<uint16_t>(1), static_cast<uint16_t>(4))] =
        AcsqTaskInfo{1, minCyc, minCyc + 5000};
    InitParser(acsqTimeMap, minCyc);
    parser_->aicpuFreq_ = 0;
    parser_->ProcessHcclData();
    EXPECT_TRUE(parser_->timelineJson_.empty());
}

// 测试ProcessJsonData，空输入下各处理流程正常执行且不生成事件
TEST_F(Mc2TimelineParserTest, process_json_data_with_empty_input_expect_no_event)
{
    InitParser(AcsqTimeMapType{}, 1000);
    parser_->minSysCyc_ = 1000;
    parser_->aicpuFreq_ = 50000;
    std::vector<AicpuKfcProfCommTurn> aicpuTurns;
    std::vector<MsprofAicpuHcclTaskInfo> aicpuHcclTasks;
    std::vector<MsprofAicTimeStampInfoUpdate> aicoreTimeStamps;
    parser_->ProcessJsonData(aicpuTurns, aicpuHcclTasks, aicoreTimeStamps);
    EXPECT_TRUE(parser_->timelineJson_.empty());
}

// 测试TimelineToJson，芯片类型不支持时返回false
TEST_F(Mc2TimelineParserTest, timeline_to_json_when_chip_not_support_expect_false)
{
    InitParser(AcsqTimeMapType{}, 1000, "Ascend310P3");
    EXPECT_FALSE(parser_->TimelineToJson("test/ut/resources/mc2_timeline_not_exist"));
}

// 测试TimelineToJson，获取频率失败时使用默认频率并成功生成结果
TEST_F(Mc2TimelineParserTest, timeline_to_json_when_freq_fallback_expect_success)
{
    ScopedHalGetDeviceInfoMock freqMock(nullptr);
    InitParser(AcsqTimeMapType{}, 1000, "Ascend910B1");
    ASSERT_TRUE(parser_->TimelineToJson("test/ut/resources/mc2_timeline_not_exist"));
    EXPECT_EQ(parser_->aicpuFreq_, FREQ);
    EXPECT_EQ(parser_->timelineJson_["profilingType"], "op");
    EXPECT_EQ(parser_->timelineJson_["displayTimeUnit"], "ns");
    EXPECT_EQ(parser_->timelineJson_["schemaVersion"], 1);
    EXPECT_TRUE(parser_->timelineJson_.contains("traceEvents"));
}

// 测试TimelineToJson，hal接口正常返回频率时成功生成结果
TEST_F(Mc2TimelineParserTest, timeline_to_json_with_hal_freq_expect_success)
{
    ScopedHalGetDeviceInfoMock freqMock([](uint32_t, int32_t, int32_t, int64_t *value) -> drvError_t {
        if (value != nullptr) {
            *value = TEST_TASK_SCHED_FREQ;
        }
        return DRV_ERROR_NONE;
    });
    InitParser(AcsqTimeMapType{}, 1000, "Ascend910B1");
    ASSERT_TRUE(parser_->TimelineToJson("test/ut/resources/mc2_timeline_not_exist"));
    EXPECT_EQ(parser_->aicpuFreq_, TEST_TASK_SCHED_FREQ);
    EXPECT_EQ(parser_->timelineJson_["profilingType"], "op");
    EXPECT_TRUE(parser_->timelineJson_.contains("traceEvents"));
}
