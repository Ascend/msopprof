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

#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <set>
#include <string>
#include <vector>
#include <gtest/gtest.h>

#define private public
#include "parse/data_calculator/instr_detail_calculator/gpr_live_register_calculator.h"
#undef private

using namespace Profiling;
using namespace Profiling::Parse;

namespace {
class GprLiveRegisterCalculatorUt : public testing::Test {
protected:
    static MergeInfo MakeInstruction(const std::string &name, const std::string &detail)
    {
        MergeInfo mergeInfo{};
        mergeInfo.name = name;
        mergeInfo.detail = detail;
        return mergeInfo;
    }

    void RegisterInstructions(std::vector<MergeInfo> instructions)
    {
        instrDetailTable_ = Utility::MakeShared<InstrDetailTable>(instructions);
        ASSERT_NE(instrDetailTable_, nullptr);
        ASSERT_TRUE(dataCenter_.DataTableRegister(instrDetailTable_));
    }

    std::vector<int> GetGprCounts() const
    {
        const auto counts = instrDetailTable_->GetColumnData<int>(InstrDetailTable::GPR_COUNT);
        return counts == nullptr ? std::vector<int>{} : *counts;
    }

    DataCenter dataCenter_;
    std::shared_ptr<InstrDetailTable> instrDetailTable_;
};
} // namespace

/**
 * |  用例集  | GprLiveRegisterCalculatorUt
 * | 测试函数 | Entry
 * |  用例名  | test_Entry_when_instr_detail_table_is_missing_expect_nonblocking_error
 * | 用例描述 | 不向DataCenter注册强制依赖的InstrDetailTable，验证计算入口返回NONBLOCKING_ERROR而不生成统计
 */
TEST_F(GprLiveRegisterCalculatorUt, test_Entry_when_instr_detail_table_is_missing_expect_nonblocking_error)
{
    InstrDetailConfig config{ChipProductType::ASCEND910B_SERIES};
    GPRLiveRegisterCalculator calculator(dataCenter_, config);
    EXPECT_EQ(calculator.Entry(), PluginErrorCode::NONBLOCKING_ERROR);
}

/**
 * |  用例集  | GprLiveRegisterCalculatorUt
 * | 测试函数 | Entry
 * |  用例名  | test_Entry_when_a2_destination_register_is_redefined_expect_expired_registers_removed_from_counts
 * | 用例描述 | 三条A2指令复用源寄存器并重写X0，验证旧活跃区间被逐条扣减且最终GPR_COUNT依次为3、2、2
 */
TEST_F(GprLiveRegisterCalculatorUt, test_Entry_when_a2_destination_register_is_redefined_expect_expired_registers_removed_from_counts)
{
    RegisterInstructions({
        MakeInstruction("ADD", "X0, X1, X2"),
        MakeInstruction("CMP", "X1"),
        MakeInstruction("ADD", "X0, X2")
    });
    InstrDetailConfig config{ChipProductType::ASCEND910B_SERIES};
    GPRLiveRegisterCalculator calculator(dataCenter_, config);

    ASSERT_EQ(calculator.Entry(), PluginErrorCode::SUCCESS);
    const std::vector<int> expectedCounts = {3, 2, 2};
    EXPECT_EQ(GetGprCounts(), expectedCounts);
}

/**
 * |  用例集  | GprLiveRegisterCalculatorUt
 * | 测试函数 | GetDstAndSrcRegister
 * |  用例名  | test_GetDstAndSrcRegister_when_310p_scalar_add_mixes_register_syntaxes_expect_dst_and_src_split
 * | 用例描述 | 310P的scalar_add混用四种寄存器写法，验证全部操作数被提取且首个操作数作为目的、其余作为源
 */
TEST_F(GprLiveRegisterCalculatorUt, test_GetDstAndSrcRegister_when_310p_scalar_add_mixes_register_syntaxes_expect_dst_and_src_split)
{
    InstrDetailConfig config{ChipProductType::ASCEND310P_SERIES};
    GPRLiveRegisterCalculator calculator(dataCenter_, config);
    const auto mergeInfo = MakeInstruction("scalar_add", "X0, xd:0x1, xn:2, x[3]");
    std::vector<std::string> dstRegisters;
    std::vector<std::string> srcRegisters;

    calculator.GetDstAndSrcRegister(dstRegisters, srcRegisters, mergeInfo);

    const std::vector<std::string> expectedDst = {"0"};
    const std::vector<std::string> expectedSrc = {"1", "2", "3"};
    EXPECT_EQ(dstRegisters, expectedDst);
    EXPECT_EQ(srcRegisters, expectedSrc);
}

