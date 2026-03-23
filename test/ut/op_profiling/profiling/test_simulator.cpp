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

#include <unistd.h>
#include <sys/stat.h>
#include <gtest/gtest.h>
#include <experimental/filesystem>
#include "mockcpp/mockcpp.hpp"

#include "common/prof_args.h"
#include "ascend_helper.h"
#include "common/defs.h"
#include "filesystem.h"
#include "json_parser.h"
#include "cmd_execute.h"
#define private public
#define protected public
#include "profiling/simulator/data_parse/sim_data_parse.h"
#include "profiling/simulator/data_parse/sim_defs.h"
#include "profiling/simulator/data_parse/sim_dump_parser_task.h"
#include "profiling/op_prof_data_parse.h"
#include "profiling/simulator/data_parse/sim_common_statistic.h"
#include "profiling/simulator/run/simulator_task.h"
#include "profiling/simulator/op_sim_prof.h"
#include "profiling/simulator/data_parse/api_data.h"
#undef private
#undef protected
#include "op_runner.h"

using namespace Profiling;
using namespace Utility;
using namespace Common;
using namespace Serialization;
using namespace std;
using json = nlohmann::json;

string GOLDEN_INSTR_LOG_910B_LOGICID = "[info] [00001679] (PC: 0x126cf340) SCALAR   : (Binary: 0x02dc0880) MOV_XD_SPR  XD:X14=0x1, SPR:SUBBLOCKID,\n"
                                       "[info] [00001623] (PC: 0x126cf2dc) SCALAR   : (Binary: 0x02161880) MOV_XD_SPR  XD:X11=0, SPR:BLOCKID, ";

string GOLDEN_INSTR_LOG = "[info] [00000254] (PC: 0x10cfa000) SCALAR   : (Binary: 0x07000001) MOV_XD_IMM  XD:X0=0x1, IMM:0x1,\n"
                          "[info] [00000258] (PC: 0x10cfa008) VEC      : (Binary: 0x80400000) MOVEMASK  dtype:B16 XD:0xffffffffffffffff\n"
                          "[info] [00000545] (PC: 0x10cfa058) MTE2     : (Binary: 0x74401100) DMA_MOV  Src:OUT, Dst:UB, XD:X0=0\n"
                          "[info] [00000181](PC: 0x10ce8000) SCALAR : (Binary: 0x07000001) scalar_mov_xd_imme16(x[0]= 0x1, imme16:0x1 )\n"
                          "[info] [00000182](PC: 0x10ce8004) SCALAR : (Binary: 0x02000080) scalar_neg(dType:0x0,  x[0]= 0xffffffffffffffff, xn:0x0)\n"
                          "[info] [00000184](PC: 0x10ce8010) SCALAR : (Binary: 0x02003880) scalar_mov_xd_special( x[0]= 0xc, SPR_CTL= 0xc,)";

string GOLDEN_INSTR_POPPED_LOG = "[info] [00000253] (PC: 0x10cfa000) SCALAR   : (Binary: 0x07000001) MOV_XD_IMM  XD:X0=0, IMM:0x1,\n"
                                 "[info] [00000256] (PC: 0x10cfa008) VEC      : (Binary: 0x80400000) MOVEMASK  dtype:B16 XD:0xffffffffffffffff\n"
                                 "[info] [00000276] (PC: 0x10cfa058) MTE2     : (Binary: 0x74401100) DMA_MOV  Src:OUT, Dst:UB, XD:X0=0\n"
                                 "[info] [00000181] (PC: 0x10ce8000) SCALAR   : (Binary: 0x07000001) scalar_mov_xd_imme16(x[0]= 0x0, imme16:0x1 )\n"
                                 "[00000182](PC: 0x10ce8004) SCALAR : (Binary: 0x02000080) scalar_neg(dType:0x0,  x[0]= 0x0, xn:0x0)\n"
                                 "[00000184](PC: 0x10ce8008) VECTOR : (Binary: 0x80400000) MOVEMASK  XN:X0=0xffffffffffffffff, Id:415";

string GOLDEN_ICACHE_LOG = "[info] [00000001]: icache read address is 0x10cfa000, size is 0x00000010, status is MISS\n"
                           "[info] [00000002]: icache refill request, id is 0x00000003, address is 0x10cfa000\n"
                           "[info] [00000250]: icache refill acknowledge, id is 0x00000003, address is 0x10cfa000\n"
                           "[info] [00000000]: icache read address is 0x10ce8000, size is 0x00000010, status is MISS";

