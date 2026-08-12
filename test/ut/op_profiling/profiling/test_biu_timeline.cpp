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

#include "filesystem.h"
#define private public
#define protected public
#include "profiling/device/data_visualize/biu_timeline.h"
#undef private
#undef protected

using namespace Visualize;
using namespace Utility;
using namespace std;

class BiuTimelineTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {
        GlobalMockObject::verify();
    }
    void PrepareBiuPerfTestData(vector<char>& totalBin) {
        totalBin.resize(2097160, 0);
        InstrProfHeadInfo headInfo;
        headInfo.coreId = 0;
        headInfo.coreType = 1;
        headInfo.validLen = 24;
        BiuPerfInfo biuPerfInfo1;
        biuPerfInfo1.cycles = 10;
        biuPerfInfo1.biuInfo = 0xf001;
        BiuPerfInfo biuPerfInfo2;
        biuPerfInfo2.cycles = 20;
        biuPerfInfo2.biuInfo = 0xf000;
        BiuPerfInfo biuPerfInfo3;
        biuPerfInfo3.cycles = 30;
        biuPerfInfo3.biuInfo = 0x2111;
        BiuPerfInfo biuPerfInfo4;
        biuPerfInfo4.cycles = 40;
        biuPerfInfo4.biuInfo = 0xa60c;
        BiuPerfInfo biuPerfInfo5;
        biuPerfInfo5.cycles = 50;
        biuPerfInfo5.biuInfo = 0xa60c;
        BiuPerfInfo biuPerfInfo6;
        biuPerfInfo6.cycles = 60;
        biuPerfInfo6.biuInfo = 0x060d;
        ASSERT_TRUE(memcpy_s(&totalBin[0], sizeof(totalBin), &headInfo, sizeof(InstrProfHeadInfo)) == EOK);
        ASSERT_TRUE(memcpy_s(&totalBin[8], sizeof(BiuPerfInfo), &biuPerfInfo1, sizeof(BiuPerfInfo)) == EOK);
        ASSERT_TRUE(memcpy_s(&totalBin[12], sizeof(BiuPerfInfo), &biuPerfInfo2, sizeof(BiuPerfInfo)) == EOK);
        ASSERT_TRUE(memcpy_s(&totalBin[16], sizeof(BiuPerfInfo), &biuPerfInfo3, sizeof(BiuPerfInfo)) == EOK);
        ASSERT_TRUE(memcpy_s(&totalBin[20], sizeof(BiuPerfInfo), &biuPerfInfo4, sizeof(BiuPerfInfo)) == EOK);
        ASSERT_TRUE(memcpy_s(&totalBin[24], sizeof(BiuPerfInfo), &biuPerfInfo5, sizeof(BiuPerfInfo)) == EOK);
        ASSERT_TRUE(memcpy_s(&totalBin[28], sizeof(BiuPerfInfo), &biuPerfInfo6, sizeof(BiuPerfInfo)) == EOK);
    }

    void MockFileOperations(const vector<char>& totalBin) {
        MOCKER(&IsReadable)
            .stubs()
            .will(returnValue(true));
        MOCKER(&GetFileSize)
            .stubs()
            .will(returnValue(size_t(2097160)));
        MOCKER(&ReadBinFileByMultiStruct)
            .stubs()
            .with(any(), any(), any(), outBound(totalBin))
            .will(returnValue(true));
    }
};

/**
/* | 用例集 | BiuTimelineTest
/* |测试函数| BiuTimeline::TimelineToJson()
/* | 用例名 | test_TimelineToJson_parse_json_success
/* |用例描述| 测试TimelineToJson，生成json文件结果正常
*/
TEST_F(BiuTimelineTest, test_TimelineToJson_parse_json_success)
{
    BiuTimeline biuTimeline;
    MOCKER(&BiuTimeline::ParseBiuTimeStamps)
        .stubs()
        .will(returnValue(true));
    std::vector<std::vector<BiuTimelineInfo>> timelineVec{INSTR_PROF_CHANNEL_NUM};
    timelineVec[0].emplace_back(BiuTimelineInfo("SCALAR", "core0.veccore0", "MarkStamp0", 50, 1));
    timelineVec[1].emplace_back(BiuTimelineInfo("CUBE", "core0.veccore0", "MarkStamp0", 50, 1));
    biuTimeline.timelineVec_ = timelineVec;
    ASSERT_TRUE(biuTimeline.TimelineToJson("test/ut/resources/"));
    ASSERT_EQ(biuTimeline.timelineJson_["traceEvents"].size(), 2);
}

