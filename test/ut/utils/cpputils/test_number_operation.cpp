#include <gtest/gtest.h>
#include <vector>
#include "log.h"
#include "number_operation.h"


using namespace Utility;

TEST(number, SafeEqual_with_success)
{
    float a = 1.11111;
    float b = 1.11112;
    ASSERT_TRUE(SafeEqual(a, b));
}

TEST(number, OverFlowReturnValue_with_success)
{
    int64_t maxValue = std::numeric_limits<int64_t>::max();
    int64_t minValue = std::numeric_limits<int64_t>::lowest();
    int64_t x1 = 1;
    int64_t x2 = -1;
    ASSERT_TRUE(OverFlowReturnValue(x1, false) == 0);
    ASSERT_TRUE(OverFlowReturnValue(x1, true) == maxValue);
    ASSERT_TRUE(OverFlowReturnValue(x2, true) == minValue);
}

TEST(number, SafeAdd_with_success)
{
    std::string location = "test";
    int64_t maxValue = std::numeric_limits<int64_t>::max();
    int64_t minValue = std::numeric_limits<int64_t>::lowest();
    uint64_t value = std::numeric_limits<uint64_t>::max();
    int x = 100;
    uint64_t m1 = 0;
    uint64_t m2 = 100;
    ASSERT_TRUE(SafeAdd(m1, m2, location) == 100);
    ASSERT_TRUE(SafeAdd(maxValue, maxValue, location) == maxValue);
    ASSERT_TRUE(SafeAdd(minValue, minValue, location) == minValue);
    ASSERT_TRUE(SafeAdd(value, value, location) == value);
    ASSERT_TRUE(SafeAdd(value, value, location, false) == 0);
    ASSERT_TRUE(SafeAdd(x, x, location) == 200);
}

TEST(number, SafeSub_with_success)
{
    std::string location = "test";
    int64_t maxValue = std::numeric_limits<int64_t>::max();
    int64_t minValue = std::numeric_limits<int64_t>::lowest();
    uint64_t value = std::numeric_limits<uint64_t>::max();
    int x = 100;
    uint64_t y = 0;
    ASSERT_TRUE(SafeSub(maxValue, minValue, location) == maxValue);
    ASSERT_TRUE(SafeSub(minValue, maxValue, location) == minValue);
    ASSERT_TRUE(SafeSub(y, value, location) == value);
    ASSERT_TRUE(SafeSub(y, value, location, false) == 0);
    ASSERT_TRUE(SafeSub(x, 1, location) == 99);
}

TEST(number, SafeMul_with_success)
{
    std::string location = "test";
    float floatMaxValue = std::numeric_limits<float>::max();
    float floatMinValue = std::numeric_limits<float>::lowest();
    int64_t intMaxValue = std::numeric_limits<int64_t>::max();
    int64_t intMinValue = std::numeric_limits<int64_t>::lowest();
    int x = 100;
    ASSERT_TRUE(SafeMul(floatMaxValue, floatMaxValue, location) == floatMaxValue);
    ASSERT_TRUE(SafeMul(floatMaxValue, floatMinValue, location) == floatMinValue);
    ASSERT_TRUE(SafeMul(intMaxValue, intMaxValue, location) == intMaxValue);
    ASSERT_TRUE(SafeMul(intMaxValue, intMinValue, location) == intMinValue);
    ASSERT_TRUE(SafeMul(intMaxValue, intMaxValue, location, false) == 0);
    ASSERT_TRUE(SafeMul(x, x, location) == 10000);
}

TEST(number, SafeAddAll_with_success)
{
    std::string location = "test";
    uint64_t maxValue = std::numeric_limits<uint64_t>::max();
    std::vector<uint64_t> vec1 = {maxValue, maxValue, maxValue};
    std::vector<uint64_t> vec2 =  {0, 10, 20};
    ASSERT_TRUE(SafeAddAll(vec1, location) == maxValue);
    ASSERT_TRUE(SafeAddAll(vec2, location) == 30);
}

TEST(number, SafeMulAll_with_success)
{
    std::string location = "test";
    uint64_t maxValue = std::numeric_limits<uint64_t>::max();
    std::vector<uint64_t> vec1 =  {maxValue, maxValue, maxValue};
    std::vector<uint64_t> vec2 =  {0, 10, 20};
    ASSERT_TRUE(SafeMulAll(vec1, location) == maxValue);
    ASSERT_TRUE(SafeMulAll(vec2, location) == 0);
}

TEST(number, SafeMulAddAll_with_success)
{
    std::string location = "test";
    uint64_t maxValue = std::numeric_limits<uint64_t>::max();
    std::vector<uint64_t> vec1= {maxValue, maxValue, maxValue};
    std::vector<uint64_t> vec2 = vec1;
    ASSERT_TRUE(SafeMulAddAll(vec1, vec2, location, false) == 0);
    ASSERT_TRUE(SafeMulAddAll(vec1, vec2, location) == maxValue);
}

/**
 * |  用例集  | NumberOperationUt
 * | 测试函数 | ExtractKBits
 * |  用例名  | test_ExtractKBits_should_return_correct_result
 * | 用例描述 | 测试ExtractKBits函数能够正确提取指定位宽的值
 */
TEST(InstrDetailUtilsUt, test_ExtractKBits_should_return_correct_result) {
    uint64_t testValue = 0x101;
    ASSERT_EQ(ExtractKBits(testValue, 0, 9), 0x101);
    ASSERT_EQ(ExtractKBits(testValue, 1, 1), 0x0);
    ASSERT_EQ(ExtractKBits(testValue, 1, 2), 0x0);
    ASSERT_EQ(ExtractKBits(testValue, 4, 5), 0x10);
    ASSERT_EQ(ExtractKBits(testValue, 3, 6), 0x20);
}

/**
 * |  用例集  | NumberOperationUt
 * | 测试函数 | ExtractKBits
 * |  用例名  | test_ExtractKBits_should_return_0_when_input_is_invalid
 * | 用例描述 | 测试ExtractKBits函数在输入无效时返回0
 */
TEST(InstrDetailUtilsUt, test_ExtractKBits_should_return_0_when_input_is_invalid) {
    uint64_t testValue = 0x101;
    ASSERT_EQ(ExtractKBits(testValue, 0, 64), 0);
    ASSERT_EQ(ExtractKBits(testValue, 1, 65), 0);
}