string GOLDEN_REG_LOG = "[info] [00276009]: write spr: name = PC, val = 0x119cf708\n"
                        "[info] [00276009]: write spr: name = PC, val = 0x119cf70c\n";

string GOLDEN_INSTR_LOG_310P =
        "[info] [00000342](PC: 0x10cfa008) SCALAR : (Binary: 0x02000080) scalar_neg(dType:0x0,  x[0]= 0xffffffffffffffff, xn:0x0)  \n"
        "[info] [00000343](PC: 0x10cfa00c) SCALAR : (Binary: 0x07430004) scalar_movk_xd_imme16(x[1]= 0x44000, imme16:0x4 ) \n"
        "[info] [00000344](PC: 0x10cfa010) SCALAR : (Binary: 0x04c01fd8) scalar_st_imm(dType:0x3,  x[0]= 0xffffffffffffffff, x[1]= 0x44000, imm12= 0xfd8, post = 0x0) start_cycle(343) end_cycle(346) \n"
        "[info] [00000350](PC: 0x10cfa03c) MTE2 : (Binary: 0x74401100) mov_out_to_ub(xd:0, xn:1, xm:2, xdValue:0x0, xnValue:0x1068be00, xmValue:0x10012, srcIDValue:2, destIDValue:1, reluValue:0, padValue:0), instr ID is: 1 \n"
        "[info] [00000354](PC: 0x10cfa468) MTE2 : (Binary: 0x74615180) mov_out_to_ub(xd:16, xn:21, xm:3, xdValue:0x0, xnValue:0x10ce1200, xmValue:0x80010, srcIDValue:2, destIDValue:1, reluValue:0, padValue:0), instr ID is: 2 \n"
        "[info] [00000355](PC: 0x10cfa024) SCALAR : (Binary: 0x077b0004) scalar_movk_xd_imme16(x[29]= 0x43fb8, imme16:0x4 ) \n"
        "[info] [00000502](PC: 0x10cfa2f8) SCALAR ISSUE : (Binary: 0x40a20098) set_event(pipe_type: SCALAR, tigger_pipe: VECTOR, event_id: 0) \n"
        "[info] [00000504](PC: 0x10cfa308) VEC0 IB ISSUE : (Binary: 0x40c20098) wait_event(pipe_type: SCALAR, tigger_pipe: VECTOR, event_id: 0) ";

string GOLDEN_INSTR_POPPED_LOG_310P =
        "[info] [00000341](PC: 0x10cfa008) SCALAR : (Binary: 0x02000080) scalar_neg(dType:0x0,  x[0]= 0x0, xn:0x0) \n"
        "[info] [00000342](PC: 0x10cfa00c) SCALAR : (Binary: 0x07430004) scalar_movk_xd_imme16(x[1]= 0x4000, imme16:0x4 ) \n"
        "[info] [00000343](PC: 0x10cfa010) SCALAR : (Binary: 0x04c01fd8) scalar_st_imm(dType:0x3,  x[0]= 0xffffffffffffffff, x[1]= 0x44000, imm12= 0xfd8, post = 0x0) \n"
        "[info] [00000344](PC: 0x10cfa03c) MTE2 : (Binary: 0x74401100) mov_out_to_ub(xd:0, xn:1, xm:2, xdValue:0x0, xnValue:0x1068be00, xmValue:0x10012, srcIDValue:2, destIDValue:1, reluValue:0, padValue:0), instr ID is: 1. poped from IQ \n"
        "[info] [00000344](PC: 0x10cfa468) MTE2 : (Binary: 0x74615180) mov_out_to_ub(xd:16, xn:21, xm:3, xdValue:0x0, xnValue:0x10ce1200, xmValue:0x80010, srcIDValue:2, destIDValue:1, reluValue:0, padValue:0), instr ID is: 2. pop success 2 \n"
        "[info] [00000345](PC: 0x10cfa024) SCALAR : (Binary: 0x077b0004) scalar_movk_xd_imme16(x[29]= 0x3fb8, imme16:0x4 ) \n"
        "[info] [00000500](PC: 0x10cfa2f8) SCALAR ISSUE : (Binary: 0x40a20098) set_event(pipe_type: SCALAR, tigger_pipe: VECTOR, event_id: 0) \n"
        "[info] [00000504](PC: 0x10cfa308) VEC0 IB ISSUE : (Binary: 0x40c20098) wait_event(pipe_type: SCALAR, tigger_pipe: VECTOR, event_id: 0) ";