/**
/* | 用例集 | BiuTimelineTest
/* |测试函数| PipeBiuTimeline::ParseSingleBiuTimeStamps()
/* | 用例名 | test_ParseSingleBiuTimeStamps_pipe_timeline_success
/* |用例描述| 测试ParseSingleBiuTimeStamps，解析pipe流水结果正常
*/
TEST_F(BiuTimelineTest, test_ParseSingleBiuTimeStamps_pipe_timeline_success)
{
    vector<char> totalBin;
    PrepareBiuPerfTestData(totalBin);
    MOCKER(&IsReadable)
        .stubs()
        .will(returnValue(true));
    MOCKER(&GetFileSize)
        .stubs()
        .will(returnValue(size_t(2097160)));
    MOCKER(&ReadBinFileByMultiStruct)
        .stubs()
        .with(any(), any(), any(), outBound(totalBin))
        .will(returnValue(true));
    PipeBiuTimeline pipeBiuTimeline;
    ASSERT_TRUE(pipeBiuTimeline.ParseSingleBiuTimeStamps("test/ut/resources/"));
    std::vector<BiuTimelineInfo> timelineVec = pipeBiuTimeline.timelineVec_[1];
    ASSERT_EQ(timelineVec.size(), 5);
    ASSERT_EQ(timelineVec[0].pipeName, "SCALAR");
    ASSERT_EQ(timelineVec[0].coreName, "core0.veccore0");
    ASSERT_EQ(timelineVec[0].lineName, "SCALAR");
    ASSERT_EQ(timelineVec[0].start, 10);
    ASSERT_EQ(timelineVec[0].duration, 20);
    ASSERT_EQ(timelineVec[1].pipeName, "CUBE");
    ASSERT_EQ(timelineVec[1].coreName, "core0.veccore0");
    ASSERT_EQ(timelineVec[1].lineName, "MarkStamp273");
    ASSERT_EQ(timelineVec[1].start, 60);
    ASSERT_EQ(timelineVec[1].duration, 1);
    ASSERT_EQ(timelineVec[2].pipeName, "FIXP");
    ASSERT_EQ(timelineVec[2].coreName, "core0.veccore0");
    ASSERT_EQ(timelineVec[2].lineName, "MarkStamp1548");
    ASSERT_EQ(timelineVec[2].start, 100);
    ASSERT_EQ(timelineVec[2].duration, 1);
    ASSERT_EQ(timelineVec[3].pipeName, "FIXP");
    ASSERT_EQ(timelineVec[3].coreName, "core0.veccore0");
    ASSERT_EQ(timelineVec[3].lineName, "MarkStamp1548");
    ASSERT_EQ(timelineVec[3].start, 150);
    ASSERT_EQ(timelineVec[3].duration, 1);
    ASSERT_EQ(timelineVec[4].pipeName, "SCALAR");
    ASSERT_EQ(timelineVec[4].coreName, "core0.veccore0");
    ASSERT_EQ(timelineVec[4].lineName, "MarkStamp1549");
    ASSERT_EQ(timelineVec[4].start, 210);
    ASSERT_EQ(timelineVec[4].duration, 1);
}

/**
/* | 用例集 | BiuTimelineTest
/* |测试函数| InstrBiuTimeline::ParseSingleBiuTimeStamps()
/* | 用例名 | test_ParseSingleBiuTimeStamps_instr_timeline_success
/* |用例描述| 测试ParseSingleBiuTimeStamps，解析instr流水结果正常
*/
TEST_F(BiuTimelineTest, test_ParseSingleBiuTimeStamps_instr_timeline_success)
{
    vector<char> totalBin;
    PrepareBiuPerfTestData(totalBin);
    MOCKER(&IsReadable)
        .stubs()
        .will(returnValue(true));
    MOCKER(&GetFileSize)
        .stubs()
        .will(returnValue(size_t(2097160)));
    MOCKER(&ReadBinFileByMultiStruct)
        .stubs()
        .with(any(), any(), any(), outBound(totalBin))
        .will(returnValue(true));
    InstrBiuTimeline instrBiuTimeline;
    ASSERT_TRUE(instrBiuTimeline.ParseSingleBiuTimeStamps("test/ut/resources/"));
    std::vector<BiuTimelineInfo> timelineVec = instrBiuTimeline.timelineVec_[1];
    ASSERT_EQ(timelineVec.size(), 1);
    ASSERT_EQ(timelineVec[0].pipeName, "FIXP");
    ASSERT_EQ(timelineVec[0].coreName, "core0.veccore0");
    ASSERT_EQ(timelineVec[0].lineName, "Instr1548");
    ASSERT_EQ(timelineVec[0].start, 100);
    ASSERT_EQ(timelineVec[0].duration, 50);
}

/**
/* | 用例集 | BiuTimelineTest
/* |测试函数| InstrBiuTimeline::PrintPipeIdFull()
/* | 用例名 | test_PrintPipeIdFull_parse_log_success
/* |用例描述| 测试PrintPipeIdFull，解析tune日志正常，id达到1024以上的pipe日志提示
*/
TEST_F(BiuTimelineTest, test_PrintPipeIdFull_parse_log_success)
{
    ofstream file("test/ut/resources/dump/dfx_tune.log");
    ASSERT_TRUE(file.is_open());
    file << "TUNE-ERROR: all dfx ids consumed[pipe=mte2]" << endl;
    file << "TUNE-ERROR: all dfx ids consumed[pipe=mte3]" << endl;
    file << "TUNE-ERROR: all dfx ids consumed[pipe=cube]" << endl;
    file.close();
    InstrBiuTimeline instrBiuTimeline;
    instrBiuTimeline.outputPath_ = "test/ut/resources";
    testing::internal::CaptureStdout();
    instrBiuTimeline.PrintPipeIdFull();
    std::string capture = testing::internal::GetCapturedStdout();
    EXPECT_NE(capture.find("InstrTimeline of pipes[cube, mte2, mte3] is incomplete because these pipes have more than 1024 instructions."), std::string::npos);
}

/**
/* | 用例集 | BiuTimelineTest
/* |测试函数| BiuTimeline::UpdateEndMarks()
/* | 用例名 | test_UpdateEndMarks_not_in_sequence
/* |用例描述| 测试UpdateEndMarks，markId不在end标记序列中，删除该通道的endMark
*/
TEST_F(BiuTimelineTest, test_UpdateEndMarks_not_in_sequence)
{
    BiuTimeline biuTimeline;
    EndMarkState state;
    state.startIndex = 10;
    state.nextExpectedIdx = 2;
    biuTimeline.channelEndMarkMap_[1] = state;
    biuTimeline.UpdateEndMarks(0x100, 1);
    ASSERT_EQ(biuTimeline.channelEndMarkMap_.find(1), biuTimeline.channelEndMarkMap_.end());
}

