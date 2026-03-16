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

#define private public
#define protected public
#include "parse/data_parser/mte_log_parser/mte_log_parser.h"
#include "smart_pointer.h"
#undef private
#undef protected
#include "filesystem.h"

using namespace Profiling::Parse;
using namespace Utility;

/**
 * |  用例集  | MteLogParserTest
 * | 测试函数 | Entry
 * |  用例名  | test_entry_when_input_is_invalid_and_expect_return_error
 * | 用例描述 | 输入无效时，返回错误
 */
TEST(MteLogParserTest, test_entry_when_input_is_invalid_and_expect_return_error)
{
    DataCenter dataCenter;
    MteLogCfg mteLogCfg;
    mteLogCfg.coreId = 10;
    MteLogParser mteLogParser1(dataCenter, mteLogCfg);
    ASSERT_TRUE(mteLogParser1.Entry() == PluginErrorCode::NONBLOCKING_ERROR);
    std::shared_ptr<std::vector<MteLogInstrMap>> mteLogInstrMapVecPtr = MakeShared<std::vector<MteLogInstrMap>>();
    MteLogInstrMap mteLogInstrTblMap;
    mteLogInstrMapVecPtr->emplace_back(mteLogInstrTblMap);
    dataCenter.DataTableRegister(mteLogInstrMapVecPtr);
    MteLogParser mteLogParser2(dataCenter, mteLogCfg);
    ASSERT_TRUE(mteLogParser2.Entry() == PluginErrorCode::NONBLOCKING_ERROR);
}

/**
 * |  用例集  | MteLogParserTest
 * | 测试函数 | Entry
 * |  用例名  | test_entry_when_input_is_valid_and_expect_return_success
 * | 用例描述 | 输入有效时，返回成功
 */
TEST(MteLogParserTest, test_entry_when_input_is_valid_and_expect_return_success)
{
    std::vector<std::string> mteLogFileLines = {
        "[info] 19333: BRIF send_req, addr: 0x10f74400, size: 256, tag_id: 4, instr_id: 4, req_id: 5259",
        "[info] 19687: BRIF recv_rsp, uop_cnt: 1, tnx_id: 0, tag_id: 4, instr_id: 4, req_id: 5259",
        "[info] 19689: BRIF recv_rsp, uop_cnt: 2, tnx_id: 1, tag_id: 4, instr_id: 4, req_id: 5259",
        "[info] 19335: MOV_OUT_TO_UB, set_cmd_done",
        "[info] 19703: UB1WIF send_req, addr: 0x0, size: 128, tag_id: 4, instr_id: 4, req_id: 5985",
        "[info] 19704: UB1WIF send_req, addr: 0x80, size: 128, tag_id: 4, instr_id: 4, req_id: 5986",
        "[info] 19709: UB1WIF recv_rsp, addr: 0x0, size: 128, tag_id: 4, instr_id: 4, req_id: 5985",
        "[info] 19710: UB1WIF recv_rsp, addr: 0x80, size: 128, tag_id: 4, instr_id: 4, req_id: 5986",
        "[info] 24198: UB0WIF send_req, addr: 0x200, size: 128, tag_id: 37, instr_id: 53, req_id: 9011",
        "[info] 24204: UB0WIF recv_rsp, addr: 0x200, size: 128, tag_id: 37, instr_id: 53, req_id: 9011",
        "[info] 24183: BRIF recv_rsp, uop_cnt: 1, tnx_id: 0, tag_id: 37, instr_id: 53, req_id: 8849",
        "[info] 23827: BRIF send_req, addr: 0x10f7be00, size: 256, tag_id: 37, instr_id: 53, req_id: 8849",
    };
    DataCenter dataCenter;
    MteLogCfg mteLogCfg;
    std::shared_ptr<std::vector<MteLogInstrMap>> mteLogInstrMapVecPtr = MakeShared<std::vector<MteLogInstrMap>>();
    MteLogInstrMap mteLogInstrTblMap;
    mteLogInstrMapVecPtr->emplace_back(mteLogInstrTblMap);
    mteLogCfg.coreId = 0;
    mteLogCfg.chipType = ChipProductType::ASCEND910B4;
    dataCenter.DataTableRegister(mteLogInstrMapVecPtr);
    MteLogParser mteLogParser(dataCenter, mteLogCfg);
    mteLogParser.DependencyRegister();
    MOCKER(&ReadFileByMMap)
        .stubs()
        .with(any(), outBound(mteLogFileLines))
        .will(returnValue(true));
    ASSERT_EQ(mteLogParser.Entry(), PluginErrorCode::SUCCESS);
    ASSERT_EQ((*mteLogInstrMapVecPtr)[0].size(), 2);
    ASSERT_EQ((*mteLogInstrMapVecPtr)[0][4].instrType, MteLogInstrType::GM_TO_UB);
    ASSERT_EQ((*mteLogInstrMapVecPtr)[0][4].reqTbl[5259].dataSize, 256);
    ASSERT_EQ((*mteLogInstrMapVecPtr)[0][53].instrType, MteLogInstrType::GM_TO_UB);
    ASSERT_EQ((*mteLogInstrMapVecPtr)[0][53].reqTbl[8849].dataSize, 256);
    GlobalMockObject::verify();
}