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
#include <dlfcn.h>
#include "mockcpp/mockcpp.hpp"
#include "ascend_helper.h"
#include "filesystem.h"
#define private public
#include "ascend_hal/ascend_hal.h"
#include "common/hal_helper.h"
#undef private
using namespace Common;

namespace Common {
using halGetDeviceInfoFunc = drvError_t(*)(uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value);
extern halGetDeviceInfoFunc g_halGetDeviceInfo;
}

int DcmiInitFuncMock1() {
    return -1;
}

int DcmiInitFuncMock2() {
    return 0;
}

int DcmiGetCardIdDeviceIdFuncMock (int *a, int *b, int c) {
    if (c == 0) {
        return 0;
    }
    return -1;
}

int DcmiGetDeviceGmInfoFuncMock(int a, int b, dcmi_gm_product_info_t *c) {
    if (a == 0) {
        return 0;
    }
    return -1;
}

int CommonMock() {
    return 1;
}

class HalHelperTest : public testing::Test {
protected:
   HalHelper halHelper;
   int64_t freq;
   int64_t aiCoreNum;

   void SetUp() override {
       g_halGetDeviceInfo = nullptr;
   }

   void TearDown() override {
       g_halGetDeviceInfo = nullptr;
   }
};

TEST_F(HalHelperTest, GetPlatformType_NullGetDeviceInfo_ReturnFalse)
{
   g_halGetDeviceInfo = nullptr;
   EXPECT_TRUE(halHelper.GetPlatformType() == Common::ChipType::END_TYPE);
}

TEST_F(HalHelperTest, GetPlatformType_ErrorGetDeviceInfo_ReturnFalse)
{
   g_halGetDeviceInfo = [](uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value) {
       return (drvError_t)1;
   };
   EXPECT_TRUE(halHelper.GetPlatformType() == Common::ChipType::END_TYPE);
}

TEST_F(HalHelperTest, GetPlatformType_ChipTypeOutOfRange_ReturnFalse)
{
   g_halGetDeviceInfo = [](uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value) {
       *value = 0x1111111111111111;
       return DRV_ERROR_NONE;
   };
   EXPECT_TRUE(halHelper.GetPlatformType() == Common::ChipType::END_TYPE);
}

TEST_F(HalHelperTest, GetPlatformType_Success_ReturnTrue)
{
   g_halGetDeviceInfo = [](uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value) {
       *value = 0x100;
       return DRV_ERROR_NONE;
   };
   EXPECT_EQ(halHelper.GetPlatformType(), static_cast<ChipType>(0x1));
}

TEST_F(HalHelperTest, GetAicoreFreq_NullFunc_ReturnFalse)
{
   g_halGetDeviceInfo = nullptr;
   bool result = halHelper.GetAicoreFreq(freq);

   EXPECT_FALSE(result);
}

TEST_F(HalHelperTest, GetAicoreFreq_InvalidFreq_ReturnFalse)
{
   g_halGetDeviceInfo = [](uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value) {
       return DRV_ERROR_NONE;
   };

   bool result = halHelper.GetAicoreFreq(freq);

   EXPECT_FALSE(result);
}

TEST_F(HalHelperTest, GetAicoreFreq_ValidFreq_ReturnTrue)
{
   g_halGetDeviceInfo = [](uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value) {
       *value = 123456789;
       return DRV_ERROR_NONE;
   };

   bool result = halHelper.GetAicoreFreq(freq);
   EXPECT_TRUE(result);
   EXPECT_EQ(freq, 123456789);
}

TEST_F(HalHelperTest, GetTaskSchedulerFreq_InvalidFreq_ReturnFalse)
{
    g_halGetDeviceInfo = [](uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value) {
        return DRV_ERROR_NONE;
    };
    bool result = halHelper.GetTaskSchedulerFreq(freq);
    EXPECT_FALSE(result);
}

TEST_F(HalHelperTest, GetTaskSchedulerFreq_ValidFreq_ReturnTrue)
{
    g_halGetDeviceInfo = [](uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value) {
        *value = 123456789;
        return DRV_ERROR_NONE;
    };
    bool result = halHelper.GetTaskSchedulerFreq(freq);
    EXPECT_TRUE(result);
    EXPECT_EQ(freq, 123456789);
}