/**
/* | 用例集 | BiuTimelineTest
/* |测试函数| BiuTimeline::UpdateEndMarks()
/* | 用例名 | test_UpdateEndMarks_first_in_sequence
/* |用例描述| 测试UpdateEndMarks，markId是end标记序列的第一个，开始新的顺序匹配
*/
TEST_F(BiuTimelineTest, test_UpdateEndMarks_first_in_sequence)
{
    BiuTimeline biuTimeline;
    biuTimeline.timelineVec_[1].emplace_back(BiuTimelineInfo("SCALAR", "core0.veccore0", "MarkStamp0", 50, 1));
    biuTimeline.timelineVec_[1].emplace_back(BiuTimelineInfo("CUBE", "core0.veccore0", "MarkStamp1", 100, 1));
    biuTimeline.UpdateEndMarks(0xd88, 1);
    ASSERT_EQ(biuTimeline.channelEndMarkMap_[1].startIndex, 2);
    ASSERT_EQ(biuTimeline.channelEndMarkMap_[1].nextExpectedIdx, 1);
}

/**
/* | 用例集 | BiuTimelineTest
/* |测试函数| BiuTimeline::UpdateEndMarks()
/* | 用例名 | test_UpdateEndMarks_sequence_not_continuous
/* |用例描述| 测试UpdateEndMarks，打点序列不连续，清空channelEndMarkMap
*/
TEST_F(BiuTimelineTest, test_UpdateEndMarks_sequence_not_continuous)
{
    BiuTimeline biuTimeline;
    EndMarkState state;
    state.startIndex = 10;
    state.nextExpectedIdx = 2;
    biuTimeline.channelEndMarkMap_[1] = state;
    biuTimeline.UpdateEndMarks(0xdff, 1);
    ASSERT_EQ(biuTimeline.channelEndMarkMap_.find(1), biuTimeline.channelEndMarkMap_.end());
}

/**
/* | 用例集 | BiuTimelineTest
/* |测试函数| BiuTimeline::UpdateEndMarks()
/* | 用例名 | test_UpdateEndMarks_sequence_continuous
/* |用例描述| 测试UpdateEndMarks，打点序列连续，正常更新nextExpectedIdx
*/
TEST_F(BiuTimelineTest, test_UpdateEndMarks_sequence_continuous)
{
    BiuTimeline biuTimeline;
    EndMarkState state;
    state.startIndex = 10;
    state.nextExpectedIdx = 2;
    biuTimeline.channelEndMarkMap_[1] = state;
    biuTimeline.UpdateEndMarks(0xdaa, 1);
    ASSERT_EQ(biuTimeline.channelEndMarkMap_[1].startIndex, 10);
    ASSERT_EQ(biuTimeline.channelEndMarkMap_[1].nextExpectedIdx, 3);
}

/**
/* | 用例集 | BiuTimelineTest
/* |测试函数| BiuTimeline::UpdateEndMarks()
/* | 用例名 | test_UpdateEndMarks_full_sequence
/* |用例描述| 测试UpdateEndMarks，完整的8个连续打点序列
*/
TEST_F(BiuTimelineTest, test_UpdateEndMarks_full_sequence)
{
    BiuTimeline biuTimeline;
    for (size_t i = 0; i < biuTimeline.endMarkSequence_.size(); i++) {
        biuTimeline.UpdateEndMarks(biuTimeline.endMarkSequence_[i], 1);
        ASSERT_EQ(biuTimeline.channelEndMarkMap_[1].nextExpectedIdx, i + 1);
    }
    ASSERT_EQ(biuTimeline.channelEndMarkMap_[1].nextExpectedIdx, 8);
}

/**
/* | 用例集 | BiuTimelineTest
/* |测试函数| InstrBiuTimeline::ParseDfxMapInfo()
/* | 用例名 | test_ParseDfxMapInfo_file_not_exist
/* |用例描述| 测试ParseDfxMapInfo，文件不存在时直接返回
*/
TEST_F(BiuTimelineTest, test_ParseDfxMapInfo_file_not_exist)
{
    InstrBiuTimeline instrBiuTimeline;
    instrBiuTimeline.outputPath_ = "test/ut/resources/nonexistent_path";
    instrBiuTimeline.ParseDfxMapInfo();
    ASSERT_TRUE(instrBiuTimeline.dfxRegionInfoMap_.empty());
}