string GOLDEN_ICACHE_LOG_310P =
        "[info] [00000000]: icache prefetch data is 0x10cfa180, id is 0x4\n"
        "[info] [00000000]: icache read address is 0x10cfa000, size is 0x00000010, status is MISS\n"
        "[info] [00000308]: icache read address is 0x10cfa010, size is 0x00000010, status is MISS\n"
        "[info] [00000318]: icache refill request, req_type:00000001, req_id_ is 0x1, ll_req_id_ is 0x0000000b, address is 0x10cfa480, buf_id is 0x3\n"
        "[info] [00000319]: icache read address is 0x10cfa020, size is 0x00000010, status is MISS";

string GOLDEN_REG_LOG_310P = "[info] tick: 0, write spr reg: SPR_PC, 8, value: 10cfa000\n"
                             "[info] tick: 339, write spr reg: SPR_PC, 8, value: 10cfa004";

std::vector<Serialization::Instr> INSTR_VEC_310P = {
        // starts, end, pc, pipe, name, detail
        {782,  788,  1, 20, 1, 1, 1., 0, 0, -1, -1, "0x10cfa000", "SCALAR", "scalar_mov_xd_imme16", "x[0]=0x0,imme16:0x1"},
        {779,  1112, 1, 20, 1, 1, 1., 0, 0, -1, -1, "0x10cfa008", "SCALAR", "wait_event",           "pipe_type: L2,tigger_pipe: SCALAR,event_id: 0"},
        {787,  1112, 1, 20, 1, 1, 1., 0, 0, -1, -1, "0x10cfa012", "MTE2",   "set_event",            "pipe_type: L2,tigger_pipe: SCALAR,event_id: 0"},
        {1822, 1822, 1, 20, 1, 1, 1., 0, 0, -1, -1, "0x10cfa850", "VECTOR", "set_event",            "pipe_type: VECTOR, tigger_pipe: L3, event_id: 0"},
        {1840, 1840, 1, 20, 1, 1, 1., 0, 0, -1, -1, "0x10cfa394", "MTE3",   "wait_event",           "pipe_type: VECTOR, tigger_pipe: L3, event_id: 0"},
};

string GOLDEN_INSTR_LOG_310B =
        "[info] [00000342](PC: 0x10cfa008) SCALAR : (Binary: 0x02000080) (ID: 000731) scalar_neg(dType:0x0,  x[0]= 0xffffffffffffffff, xn:0x0)  \n"
        "[info] [00000343](PC: 0x10cfa00c) SCALAR : (Binary: 0x07430004) (ID: 000732) scalar_movk_xd_imme16(x[1]= 0x44000, imme16:0x4 ) \n"
        "[info] [00000344](PC: 0x10cfa010) SCALAR : (Binary: 0x04c01fd8) (ID: 000733) scalar_st_imm(dType:0x3,  x[0]= 0xffffffffffffffff, x[1]= 0x44000, imm12= 0xfd8, post = 0x0) start_cycle(343) end_cycle(346) \n"
        "[info] [00000350](PC: 0x10cfa03c) MTE2 : (Binary: 0x74401100) (ID: 000734) mov_out_to_ub(xd:0, xn:1, xm:2, xdValue:0x0, xnValue:0x1068be00, xmValue:0x10012, srcIDValue:2, destIDValue:1, reluValue:0, padValue:0), instr ID is: 1 \n"
        "[info] [00000354](PC: 0x10cfa468) MTE2 : (Binary: 0x74615180) (ID: 000735) mov_out_to_ub(xd:16, xn:21, xm:3, xdValue:0x0, xnValue:0x10ce1200, xmValue:0x80010, srcIDValue:2, destIDValue:1, reluValue:0, padValue:0), instr ID is: 2 \n"
        "[info] [00000355](PC: 0x10cfa024) SCALAR : (Binary: 0x077b0004) (ID: 000736) scalar_movk_xd_imme16(x[29]= 0x43fb8, imme16:0x4 ) ";

