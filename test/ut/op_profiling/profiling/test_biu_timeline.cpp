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
        biuPerfInfo4.biuInfo = 0xa60f;
        BiuPerfInfo biuPerfInfo5;
        biuPerfInfo5.cycles = 50;
        biuPerfInfo5.biuInfo = 0xae0f;
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
    ASSERT_EQ(timelineVec[2].lineName, "MarkStamp1551");
    ASSERT_EQ(timelineVec[2].start, 100);
    ASSERT_EQ(timelineVec[2].duration, 1);
    ASSERT_EQ(timelineVec[3].pipeName, "FIXP");
    ASSERT_EQ(timelineVec[3].coreName, "core0.veccore0");
    ASSERT_EQ(timelineVec[3].lineName, "MarkStamp3599");
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
    ASSERT_EQ(timelineVec[0].lineName, "MarkStamp1551");
    ASSERT_EQ(timelineVec[0].start, 100);
    ASSERT_EQ(timelineVec[0].duration, 50);
}

/**
/* | 用例集 | BiuTimelineTest
/* |测试函数| InstrBiuTimeline::PrintPipeIdFull()
/* | 用例名 | test_PrintPipeIdFull_parse_log_success
/* |用例描述| 测试PrintPipeIdFull，解析tune日志正常，id达到512以上的pipe日志提示
*/
TEST_F(BiuTimelineTest, test_PrintPipeIdFull_parse_log_success)
{
    ofstream file("test/ut/resources/dump/dfx_tune.log");
    ASSERT_TRUE(file.is_open());
    file << "TUNE-ERROR:all dfx ids consumed[pipe=mte2]" << endl;
    file << "TUNE-ERROR:all dfx ids consumed[pipe=mte3]" << endl;
    file << "TUNE-ERROR:all dfx ids consumed[pipe=cube]" << endl;
    file.close();
    InstrBiuTimeline instrBiuTimeline;
    instrBiuTimeline.outputPath_ = "test/ut/resources";
    testing::internal::CaptureStdout();
    instrBiuTimeline.PrintPipeIdFull();
    std::string capture = testing::internal::GetCapturedStdout();
    EXPECT_NE(capture.find("InstrTimeline of pipes[cube, mte2, mte3] is incomplete because these pipes have more than 512 instructions."), std::string::npos);
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