/**
/* | 用例集 | BiuTimelineTest
/* |测试函数| InstrBiuTimeline::ParseDfxMapInfo()
/* | 用例名 | test_ParseDfxMapInfo_parse_success
/* |用例描述| 测试ParseDfxMapInfo，解析dfx_region_map.txt成功
*/
TEST_F(BiuTimelineTest, test_ParseDfxMapInfo_parse_success)
{
    std::string mapFileStr = "test/ut/resources/dump/dfx_region_map.txt";
    ofstream mapFile(mapFileStr);
    ASSERT_TRUE(mapFile.is_open());
    mapFile << "region_id=100,pipe=FIXP,some_info,pc=0x1000,opcode=add_op" << endl;
    mapFile << "region_id=200,pipe=cube,some_info,pc=0x2000,opcode=mul_op" << endl;
    mapFile << "region_id=300,pipe=invalid,some_info,pc=0x3000,opcode=sub_op" << endl;
    mapFile << "invalid_line" << endl;
    mapFile.close();
    std::string pcStartFileStr = "test/ut/resources/dump/pc_start_addr.txt";
    ofstream pcStartFile(pcStartFileStr);
    ASSERT_TRUE(pcStartFile.is_open());
    pcStartFile << "0x10000" << endl;
    pcStartFile.close();

    InstrBiuTimeline instrBiuTimeline;
    instrBiuTimeline.outputPath_ = "test/ut/resources";
    instrBiuTimeline.ParseDfxMapInfo();
    ASSERT_EQ(instrBiuTimeline.dfxRegionInfoMap_.size(), 2);
    ASSERT_EQ(instrBiuTimeline.dfxRegionInfoMap_.count({"FIXP", 100}), 1);
    ASSERT_EQ(instrBiuTimeline.dfxRegionInfoMap_.count({"CUBE", 200}), 1);
    ASSERT_EQ(instrBiuTimeline.dfxRegionInfoMap_[std::make_pair("FIXP", 100)].InstrName, "add_op");
    ASSERT_EQ(instrBiuTimeline.dfxRegionInfoMap_[std::make_pair("CUBE", 200)].InstrName, "mul_op");
    ASSERT_EQ(instrBiuTimeline.dfxRegionInfoMap_[std::make_pair("FIXP", 100)].pc, 0x11000);
    ASSERT_EQ(instrBiuTimeline.dfxRegionInfoMap_[std::make_pair("CUBE", 200)].pc, 0x12000);
    RemoveAll(mapFileStr);
    RemoveAll(pcStartFileStr);
}

/**
/* | 用例集 | BiuTimelineTest
/* |测试函数| InstrBiuTimeline::GetInstrNameId()
/* | 用例名 | test_GetInstrNameId_multiple_unique
/* |用例描述| 测试GetInstrNameId，多个不同名称分配不同id
*/
TEST_F(BiuTimelineTest, test_GetInstrNameId_multiple_unique)
{
    InstrBiuTimeline instrBiuTimeline;
    for (size_t i = 0; i < 10; i++) {
        std::string name = "instr_" + std::to_string(i);
        size_t id = instrBiuTimeline.GetInstrNameId(name);
        ASSERT_EQ(id, i);
    }
    ASSERT_EQ(instrBiuTimeline.GetInstrNameId("instr_9"), 9);
    ASSERT_EQ(instrBiuTimeline.instrName2Id_.size(), 10);
}

// ==================== trace_start / trace_end 打点测试 ====================

/**
 * | 用例集 | BiuTimelineTest
 * |测试函数| PipeBiuTimeline::ParseDfxRegion()
 * | 用例名 | test_ParseDfxRegion_trace_start_end_paired
 * |用例描述| 测试trace_start和trace_end配对成功，生成正确的timeline区间事件
 */
TEST_F(BiuTimelineTest, test_ParseDfxRegion_trace_start_end_paired) {
    PipeBiuTimeline pipeBiuTimeline;
    // 编码格式: bit11=start/end, bit10=固定1, bits0-9=regionId
    // trace_start(5): (0<<11)|(1<<10)|5 = 1029 (0x405)
    uint16_t traceStartId = 1029;
    // trace_end(5): (1<<11)|(1<<10)|5 = 3077 (0xC05)
    uint16_t traceEndId = 3077;

    // 模拟 start 打点，当前累计 cycle = 100
    pipeBiuTimeline.channelCycleMap_[1] = 100;
    pipeBiuTimeline.ParseDfxRegion(traceStartId, 1, "core0.veccore0", "VECTOR");

    // 模拟 end 打点，当前累计 cycle = 350
    pipeBiuTimeline.channelCycleMap_[1] = 350;
    pipeBiuTimeline.ParseDfxRegion(traceEndId, 1, "core0.veccore0", "VECTOR");

    // 验证生成3个事件: 2个MarkStamp + 1个区间事件
    auto &timelineVec = pipeBiuTimeline.timelineVec_[1];
    ASSERT_EQ(timelineVec.size(), 3);

    // 事件1: MarkStamp1029 (start打点的MarkStamp)
    ASSERT_EQ(timelineVec[0].lineName, "MarkStamp1029");
    ASSERT_EQ(timelineVec[0].start, 100);
    ASSERT_EQ(timelineVec[0].duration, 1);

    // 事件2: MarkStamp3077 (end打点的MarkStamp)
    ASSERT_EQ(timelineVec[1].lineName, "MarkStamp3077");
    ASSERT_EQ(timelineVec[1].start, 350);
    ASSERT_EQ(timelineVec[1].duration, 1);

    // 事件3: region_5 (trace区间事件)
    ASSERT_EQ(timelineVec[2].pipeName, "VECTOR");
    ASSERT_EQ(timelineVec[2].coreName, "core0.veccore0");
    ASSERT_EQ(timelineVec[2].lineName, "Region5");
    ASSERT_EQ(timelineVec[2].start, 100);
    ASSERT_EQ(timelineVec[2].duration, 250);
    // 验证trace事件颜色: regionId=5, TOTAL_CNAME_MAP[5]="thread_state_running"
    ASSERT_EQ(timelineVec[2].cName, "thread_state_running");
    // 验证缓存已清空
    ASSERT_TRUE(pipeBiuTimeline.traceStartCache_.empty());
}

