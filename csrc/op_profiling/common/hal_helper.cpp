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


#include "hal_helper.h"
#include <dlfcn.h>
#include "log.h"
#include "ascend_helper.h"
#include "filesystem.h"
using namespace Utility;

namespace Common {
using halGetDeviceInfoFunc = drvError_t(*)(uint32_t, int32_t, int32_t, int64_t*);
halGetDeviceInfoFunc g_halGetDeviceInfo;

HalHelper &HalHelper::Instance(void)
{
    static HalHelper instance;
    return instance;
}

HalHelper::HalHelper()
{
    std::string halSo = GetSoFromEnvVar("libascend_hal.so");
    std::string dcmiSo = GetSoFromEnvVar("libdcmi.so");
    if (dcmiSo.empty() || !CheckInputFileValid(dcmiSo, "so")) {
        LogWarn("Can't find valid libdcmi.so, please check your LD_LIBRARY_PATH");
    }
    if (halSo.empty() || !CheckInputFileValid(halSo, "so")) {
        LogWarn("Can't find valid libascend_hal.so, please check your LD_LIBRARY_PATH");
        return;
    }
    handleHal_ = dlopen(halSo.c_str(), RTLD_LAZY);
    handleDcmi_ = dlopen(dcmiSo.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (handleHal_ == nullptr) {
        return;
    }
    g_halGetDeviceInfo = (halGetDeviceInfoFunc) dlsym(handleHal_, "halGetDeviceInfo");
}

HalHelper::~HalHelper()
{
    if (handleHal_ != nullptr) {
        dlclose(handleHal_);
        handleHal_ = nullptr;
    }
    if (handleDcmi_ != nullptr) {
        dlclose(handleDcmi_);
        handleDcmi_ = nullptr;
    }
}

ChipType HalHelper::GetPlatformType(void) const
{
    static ChipType chipType = ChipType::END_TYPE;
    if (chipType != ChipType::END_TYPE) {
        return chipType;
    }
    if (g_halGetDeviceInfo == nullptr) {
        LogError("Fail to get soc platform because of null ptr");
        return ChipType::END_TYPE;
    }
    int64_t versionInfo = 0;
    int ret = g_halGetDeviceInfo(0, MODULE_TYPE_SYSTEM, INFO_TYPE_VERSION, &versionInfo);
    if (ret != DRV_ERROR_NONE) {
        LogError("Fail to get soc platform because of error code:%d.", ret);
        return ChipType::END_TYPE;
    }
 
    uint32_t platformType = (static_cast<uint64_t>(versionInfo) >> 8) & 0xff;
    if ((platformType > static_cast<uint32_t>(ChipType::ASCEND310B) &&
        platformType < static_cast<uint32_t>(ChipType::ASCEND950)) ||
        platformType >= static_cast<uint32_t>(ChipType::END_TYPE)) {
        LogError("Fail to get soc platform because of invalid type:%u.", platformType);
        return ChipType::END_TYPE;
    }
    chipType = static_cast<ChipType>(platformType);
    return chipType;
}

bool HalHelper::GetAicoreFreq(int64_t &freq) const
{
    if (g_halGetDeviceInfo == nullptr) {
        return false;
    }
    freq = 0;
    int ret = g_halGetDeviceInfo(0, MODULE_TYPE_AICORE, INFO_TYPE_FREQUE, &freq);
    if (ret != DRV_ERROR_NONE || freq == 0) {
        return false;
    }
    return true;
}

bool HalHelper::GetTaskSchedulerFreq(int64_t &freq) const
{
    if (g_halGetDeviceInfo == nullptr) {
        return false;
    }
    freq = 0;
    int ret = g_halGetDeviceInfo(0, MODULE_TYPE_SYSTEM, INFO_TYPE_DEV_OSC_FREQUE, &freq);
    if (ret != DRV_ERROR_NONE || freq == 0) {
        return false;
    }
    return true;
}

bool HalHelper::GetAiCoreNum(int64_t &aiCoreNum) const
{
    if (g_halGetDeviceInfo == nullptr) {
        return false;
    }
    aiCoreNum = 0;
    int ret = g_halGetDeviceInfo(0, MODULE_TYPE_AICORE, INFO_TYPE_CORE_NUM, &aiCoreNum);
    if (ret != DRV_ERROR_NONE || aiCoreNum == 0) {
        return false;
    }
    return true;
}

bool HalHelper::IsSupportPlatform(void) const
{
    ChipType chipType = GetPlatformType();
    return IsSupportPlatform(chipType);
}

bool HalHelper::IsSupportPlatform(ChipType chipType) const
{
    return chipType == ChipType::ASCEND910B || chipType == ChipType::ASCEND310P || chipType == ChipType::ASCEND310B
        || chipType == ChipType::ASCEND950;
}

bool HalHelper::DcmiInit()
{
    using DcmiInitFunc = int(*)();
    if (handleDcmi_ == nullptr) {
        return false;
    }
    DcmiInitFunc dcmiInit = (DcmiInitFunc) dlsym(handleDcmi_, "dcmi_init");
    if (dcmiInit == nullptr) {
        LogDebug("Can not get device dcmi info function.");
        dlclose(handleDcmi_);
        handleDcmi_ = nullptr;
        return false;
    }
    int ret = dcmiInit();
    return ret == 0;
}

bool HalHelper::GetCardIdDeviceIdFromLogicId(int *cardId, int *chipId, unsigned int logicId) const
{
    using DcmiGetCardIdDeviceIdFunc = int(*)(int *, int *, int);
    if (handleDcmi_ == nullptr) {
        LogDebug("Can not get device id info.");
        return false;
    }
    DcmiGetCardIdDeviceIdFunc dcmiGetCardIdDeviceId = (DcmiGetCardIdDeviceIdFunc) dlsym(handleDcmi_,
        "dcmi_get_card_id_device_id_from_logicid");
    if (dcmiGetCardIdDeviceId == nullptr) {
        LogDebug("Can not get device id info function.");
        return false;
    }
    int ret = dcmiGetCardIdDeviceId(cardId, chipId, logicId);
    if (ret != 0) {
        LogDebug("Can not get card and device id. Error code is %d", ret);
        return false;
    }
    return true;
}

bool HalHelper::SetGmType(int cardId, int deviceId, dcmi_gm_product_info_t &gmInfo) const
{
    using DcmiGetDeviceGmInfoFunc = int(*)(int, int, dcmi_gm_product_info_t*);
    if (handleDcmi_ == nullptr) {
        LogDebug("Can not get device gm info.");
        return false;
    }
    DcmiGetDeviceGmInfoFunc dcmiGetDeviceGmInfo = (DcmiGetDeviceGmInfoFunc) dlsym(handleDcmi_,
        "dcmi_get_device_hbm_product_info");
    if (dcmiGetDeviceGmInfo  == nullptr) {
        LogDebug("Can not get device gm info function.");
        return false;
    }
    int ret = dcmiGetDeviceGmInfo(cardId, deviceId, &gmInfo);
    if (ret != 0) {
        LogDebug("Can not get gm.Error code is %d", ret);
        return false;
    }
    LogDebug("The gm of device %d is %d", deviceId, gmInfo.manufacturer_id);
    return true;
}

void HalHelper::CheckGmType()
{
    gmType_ = GmType::DEFAULT;
    if (!DcmiInit()) {
        LogWarn("Failed to get gm info, the default parameters will be used.");
        return;
    }
    int cardId = 0;
    int chipId = 0;
    if (!GetCardIdDeviceIdFromLogicId(&cardId, &chipId, deviceId_)) {
        return;
    }
    dcmi_gm_product_info_t gmInfo;
    if (!SetGmType(cardId, chipId, gmInfo)) {
        return;
    }
    if (GM_PRODUCT.find(gmInfo.manufacturer_id) != GM_PRODUCT.end()) {
        gmType_ = static_cast<GmType>(gmInfo.manufacturer_id);
    }
}

void HalHelper::SetCurrentDeviceId(unsigned int deviceId)
{
    deviceId_ = deviceId;
}

int HalHelper::GetCurrentDeviceId() const
{
    return deviceId_;
}

GmType HalHelper::GetGmType() const
{
    return gmType_;
}
}  // namespace Common