string GOLDEN_INSTR_POPPED_LOG_310B =
        "[info] [00000341](PC: 0x10cfa008) SCALAR : (Binary: 0x02000080) (ID: 000731) scalar_neg(dType:0x0,  x[0]= 0x0, xn:0x0) \n"
        "[info] [00000342](PC: 0x10cfa00c) SCALAR : (Binary: 0x07430004) (ID: 000732) scalar_movk_xd_imme16(x[1]= 0x4000, imme16:0x4 ) \n"
        "[info] [00000343](PC: 0x10cfa010) SCALAR : (Binary: 0x04c01fd8) (ID: 000733) scalar_st_imm(dType:0x3,  x[0]= 0xffffffffffffffff, x[1]= 0x44000, imm12= 0xfd8, post = 0x0) \n"
        "[info] [00000344](PC: 0x10cfa03c) MTE2 : (Binary: 0x74401100) (ID: 000734) mov_out_to_ub(xd:0, xn:1, xm:2, xdValue:0x0, xnValue:0x1068be00, xmValue:0x10012, srcIDValue:2, destIDValue:1, reluValue:0, padValue:0), instr ID is: 1. poped from IQ \n"
        "[info] [00000344](PC: 0x10cfa468) MTE2 : (Binary: 0x74615180) (ID: 000735) mov_out_to_ub(xd:16, xn:21, xm:3, xdValue:0x0, xnValue:0x10ce1200, xmValue:0x80010, srcIDValue:2, destIDValue:1, reluValue:0, padValue:0), instr ID is: 2. pop success 2 \n"
        "[info] [00000345](PC: 0x10cfa024) SCALAR : (Binary: 0x077b0004) (ID: 000736) scalar_movk_xd_imme16(x[29]= 0x3fb8, imme16:0x4 ) ";

string GOLDEN_ICACHE_LOG_310B =
        "[info] @2354: icache read address is 0x13dbf0f0, size is 0x00000010, status is HIT\n"
        "[info] @2354: hit pc send to ibuffer,pc:0x13dbf0f0,gid:2a3.\n"
        "[info] @2355: read icache is hl_port_req.req addr is 0x13dbf100.\n"
        "[info] @2355: lookup_tag is fetch req addr:0x13dbf100\n"
        "[info] @2355: icache read address is 0x10cfa100, size is 0x00000100, status is MISS\n"
        "[info] 2355: icache fetch request, id is 0x000002a5, address is 0x13dbf100";

string GOLDEN_REG_LOG_310B = "[info] [00276009]: write spr: name = PC, val = 0x10cfa000\n"
                             "[info] [00276009]: write spr: name = PC, val = 0x10cfa004\n";

std::vector<IcacheParseInfo> valueIcache = {
        {00000001, 0x10ce8000, "id is 0x00000001, address is 0x10ce8000"},
        {00000002, 0x10ce8000, "id is 0x00000002, address is 0x10ce8000"}
};
std::vector<InstrParseInfo> valueInstr = {
        {00000227, 0x10ce8000, 0, -1, -1, "SCALAR", "scalar_mov_xd_imme16", "x[0]= 0x0, imme16:0x1"}
};

std::string GOLDEN_LLVM_SYMBOLIZER_STD = "[{\"Address\":\"0x0\",""\"ModuleName\":\"/home/test/project/msopt_py/build/output/profiling/test/test.o\","
                                         "\"Symbol\":[{\"Column\":0,\"Discriminator\":0,\"FileName\":\"/home/test/project/add_custom/add_custom.cpp\",""\"FunctionName\":\"add_custom\",\"Line\":99,\"StartAddress\":\"0x0\",\"StartFileName\":\"/home/test/project/add_custom/add_custom.cpp\",\"StartLine\":98}]},"
                                         "{\"Address\":\"0x8\",\"ModuleName\":\"/home/test/project/msopt_py/build/output/profiling/test/test.o\","
                                         "\"Symbol\":[{\"Column\":0,\"Discriminator\":0,\"FileName\":\"/home/test/project/add_custom/add_custom.cpp\",\"FunctionName\":\"add_custom\",\"Line\":99,\"StartAddress\":\"0x0\",\"StartFileName\":\"/home/test/project/add_custom/add_custom.cpp\",""\"StartLine\":98}]},"
                                         "{\"Address\":\"0x58\",\"ModuleName\":\"/home/test/project/msopt_py/build/output/profiling/test/test.o\","
                                         "\"Symbol\":[{\"Column\":0,\"Discriminator\":0,\"FileName\":\"/home/test/project/add_custom/add_custom.cpp\",""\"FunctionName\":\"add_custom\",\"Line\":99,\"StartAddress\":\"0x0\",\"StartFileName\":\"/home/test/project/add_custom/add_custom.cpp\",\"StartLine\":98}]}]";