/**
 * | 用例集 | BiuTimelineTest
 * |测试函数| PipeBiuTimeline::ParseDfxRegion()
 * | 用例名 | test_ParseDfxRegion_trace_end_without_start
 * |用例描述| 测试trace_end没有配对的trace_start，只生成MarkStamp点，未配对end被收集到unpairedTraces_
 */
TEST_F(BiuTimelineTest, test_ParseDfxRegion_trace_end_without_start) {
    PipeBiuTimeline pipeBiuTimeline;
    // 直接发送 trace_end(3): 3072+3 = 3075
    uint16_t traceEndId = 3075;
    pipeBiuTimeline.channelCycleMap_[1] = 200;
    pipeBiuTimeline.ParseDfxRegion(traceEndId, 1, "core0.veccore0", "VECTOR");

    // 验证生成了一个MarkStamp瞬时点
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[1].size(), 1);
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[1][0].lineName, "MarkStamp3075");
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[1][0].start, 200);
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[1][0].duration, 1);
    // 没有配对的start，start缓存为空
    ASSERT_TRUE(pipeBiuTimeline.traceStartCache_.empty());
    // 未配对的end被收集到unpairedTraces_
    ASSERT_EQ(pipeBiuTimeline.unpairedTraces_.size(), 1);
    ASSERT_EQ(*pipeBiuTimeline.unpairedTraces_.begin(), 3);
}

/**
 * | 用例集 | BiuTimelineTest
 * |测试函数| PipeBiuTimeline::ParseDfxRegion()
 * | 用例名 | test_ParseDfxRegion_trace_start_without_end
 * |用例描述| 测试trace_start没有配对的trace_end，只生成MarkStamp点，PrintMissData打debug日志
 */
TEST_F(BiuTimelineTest, test_ParseDfxRegion_trace_start_without_end) {
    PipeBiuTimeline pipeBiuTimeline;
    // trace_start(7): 1024+7 = 1031
    uint16_t traceStartId = 1031;
    pipeBiuTimeline.channelCycleMap_[1] = 500;
    pipeBiuTimeline.ParseDfxRegion(traceStartId, 1, "core0.veccore0", "CUBE");

    // 验证生成了1个MarkStamp事件，缓存中存在start记录
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[1].size(), 1);
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[1][0].lineName, "MarkStamp1031");
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[1][0].start, 500);
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[1][0].duration, 1);
    ASSERT_EQ(pipeBiuTimeline.traceStartCache_.size(), 1);
    auto key = std::make_tuple(std::string("CUBE"), 1u, (uint16_t)7);
    ASSERT_EQ(pipeBiuTimeline.traceStartCache_[key].startCycle, 500);
    ASSERT_EQ(pipeBiuTimeline.traceStartCache_[key].coreName, "core0.veccore0");

    // 调用PrintMissData，只打debug日志，不生成新事件
    pipeBiuTimeline.PrintMissData();

    // 验证事件数量不变，缓存已清空
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[1].size(), 1);
    ASSERT_TRUE(pipeBiuTimeline.traceStartCache_.empty());
    ASSERT_TRUE(pipeBiuTimeline.unpairedTraces_.empty());
}

/**
 * | 用例集 | BiuTimelineTest
 * |测试函数| PipeBiuTimeline::ParseDfxRegion()
 * | 用例名 | test_ParseDfxRegion_trace_multiple_pairs
 * |用例描述| 测试多组不同traceId的start/end交错配对
 */
TEST_F(BiuTimelineTest, test_ParseDfxRegion_trace_multiple_pairs) {
    PipeBiuTimeline pipeBiuTimeline;
    // trace_start(1): 1025
    pipeBiuTimeline.channelCycleMap_[1] = 10;
    pipeBiuTimeline.ParseDfxRegion(1025, 1, "core0.veccore0", "VECTOR");
    // trace_start(2): 1026
    pipeBiuTimeline.channelCycleMap_[1] = 20;
    pipeBiuTimeline.ParseDfxRegion(1026, 1, "core0.veccore0", "VECTOR");
    // trace_end(1): 3073
    pipeBiuTimeline.channelCycleMap_[1] = 50;
    pipeBiuTimeline.ParseDfxRegion(3073, 1, "core0.veccore0", "VECTOR");
    // trace_end(2): 3074
    pipeBiuTimeline.channelCycleMap_[1] = 100;
    pipeBiuTimeline.ParseDfxRegion(3074, 1, "core0.veccore0", "VECTOR");

    auto &timelineVec = pipeBiuTimeline.timelineVec_[1];
    // 4个MarkStamp + 2个区间事件 = 6个事件
    ASSERT_EQ(timelineVec.size(), 6);

    // 事件1: MarkStamp1025 (trace_start(1))
    ASSERT_EQ(timelineVec[0].lineName, "MarkStamp1025");
    ASSERT_EQ(timelineVec[0].start, 10);
    // 事件2: MarkStamp1026 (trace_start(2))
    ASSERT_EQ(timelineVec[1].lineName, "MarkStamp1026");
    ASSERT_EQ(timelineVec[1].start, 20);
    // 事件3: MarkStamp3073 (trace_end(1))
    ASSERT_EQ(timelineVec[2].lineName, "MarkStamp3073");
    ASSERT_EQ(timelineVec[2].start, 50);
    // 事件4: Region1 (trace_end(1)配对成功后追加的区间事件)
    ASSERT_EQ(timelineVec[3].lineName, "Region1");
    ASSERT_EQ(timelineVec[3].start, 10);
    ASSERT_EQ(timelineVec[3].duration, 40);
    ASSERT_EQ(timelineVec[3].cName, "good");
    // 事件5: MarkStamp3074 (trace_end(2))
    ASSERT_EQ(timelineVec[4].lineName, "MarkStamp3074");
    ASSERT_EQ(timelineVec[4].start, 100);
    // 事件6: Region2 (trace_end(2)配对成功后追加的区间事件)
    ASSERT_EQ(timelineVec[5].lineName, "Region2");
    ASSERT_EQ(timelineVec[5].start, 20);
    ASSERT_EQ(timelineVec[5].duration, 80);
    ASSERT_EQ(timelineVec[5].cName, "thread_state_iowait");
    ASSERT_TRUE(pipeBiuTimeline.traceStartCache_.empty());
    ASSERT_TRUE(pipeBiuTimeline.unpairedTraces_.empty());
}