TEST_F(HalHelperTest, GetAiCoreNum_InvalidAiCoreNum_ReturnFalse)
{
    g_halGetDeviceInfo = [](uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value) {
        return DRV_ERROR_NONE;
    };
    bool result = halHelper.GetAiCoreNum(aiCoreNum);
    EXPECT_FALSE(result);
}

TEST_F(HalHelperTest, GetAiCoreNum_ValidAiCoreNum_ReturnTrue)
{
    g_halGetDeviceInfo = [](uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value) {
        *value = 8;
        return DRV_ERROR_NONE;
    };
    bool result = halHelper.GetAiCoreNum(aiCoreNum);
    EXPECT_TRUE(result);
    EXPECT_EQ(aiCoreNum, 8);
}

TEST_F(HalHelperTest, SetCurrentDeviceId_Return_True)
{
    halHelper.SetCurrentDeviceId(10);
    EXPECT_EQ(halHelper.deviceId_, 10);
}

TEST_F(HalHelperTest, GetCurrentDeviceId_Return_True)
{
    halHelper.deviceId_ = 1;
    EXPECT_EQ(halHelper.GetCurrentDeviceId(), 1);
}

TEST_F(HalHelperTest, GetGmType_Return_True)
{
    halHelper.gmType_ = GmType::DEFAULT;
    EXPECT_EQ(halHelper.GetGmType(), GmType::DEFAULT);
}

TEST(HalHelper, DcmiInit_Return_False)
{
    std::string str = "aaa";
    HalHelper::Instance().handleDcmi_ = nullptr;
    bool result = HalHelper::Instance().DcmiInit();
    EXPECT_FALSE(result);

    HalHelper::Instance().handleDcmi_ = &str;
    GlobalMockObject::verify();
    void * handle = reinterpret_cast<void *>(DcmiInitFuncMock1);
    MOCKER(&dlsym)
            .stubs()
            .will(returnValue(handle));
    result = HalHelper::Instance().DcmiInit();
    EXPECT_FALSE(result);
}

TEST(HalHelper, DcmiInit_Return_True)
{
    bool result = false;
    HalHelper::Instance().handleDcmi_ = &result;
    GlobalMockObject::verify();
    int (*DcmiInitFunc2)();
    void * handle = reinterpret_cast<void *>(DcmiInitFuncMock2);
    MOCKER(&dlsym)
            .stubs()
            .will(returnValue(handle));
    result = HalHelper::Instance().DcmiInit();
    EXPECT_TRUE(result);
}

TEST(HalHelper, GetCardIdDeviceIdFromLogicId_Return_False)
{
    std::string str = "aaa";
    int carId = 1;
    int chipId = 1;
    int logicId = 1;
    HalHelper::Instance().handleDcmi_ = nullptr;
    bool result = HalHelper::Instance().GetCardIdDeviceIdFromLogicId(&carId, &chipId, logicId);
    EXPECT_FALSE(result);

    HalHelper::Instance().handleDcmi_ = &str;
    void * handle = nullptr;
    GlobalMockObject::verify();
    MOCKER(&dlsym)
            .stubs()
            .will(returnValue(handle));
    result = HalHelper::Instance().GetCardIdDeviceIdFromLogicId(&carId, &chipId, logicId);
    EXPECT_FALSE(result);

    GlobalMockObject::verify();
    handle = reinterpret_cast<void *>(DcmiGetCardIdDeviceIdFuncMock);
    MOCKER(&dlsym)
            .stubs()
            .will(returnValue(handle));
    result = HalHelper::Instance().GetCardIdDeviceIdFromLogicId(&carId, &chipId, logicId);
    EXPECT_FALSE(result);
}

TEST(HalHelper, GetCardIdDeviceIdFromLogicId_Return_True)
{
    int carId = 0;
    int chipId = 0;
    int logicId = 0;
    bool result = false;
    HalHelper::Instance().handleDcmi_ = &result;
    void * handle = reinterpret_cast<void *>(DcmiGetCardIdDeviceIdFuncMock);
    GlobalMockObject::verify();
    MOCKER(&dlsym)
            .stubs()
            .will(returnValue(handle));
    result = HalHelper::Instance().GetCardIdDeviceIdFromLogicId(&carId, &chipId, logicId);
    EXPECT_TRUE(result);
}