TEST(SimulatorTask, Run_with_success)
{
    GlobalMockObject::verify();
    MOCKER(&IsExist)
            .stubs()
            .will(returnValue(false));
    MOCKER(&MkdirRecusively)
            .stubs()
            .will(returnValue(true));
    MOCKER(&OpRunner::RunOpBinary)
            .stubs()
            .will(returnValue(true));
    MOCKER(&SimulatorTask::CheckSimulatorSoExist)
            .stubs()
            .will(returnValue(true));
    MOCKER(&Utility::GetAscendHomePath)
            .stubs()
            .will(returnValue(true));
    MOCKER(&Profiling::Task::IsSetSocVersion)
            .stubs()
            .will(returnValue(true));
    Common::ProfArgs args;
    args.argSocVersion = "Ascend910B1";
    Profiling::OpSimProf opSimProf(args);
    SimulatorTask simulatorTask("test", opSimProf);
    ASSERT_TRUE(simulatorTask.Run());
    ASSERT_TRUE(simulatorTask.GetExecutionStatus() == ExecStatus::STOPPED);
}

TEST(SimulatorTask, Run_with_argDump_on_success)
{
    GlobalMockObject::verify();
    MOCKER(&IsExist)
            .stubs()
            .will(returnValue(false));
    MOCKER(&MkdirRecusively)
            .stubs()
            .will(returnValue(true));
    MOCKER(&OpRunner::RunOpBinary)
            .stubs()
            .will(returnValue(true));
    MOCKER(&SimulatorTask::CheckSimulatorSoExist)
            .stubs()
            .will(returnValue(true));
    MOCKER(&Utility::GetAscendHomePath)
            .stubs()
            .will(returnValue(true));
    MOCKER(&Profiling::Task::IsSetSocVersion)
            .stubs()
            .will(returnValue(true));
    Common::ProfArgs args;
    args.argSocVersion = "Ascend910B1";
    args.runMode = "simulator";
    args.argDump = "off";
    Profiling::OpSimProf opSimProf(args);
    SimulatorTask simulatorTask("test", opSimProf);
    ASSERT_TRUE(simulatorTask.Run());
    ASSERT_TRUE(simulatorTask.GetExecutionStatus() == ExecStatus::STOPPED);
}

TEST(SimulatorTask, Run_with_faild)
{
    GlobalMockObject::verify();
    MOCKER(&IsExist)
            .stubs()
            .will(returnValue(false))
            .then(returnValue(true));
    MOCKER(&IsDir)
            .stubs()
            .will(returnValue(false));
    MOCKER(&MkdirRecusively)
            .stubs()
            .will(returnValue(false));
    MOCKER(&OpRunner::RunOpBinary)
            .stubs()
            .will(returnValue(false));
    Common::ProfArgs args;
    Profiling::OpSimProf opSimProf(args);
    SimulatorTask simulatorTask("test", opSimProf);
    ASSERT_FALSE(simulatorTask.Run());
}

TEST(SimDataParse, Execute_with_mutil_failed)
{
    GlobalMockObject::verify();
    MOCKER(&Utility::GetAscendHomePath)
            .stubs()
            .will(returnValue(true));
    std::string path;
    // OPPO/device0/kernel0/0/dump
    std::string testSimDir = "test/ut/resources/op_profiling/simulator/OPPO1";
    std::string deviceIdDir = JoinPath({testSimDir, "device0"});
    auto dumpDir1 = JoinPath({testSimDir, "device0/add/0/dump"});
    auto dumpDir2 = JoinPath({testSimDir, "device0/abs/0/dump"});
    // OPPO/device0/tmp_dump
    auto tempDir = JoinPath({testSimDir, "tmp_dump"});
    auto tempkernameDir = JoinPath({tempDir, "object_dump.txt"});
    MkdirRecusively(dumpDir1);
    MkdirRecusively(dumpDir2);
    MkdirRecusively(tempDir);
    ProfArgs config;
    config.argOutput = testSimDir;
    SimDataParse simAnalysis("Ascend910B1", "", "", config.argAicMetrics, true);
    ASSERT_FALSE(simAnalysis.Execute(testSimDir));
    ASSERT_EQ(simAnalysis.failedKernelNum_, 2);
    std::experimental::filesystem::remove_all(testSimDir);
    GlobalMockObject::verify();
}