/**
 * | 用例集 | BiuTimelineTest
 * |测试函数| PipeBiuTimeline::ParseDfxRegion()
 * | 用例名 | test_ParseDfxRegion_trace_different_channels
 * |用例描述| 测试相同traceId在不同channel上独立配对
 */
TEST_F(BiuTimelineTest, test_ParseDfxRegion_trace_different_channels) {
    PipeBiuTimeline pipeBiuTimeline;
    // channel 1: trace_start(0)=1024, trace_end(0)=3072
    pipeBiuTimeline.channelCycleMap_[1] = 100;
    pipeBiuTimeline.ParseDfxRegion(1024, 1, "core0.veccore0", "VECTOR");
    pipeBiuTimeline.channelCycleMap_[1] = 200;
    pipeBiuTimeline.ParseDfxRegion(3072, 1, "core0.veccore0", "VECTOR");

    // channel 2: trace_start(0)=1024, trace_end(0)=3072
    pipeBiuTimeline.channelCycleMap_[2] = 300;
    pipeBiuTimeline.ParseDfxRegion(1024, 2, "core0.veccore1", "VECTOR");
    pipeBiuTimeline.channelCycleMap_[2] = 600;
    pipeBiuTimeline.ParseDfxRegion(3072, 2, "core0.veccore1", "VECTOR");

    // 两个channel各自生成3个事件: 2个MarkStamp + 1个区间事件
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[1].size(), 3);
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[2].size(), 3);

    // channel 1: 验证区间事件
    // 事件1: MarkStamp1024, 事件2: MarkStamp3072, 事件3: Region0
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[1][2].start, 100);
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[1][2].duration, 100);
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[1][2].cName, "thread_state_runnable"); // regionId=0, TOTAL_CNAME_MAP[0]

    // channel 2: 验证区间事件
    // 事件1: MarkStamp1024, 事件2: MarkStamp3072, 事件3: Region0
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[2][2].start, 300);
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[2][2].duration, 300);
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[2][2].cName, "thread_state_runnable"); // regionId=0, TOTAL_CNAME_MAP[0]
    ASSERT_TRUE(pipeBiuTimeline.traceStartCache_.empty());
    ASSERT_TRUE(pipeBiuTimeline.unpairedTraces_.empty());
}

/**
 * | 用例集 | BiuTimelineTest
 * |测试函数| PipeBiuTimeline::ParseDfxRegion()
 * | 用例名 | test_ParseDfxRegion_trace_and_markstamp_coexist
 * |用例描述| 测试trace打点与原有MarkStamp打点共存
 */
TEST_F(BiuTimelineTest, test_ParseDfxRegion_trace_and_markstamp_coexist) {
    PipeBiuTimeline pipeBiuTimeline;
    // MarkStamp: dfxRegionId=40 (bit10=0, 普通MarkStamp)
    pipeBiuTimeline.channelCycleMap_[1] = 50;
    pipeBiuTimeline.ParseDfxRegion(40, 1, "core0.veccore0", "VECTOR");

    // trace_start(5): 1029
    pipeBiuTimeline.channelCycleMap_[1] = 100;
    pipeBiuTimeline.ParseDfxRegion(1029, 1, "core0.veccore0", "VECTOR");

    // trace_end(5): 3077
    pipeBiuTimeline.channelCycleMap_[1] = 300;
    pipeBiuTimeline.ParseDfxRegion(3077, 1, "core0.veccore0", "VECTOR");

    auto &timelineVec = pipeBiuTimeline.timelineVec_[1];
    // 3个MarkStamp + 1个区间事件 = 4个事件
    ASSERT_EQ(timelineVec.size(), 4);

    // 事件1: MarkStamp40 (普通MarkStamp)
    ASSERT_EQ(timelineVec[0].lineName, "MarkStamp40");
    ASSERT_EQ(timelineVec[0].start, 50);
    ASSERT_EQ(timelineVec[0].duration, 1);
    ASSERT_TRUE(timelineVec[0].cName.empty()); // MarkStamp使用默认颜色

    // 事件2: MarkStamp1029 (trace_start的MarkStamp)
    ASSERT_EQ(timelineVec[1].lineName, "MarkStamp1029");
    ASSERT_EQ(timelineVec[1].start, 100);
    ASSERT_EQ(timelineVec[1].duration, 1);
    ASSERT_TRUE(timelineVec[1].cName.empty());

    // 事件3: MarkStamp3077 (trace_end的MarkStamp)
    ASSERT_EQ(timelineVec[2].lineName, "MarkStamp3077");
    ASSERT_EQ(timelineVec[2].start, 300);
    ASSERT_EQ(timelineVec[2].duration, 1);
    ASSERT_TRUE(timelineVec[2].cName.empty());

    // 事件4: Region5 (trace区间事件)
    ASSERT_EQ(timelineVec[3].lineName, "Region5");
    ASSERT_EQ(timelineVec[3].start, 100);
    ASSERT_EQ(timelineVec[3].duration, 200);
    ASSERT_EQ(timelineVec[3].cName, "thread_state_running"); // regionId=5, TOTAL_CNAME_MAP[5]
    ASSERT_TRUE(pipeBiuTimeline.traceStartCache_.empty());
    ASSERT_TRUE(pipeBiuTimeline.unpairedTraces_.empty());
}