/**
 * |  用例集  | GprLiveRegisterCalculatorUt
 * | 测试函数 | GetNormalDstAndSrcRegister
 * |  用例名  | test_GetNormalDstAndSrcRegister_when_destination_operand_is_missing_expect_remaining_register_as_src
 * | 用例描述 | 目的操作数位置为1但实际只提取到一个寄存器，验证不会越界删除且剩余寄存器仍作为源返回
 */
TEST_F(GprLiveRegisterCalculatorUt, test_GetNormalDstAndSrcRegister_when_destination_operand_is_missing_expect_remaining_register_as_src)
{
    std::vector<std::string> dstRegisters;
    std::vector<std::string> srcRegisters;
    const std::vector<uint16_t> dstRegisterLoc = {1};
    std::vector<std::string> allRegisters = {"7"};

    GetNormalDstAndSrcRegister(dstRegisters, srcRegisters, dstRegisterLoc, allRegisters);

    EXPECT_TRUE(dstRegisters.empty());
    const std::vector<std::string> expectedSrc = {"7"};
    EXPECT_EQ(srcRegisters, expectedSrc);
}

/**
 * |  用例集  | GprLiveRegisterCalculatorUt
 * | 测试函数 | GetDstAndSrcRegisterA5
 * |  用例名  | test_GetDstAndSrcRegisterA5_when_json_register_list_contains_non_string_expect_only_valid_roles_extracted
 * | 用例描述 | A5的register_list混入数值元素和有效Rm、Rn、Rd字符串，验证仅跳过坏元素并正确拆分目的与源寄存器
 */
TEST_F(GprLiveRegisterCalculatorUt, test_GetDstAndSrcRegisterA5_when_json_register_list_contains_non_string_expect_only_valid_roles_extracted)
{
    InstrDetailConfig config{ChipProductType::ASCEND950_SERIES};
    GPRLiveRegisterCalculator calculator(dataCenter_, config);
    const auto mergeInfo = MakeInstruction(
        "A5_INSTR", R"({"register_list":["Rm:7",42,"Rn:8","Rd:b"]})");
    std::vector<std::string> dstRegisters;
    std::vector<std::string> srcRegisters;

    calculator.GetDstAndSrcRegisterA5(dstRegisters, srcRegisters, mergeInfo);

    const std::vector<std::string> expectedDst = {"b"};
    const std::vector<std::string> expectedSrc = {"7", "8"};
    EXPECT_EQ(dstRegisters, expectedDst);
    EXPECT_EQ(srcRegisters, expectedSrc);
}

/**
 * |  用例集  | GprLiveRegisterCalculatorUt
 * | 测试函数 | GetDstAndSrcRegisterA5
 * |  用例名  | test_GetDstAndSrcRegisterA5_when_json_register_list_is_not_array_expect_no_registers
 * | 用例描述 | 合法JSON对象中的register_list错误地使用对象而非数组，验证协议类型不符时目的与源寄存器均为空
 */
TEST_F(GprLiveRegisterCalculatorUt, test_GetDstAndSrcRegisterA5_when_json_register_list_is_not_array_expect_no_registers)
{
    InstrDetailConfig config{ChipProductType::ASCEND950_SERIES};
    GPRLiveRegisterCalculator calculator(dataCenter_, config);
    const auto mergeInfo = MakeInstruction("A5_INSTR", R"({"register_list":{"reg":"Rm:7"}})");
    std::vector<std::string> dstRegisters;
    std::vector<std::string> srcRegisters;

    calculator.GetDstAndSrcRegisterA5(dstRegisters, srcRegisters, mergeInfo);

    EXPECT_TRUE(dstRegisters.empty());
    EXPECT_TRUE(srcRegisters.empty());
}

/**
 * |  用例集  | GprLiveRegisterCalculatorUt
 * | 测试函数 | GetDstAndSrcRegister
 * |  用例名  | test_GetDstAndSrcRegister_when_instruction_is_not_mapped_expect_all_extracted_registers_as_src
 * | 用例描述 | A2指令名不在目的位置映射表中但详情包含X7和X8，验证不套用目的规则且两个寄存器都作为源返回
 */
TEST_F(GprLiveRegisterCalculatorUt, test_GetDstAndSrcRegister_when_instruction_is_not_mapped_expect_all_extracted_registers_as_src)
{
    InstrDetailConfig config{ChipProductType::ASCEND910B_SERIES};
    GPRLiveRegisterCalculator calculator(dataCenter_, config);
    const auto mergeInfo = MakeInstruction("NOT_MAPPED", "X7, X8");
    std::vector<std::string> dstRegisters;
    std::vector<std::string> srcRegisters;

    calculator.GetDstAndSrcRegister(dstRegisters, srcRegisters, mergeInfo);

    EXPECT_TRUE(dstRegisters.empty());
    const std::vector<std::string> expectedSrc = {"7", "8"};
    EXPECT_EQ(srcRegisters, expectedSrc);
}