TEST(SimDataParse, rename_csv)
{
    GlobalMockObject::verify();
    MOCKER(&GetAscendHomePath)
            .stubs()
            .will(returnValue(true));

    std::string testSimDir = "test/ut/resources/op_profiling/simulator_test/OPPO/Add/0";
    MkdirRecusively(testSimDir + "/core0.veccore0");

    std::string csvName = JoinPath({testSimDir, "core0.veccore0/core0.veccore0_instr_exe_20240910175706302.csv"});
    WriteFileByStream(csvName, "aaaa");
    ProfArgs config;
    std::string temp;
    SimDataParse simAnalysis("Ascend910B1", "", "", config.argAicMetrics);
    simAnalysis.RenameCsvFileAndModifyKernelData(csvName, temp);
    std::vector<std::string> fileNames;
    Utility::GetFileNames(testSimDir + "/core0.veccore0", fileNames);
    printf("fileNames[0]: %s", fileNames[0].c_str());
    ASSERT_TRUE(fileNames[0] == "core0.veccore0_instr_exe.csv");
    std::experimental::filesystem::remove_all(testSimDir);
}

TEST(SimDataParse, Execute_with_single)
{
    std::string path;

    // OPPO/device0/kernel0/0/dump
    std::string testSimDir = "test/ut/resources/op_profiling/simulator/OPPO1";
    std::string deviceIdDir = JoinPath({testSimDir, "device0"});
    auto dumpDir1 = JoinPath({testSimDir, "device0/add/0/dump"});
    // OPPO/device0/tmp_dump
    auto tempDir = JoinPath({testSimDir, "tmp_dump"});
    auto tempkernameDir = JoinPath({tempDir, "object_dump.txt"});
    MkdirRecusively(dumpDir1);
    MkdirRecusively(tempDir);
    ProfArgs config;
    config.argOutput = testSimDir;
    SimDataParse simAnalysis("Ascend910B1", "", "", config.argAicMetrics, true);
    ASSERT_FALSE(simAnalysis.Execute(testSimDir));
    ASSERT_TRUE(simAnalysis.failedKernelNum_ == 1);
    std::experimental::filesystem::remove_all(testSimDir);
}

TEST(SimDataParse, GetDumpPathTrue)
{
    ProfArgs config;
    std::string path;
    std::string testSimDir = "test/ut/resources/op_profiling/simulator";
    ParseInfoStruct parseInfoStruct;
    auto dumpDir = JoinPath({testSimDir, "dump"});
    auto tempDir = JoinPath({testSimDir, "tmp_dump"});
    auto tempkernameDir = JoinPath({tempDir, "object_dump.txt"});
    MkdirRecusively(dumpDir);
    MkdirRecusively(tempDir);
    config.argOutput = testSimDir;
    string kernel = "111-111";
    WriteFileByStream(tempkernameDir, kernel);
    Profiling::DataParse::inExitMode = true;
    SimDataParse simAnalysis("Ascend910B1", "", "", config.argAicMetrics);
    simAnalysis.DisposeTmp(dumpDir);
    EXPECT_TRUE(IsExist(tempDir));
    Profiling::DataParse::inExitMode = false;
    std::experimental::filesystem::remove_all(tempDir);
    std::experimental::filesystem::remove_all(dumpDir);
}

TEST(SimDataParse, GetDumpPathFalse)
{
    ProfArgs config;
    std::string path;
    std::string testSimDir = "test/ut/resources/op_profiling/simulator";
    ParseInfoStruct parseInfoStruct;
    auto dumpDir = JoinPath({testSimDir, "dump"});
    auto tempDir = JoinPath({testSimDir, "tmp_dump"});
    MkdirRecusively(dumpDir);
    MkdirRecusively(tempDir);
    config.argOutput = testSimDir;
    Profiling::DataParse::inExitMode = true;
    SimDataParse simAnalysis("Ascend910B1", "", "", config.argAicMetrics);
    simAnalysis.DisposeTmp(testSimDir);
    EXPECT_FALSE(IsExist(tempDir));
    Profiling::DataParse::inExitMode = false;
    std::experimental::filesystem::remove_all(tempDir);
    std::experimental::filesystem::remove_all(dumpDir);
}