TEST(HalHelper, SetGmType_Return_False)
{
    int a = 1;
    int b = 1;
    dcmi_gm_product_info_t c;
    std::string str = "aaa";
    HalHelper::Instance().handleDcmi_ = nullptr;
    bool result = HalHelper::Instance().SetGmType(a, b, c);
    EXPECT_FALSE(result);

    HalHelper::Instance().handleDcmi_ = &str;
    void * handle = nullptr;
    GlobalMockObject::verify();
    MOCKER(&dlsym)
            .stubs()
            .will(returnValue(handle));
    result = HalHelper::Instance().SetGmType(a, b, c);
    EXPECT_FALSE(result);

    GlobalMockObject::verify();
    handle = reinterpret_cast<void *>(DcmiGetDeviceGmInfoFuncMock);
    MOCKER(&dlsym)
            .stubs()
            .will(returnValue(handle));
    result = HalHelper::Instance().SetGmType(a, b, c);
    EXPECT_FALSE(result);
}

TEST(HalHelper, SetGmType_Return_True)
{
    int a = 0;
    int b = 0;
    dcmi_gm_product_info_t c;

    HalHelper::Instance().handleDcmi_ = &a;
    GlobalMockObject::verify();
    void *handle = reinterpret_cast<void *>(DcmiGetDeviceGmInfoFuncMock);
    MOCKER(&dlsym)
            .stubs()
            .will(returnValue(handle));
    bool result = HalHelper::Instance().SetGmType(a, b, c);
    EXPECT_TRUE(result);
}

TEST(HalHelper, CheckGmType_Return_GmType_DEFAULT)
{
    GlobalMockObject::verify();
    MOCKER(&HalHelper::DcmiInit)
            .stubs()
            .will(returnValue(false));
    HalHelper::Instance().CheckGmType();
    EXPECT_EQ(HalHelper::Instance().gmType_, GmType::DEFAULT);

    GlobalMockObject::verify();
    MOCKER(&HalHelper::DcmiInit)
            .stubs()
            .will(returnValue(true));
    MOCKER(&HalHelper::GetCardIdDeviceIdFromLogicId)
            .stubs()
            .will(returnValue(false));
    HalHelper::Instance().CheckGmType();
    EXPECT_EQ(HalHelper::Instance().gmType_, GmType::DEFAULT);

    GlobalMockObject::verify();
    MOCKER(&HalHelper::DcmiInit)
            .stubs()
            .will(returnValue(true));
    MOCKER(&HalHelper::GetCardIdDeviceIdFromLogicId)
            .stubs()
            .will(returnValue(true));
    MOCKER(&HalHelper::SetGmType)
            .stubs()
            .will(returnValue(false));
    HalHelper::Instance().CheckGmType();
    EXPECT_EQ(HalHelper::Instance().gmType_, GmType::DEFAULT);

    GlobalMockObject::verify();
}

TEST(HalHelper, IsSupportPlatform_Return_False)
{
    bool result = HalHelper::Instance().IsSupportPlatform(ChipType::END_TYPE);
    EXPECT_FALSE(result);
}

TEST(HalHelper, Constructor_HalHelper)
{
    GlobalMockObject::verify();
    void *handle = reinterpret_cast<void *>(CommonMock);
    std::string so("so");
    MOCKER(&Utility::GetSoFromEnvVar)
        .stubs()
        .will(returnValue(so));
    MOCKER(&Utility::CheckInputFileValid)
            .stubs()
            .will(returnValue(true));
    MOCKER(&dlopen)
            .stubs()
            .will(returnValue(handle));
    MOCKER(&dlsym)
            .stubs()
            .will(returnValue(handle));
    MOCKER(&dlclose)
        .stubs()
        .will(returnValue(0));
    {
        HalHelper halHelper;
        EXPECT_NE(g_halGetDeviceInfo, nullptr);
    }
    GlobalMockObject::verify();
}