/**
 * | 用例集 | BiuTimelineTest
 * |测试函数| PipeBiuTimeline::ParseSingleBiuTimeStamps()
 * | 用例名 | test_ParseSingleBiuTimeStamps_trace_timeline_success
 * |用例描述| 测试完整二进制解析流程，trace_start/trace_end通过timeline.bin正确配对
 */
TEST_F(BiuTimelineTest, test_ParseSingleBiuTimeStamps_trace_timeline_success) {
    // 构造测试数据: 1个trace配对 + 1个MarkStamp
    vector<char> totalBin;
    totalBin.resize(2097160, 0);

    InstrProfHeadInfo headInfo;
    headInfo.coreId = 0;
    headInfo.coreType = 1; // VECTOR0
    headInfo.validLen = 12; // 3个BiuPerfInfo × 4字节 = 12

    // 1. trace_start: ctrl=1(VECTOR), regionId=3, start
    //    dfxRegionId = 1024+3 = 1027, biuInfo = (1<<12)|1027 = 0x1403
    BiuPerfInfo traceStart;
    traceStart.cycles = 100;
    traceStart.biuInfo = 0x1403;

    // 2. MarkStamp: ctrl=1(VECTOR), dfxRegionId=20 (bit10=0)
    //    biuInfo = (1<<12)|20 = 0x1014
    BiuPerfInfo markStamp;
    markStamp.cycles = 50;
    markStamp.biuInfo = 0x1014;

    // 3. trace_end: ctrl=1(VECTOR), regionId=3, end
    //    dfxRegionId = 3072+3 = 3075, biuInfo = (1<<12)|3075 = 0x1C03
    BiuPerfInfo traceEnd;
    traceEnd.cycles = 200;
    traceEnd.biuInfo = 0x1C03;

    memcpy_s(&totalBin[0], sizeof(totalBin), &headInfo, sizeof(InstrProfHeadInfo));
    memcpy_s(&totalBin[8], sizeof(BiuPerfInfo), &traceStart, sizeof(BiuPerfInfo));
    memcpy_s(&totalBin[12], sizeof(BiuPerfInfo), &markStamp, sizeof(BiuPerfInfo));
    memcpy_s(&totalBin[16], sizeof(BiuPerfInfo), &traceEnd, sizeof(BiuPerfInfo));

    MOCKER(&IsReadable).stubs().will(returnValue(true));
    MOCKER(&GetFileSize).stubs().will(returnValue(size_t(2097160)));
    MOCKER(&ReadBinFileByMultiStruct).stubs().with(any(), any(), any(), outBound(totalBin)).will(returnValue(true));

    PipeBiuTimeline pipeBiuTimeline;
    ASSERT_TRUE(pipeBiuTimeline.ParseSingleBiuTimeStamps("test/ut/resources/"));

    // 所有数据来自同一文件(coreId=0, coreType=1)，channelId = 0*3+1 = 1
    auto &vecTimeline = pipeBiuTimeline.timelineVec_[1];
    // cycle累积: start=100, MarkStamp=150, end=350
    // 期望4条事件: 3个MarkStamp + 1个trace区间事件
    ASSERT_EQ(vecTimeline.size(), 4);

    // 事件1: MarkStamp1027 (trace_start的MarkStamp), start=100
    ASSERT_EQ(vecTimeline[0].lineName, "MarkStamp1027");
    ASSERT_EQ(vecTimeline[0].pipeName, "VECTOR");
    ASSERT_EQ(vecTimeline[0].start, 100);
    ASSERT_EQ(vecTimeline[0].duration, 1);
    ASSERT_TRUE(vecTimeline[0].cName.empty());

    // 事件2: MarkStamp20 (普通MarkStamp), start=150
    ASSERT_EQ(vecTimeline[1].lineName, "MarkStamp20");
    ASSERT_EQ(vecTimeline[1].pipeName, "VECTOR");
    ASSERT_EQ(vecTimeline[1].start, 150);
    ASSERT_EQ(vecTimeline[1].duration, 1);
    ASSERT_TRUE(vecTimeline[1].cName.empty());

    // 事件3: MarkStamp3075 (trace_end的MarkStamp), start=350
    ASSERT_EQ(vecTimeline[2].lineName, "MarkStamp3075");
    ASSERT_EQ(vecTimeline[2].pipeName, "VECTOR");
    ASSERT_EQ(vecTimeline[2].start, 350);
    ASSERT_EQ(vecTimeline[2].duration, 1);
    ASSERT_TRUE(vecTimeline[2].cName.empty());

    // 事件4: Region3 (trace区间事件), start=100, duration=250
    ASSERT_EQ(vecTimeline[3].lineName, "Region3");
    ASSERT_EQ(vecTimeline[3].pipeName, "VECTOR");
    ASSERT_EQ(vecTimeline[3].start, 100);
    ASSERT_EQ(vecTimeline[3].duration, 250);
    ASSERT_EQ(vecTimeline[3].cName, "thread_state_unknown"); // regionId=3, TOTAL_CNAME_MAP[3]
    ASSERT_TRUE(pipeBiuTimeline.traceStartCache_.empty());
    ASSERT_TRUE(pipeBiuTimeline.unpairedTraces_.empty());
}