TEST(SimDataParse, IsObjectKernal)
{
    ProfArgs config;
    std::string path;
    std::string path2 = "";
    SimDataParse simAnalysis("Ascend910B1", "", "", config.argAicMetrics);
    simAnalysis.HandleDumpLog(path2);
    path2 = "222";
    simAnalysis.HandleDumpLog(path2);
    string p;
    ASSERT_FALSE(simAnalysis.GetObjectOutPathAndCopyAicoreFile(path2, p));
}

TEST(SimDataParse, CreateConfigFile_RETURN_FALSE)
{
    Common::ProfArgs args;
    Profiling::OpSimProf opSimProf(args);
    SimulatorTask simulatorTask("test", opSimProf);
    simulatorTask.camodelLibDir_ = "test/ut/resources/op_profiling/device910B/dump";
    simulatorTask.tmpPath_ = "test/ut/resources/op_profiling/device910B/dump";
    map<string, string> replaceFileStr;
    replaceFileStr["00000"] = "11111";
    std::string fileName1 = "xxx";
    ASSERT_FALSE(simulatorTask.CreateConfigFile(fileName1, replaceFileStr));
    std::string fileName2 = "op_basic_info.txt";
    replaceFileStr["Op Name=abs"] = "Op Name=sub";
    ASSERT_FALSE(simulatorTask.CreateConfigFile(fileName2, replaceFileStr));
}

TEST(SimDataParse, CreateTaskDir_RETURN_FALSE)
{
    Common::ProfArgs args;
    Profiling::OpSimProf opSimProf(args);
    SimulatorTask simulatorTask("test", opSimProf);
    std::string path = "test/ut/resources/op_profiling/device910B/dump/op_basic_info.txt";
    ASSERT_FALSE(simulatorTask.CreateTaskDir(path));
}

TEST(SimDataParse, GetObjectOutPathAndCopyAicoreFile)
{
    ProfArgs config;
    GlobalMockObject::verify();
    SimDataParse simAnalysis("Ascend910B1", "", "", config.argAicMetrics);
    std::string dumpDir = "test/ut/resources/op_profiling/simulator/dump";
    ASSERT_TRUE(MkdirRecusively(dumpDir));
    std::string file = "test/ut/resources/op_profiling/simulator/dump/object_dump.txt";
    std::string content = "test/ut/resources/op_profiling/simulator/dump/add";
    std::string outPath;
    ofstream out1(file, ios_base::out | ios_base::trunc);
    out1 << "test/ut/resources/op_profiling/simulator/dump/object_dump.txt" << std::endl;
    out1 << "test/ut/resources/op_profiling/simulator/dump/object_dump.txt" << std::endl;
    ASSERT_TRUE(simAnalysis.GetObjectOutPathAndCopyAicoreFile(dumpDir, outPath));
    out1.close();
    std::experimental::filesystem::remove_all(file);

    ofstream out2(file, ios_base::out | ios_base::trunc);
    out2 << "test/ut/resources/op_profiling/simulator/dump/object_dump.txt/object_dump.txt" << std::endl;
    out2 << "test/ut/resources/op_profiling/simulator/dump/object_dump.txt/object_dump.txt" << std::endl;
    MOCKER(&MkdirRecusively)
            .stubs()
            .will(returnValue(false));
    EXPECT_FALSE(simAnalysis.GetObjectOutPathAndCopyAicoreFile(dumpDir, outPath));
    out2.close();
    std::experimental::filesystem::remove_all(file);

    ofstream out3(file, ios_base::out | ios_base::trunc);
    out3 << "" << std::endl;
    out3 << "test/ut/resources/op_profiling/simulator/dump/object_dump.txt/object_dump.txt" << std::endl;
    EXPECT_FALSE(simAnalysis.GetObjectOutPathAndCopyAicoreFile(dumpDir, outPath));
    out3.close();
    std::experimental::filesystem::remove_all(dumpDir);
    GlobalMockObject::verify();
}

TEST(SymbolizerParser, GetSymbolizerCount_with_success)
{
    SymbolizerParser symbolizerParser;
    for (int i = 0;i <1000;i++)
    {
        symbolizerParser.pcOffsets_.push_back("1");
    }
    size_t count = symbolizerParser.GetSymbolizerCount();
    ASSERT_EQ(count, 30000);
}

TEST(SymbolizerParser, ParseAddr2Offset_with_success)
{
    SymbolizerParser symbolizerParser;
    std::string jsonContext = "[{\"Address\":\"0x0\",\"ModuleName\":\"./aicore_binary.o\",\"Symbol\":[{\"Column\":0,\"Discriminator\":0,\"FileName\":\"/IncreFlashAttention.cpp\",\"FunctionName\":\"IncreFlashAttention_mix_aic\",\"Line\":23,\"StartAddress\":\"0x0\",\"StartFileName\":\"/IncreFlashAttention.cpp\",\"StartLine\":22}]},"
                              "{\"Address\":\"0x4\",\"ModuleName\":\"./aicore_binary.o\",\"Symbol\":[{\"Column\":0,\"Discriminator\":0,\"FileName\":\"/IncreFlashAttention.cpp\",\"FunctionName\":\"IncreFlashAttention_mix_aiv\",\"Line\":23,\"StartAddress\":\"0x4\",\"StartFileName\":\"/IncreFlashAttention_.cpp\",\"StartLine\":22}]},"
                              "{\"Address\":\"0x8\",\"ModuleName\":\"./aicore_binary.o\",\"Symbol\":[{\"Column\":0,\"Discriminator\":0,\"FileName\":\"/IncreFlashAttention.cpp\",\"FunctionName\":\"g_cubeTPipePtr\",\"Line\":23,\"StartAddress\":\"0x8\",\"StartFileName\":\"/IncreFlashAttention.cpp\",\"StartLine\":22}]}]";
    auto addr2Lines = nlohmann::json::parse(jsonContext);
    ASSERT_TRUE(symbolizerParser.ParseAddr2Offset(addr2Lines));
}

TEST(SimTask, checkifNeedAppend_return_line_empty_false)
{
    GlobalMockObject::verify();
    ProfArgs profArgs;
    OpSimProf opProf(profArgs);
    SimulatorTask task("test", opProf);
    std::string fileName;
    std::string part;
    std::string listName;
    std::string item;
    std::vector<std::string> newLines;
    EXPECT_FALSE(task.CreateConfigFileWithAppend(fileName, part, listName, item));
    GlobalMockObject::verify();
}

TEST(ApiDta, checkifNeedAppend_return_line_empty_false)
{
    ApiData apidata;
    CodeInstrData codeInstrData;
    std::string outputPath;
    nlohmann::json apiJson;
    CodeFile codefile;
    codefile.file = "./a.cpp";
    CodeLine codeLine;
    codefile.lines = {codeLine};
    nlohmann::json lineDetails;
    codeLine.ToJson(lineDetails);
    codeInstrData.files = {codefile};
    codeInstrData.cores = {"core1"};
    EXPECT_TRUE(apidata.FileStats(codeInstrData, "", lineDetails));
}

TEST(PipeType, pipe_get_target_value)
{
    PipeType pipeType;
    EXPECT_EQ(pipeType.FindPipe("FLOWCTRL", "", ""), "FLOWCTRL");
    EXPECT_EQ(pipeType.FindPipe("FIXP", "", ""), "FIXP");
    EXPECT_EQ(pipeType.FindPipe("EVENT", "", ""), "EVENT");
    EXPECT_EQ(pipeType.FindPipe("", "scalar_ld", ""), "SCALAR");
    EXPECT_EQ(pipeType.FindPipe("", "", ""), "ALL");
}

/**
 * |  用例集  | MteThroughtput
 * | 测试函数 | Process
 * |  用例名  | test_process_when_given_valid_data_and_expect_no_throw
 * | 用例描述 | 输入有效数据，无异常抛出
 */
TEST(MteThroughtput, test_process_when_given_valid_data_and_expect_no_throw)
{
    std::vector<std::string> dumpVec = {"core0.mte_log.dump", "core1.mte_log.dump"};
    MOCKER(&Utility::GetFileNames)
        .stubs()
        .with(any(), outBound(dumpVec), any())
        .will(returnValue(true));
    MOCKER(&Utility::GetFileSize)
        .stubs()
        .will(returnValue(1024));
    MteThroughput mteThroughtput;
    ASSERT_NO_THROW(mteThroughtput.Process("", ChipProductType::ASCEND910B4, 2));
}