/**
 * | 用例集 | BiuTimelineTest
 * |测试函数| PipeBiuTimeline::ParseDfxRegion() + PrintMissData()
 * | 用例名 | test_ParseDfxRegion_trace_mixed_paired_and_unpaired
 * |用例描述| 测试混合场景：既有配对成功的trace，也有未配对的start和end
 */
TEST_F(BiuTimelineTest, test_ParseDfxRegion_trace_mixed_paired_and_unpaired) {
    PipeBiuTimeline pipeBiuTimeline;

    // 1. trace_start(5) + trace_end(5): 配对成功，生成区间事件
    pipeBiuTimeline.channelCycleMap_[1] = 100;
    pipeBiuTimeline.ParseDfxRegion(1029, 1, "core0.veccore0", "VECTOR"); // trace_start(5)
    pipeBiuTimeline.channelCycleMap_[1] = 350;
    pipeBiuTimeline.ParseDfxRegion(3077, 1, "core0.veccore0", "VECTOR"); // trace_end(5)

    // 2. trace_end(8) without start: 未配对的end，只生成MarkStamp
    pipeBiuTimeline.channelCycleMap_[1] = 400;
    pipeBiuTimeline.ParseDfxRegion(3080, 1, "core0.veccore0", "VECTOR"); // trace_end(8)

    // 3. trace_start(9) without end: 未配对的start，只生成MarkStamp，缓存保留
    pipeBiuTimeline.channelCycleMap_[1] = 500;
    pipeBiuTimeline.ParseDfxRegion(1033, 1, "core0.veccore0", "VECTOR"); // trace_start(9)

    // 4. 验证当前状态
    // 4个MarkStamp + 1个区间事件 = 5个事件
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[1].size(), 5);
    ASSERT_EQ(pipeBiuTimeline.traceStartCache_.size(), 1); // 1个未配对的start
    ASSERT_EQ(pipeBiuTimeline.unpairedTraces_.size(), 1); // 1个未配对的end

    // 5. 调用PrintMissData，每个regionId打印一条debug日志，不生成新事件
    pipeBiuTimeline.PrintMissData();

    // 6. 验证最终结果：事件数量不变，缓存清空
    ASSERT_EQ(pipeBiuTimeline.timelineVec_[1].size(), 5);
    ASSERT_TRUE(pipeBiuTimeline.traceStartCache_.empty());
    ASSERT_TRUE(pipeBiuTimeline.unpairedTraces_.empty());

    // 验证事件内容
    auto &vec = pipeBiuTimeline.timelineVec_[1];
    // 事件1: MarkStamp1029 (trace_start(5))
    ASSERT_EQ(vec[0].lineName, "MarkStamp1029");
    // 事件2: MarkStamp3077 (trace_end(5))
    ASSERT_EQ(vec[1].lineName, "MarkStamp3077");
    // 事件3: Region5 (trace区间事件)
    ASSERT_EQ(vec[2].lineName, "Region5");
    ASSERT_EQ(vec[2].start, 100);
    ASSERT_EQ(vec[2].duration, 250);
    // 事件4: MarkStamp3080 (trace_end(8))
    ASSERT_EQ(vec[3].lineName, "MarkStamp3080");
    // 事件5: MarkStamp1033 (trace_start(9))
    ASSERT_EQ(vec[4].lineName, "MarkStamp1033");
}

/**
 * | 用例集 | BiuTimelineTest
 * |测试函数| PipeBiuTimeline::ParseDfxRegion()
 * | 用例名 | test_ParseDfxRegion_trace_duplicate_start
 * |用例描述| 测试重复TRACE_START覆盖前一条，验证覆盖后配对使用新startCycle
 */
TEST_F(BiuTimelineTest, test_ParseDfxRegion_trace_duplicate_start) {
    PipeBiuTimeline pipeBiuTimeline;
    // 第一次 trace_start(5): cycle=100
    pipeBiuTimeline.channelCycleMap_[1] = 100;
    pipeBiuTimeline.ParseDfxRegion(1029, 1, "core0.veccore0", "VECTOR");
    // 第二次 trace_start(5): cycle=200，覆盖前一条
    pipeBiuTimeline.channelCycleMap_[1] = 200;
    pipeBiuTimeline.ParseDfxRegion(1029, 1, "core0.veccore0", "VECTOR");
    // trace_end(5): cycle=350
    pipeBiuTimeline.channelCycleMap_[1] = 350;
    pipeBiuTimeline.ParseDfxRegion(3077, 1, "core0.veccore0", "VECTOR");

    auto &vec = pipeBiuTimeline.timelineVec_[1];
    // 3个MarkStamp + 1个区间事件 = 4个事件
    ASSERT_EQ(vec.size(), 4);
    // Region5 使用第二次start(200)，duration=350-200=150
    ASSERT_EQ(vec[3].lineName, "Region5");
    ASSERT_EQ(vec[3].start, 200);
    ASSERT_EQ(vec[3].duration, 150);
    ASSERT_TRUE(pipeBiuTimeline.traceStartCache_.empty());
    // 重复start被归入unpairedTraces_统一处理
    ASSERT_EQ(pipeBiuTimeline.unpairedTraces_.size(), 1);
    ASSERT_EQ(*pipeBiuTimeline.unpairedTraces_.begin(), 5);
}
