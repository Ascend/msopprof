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

#ifndef __MSOPPROF_CORE_PLATFORM_HAL_HELPER_H__
#define __MSOPPROF_CORE_PLATFORM_HAL_HELPER_H__

#include <cstdint>
#include "defs.h"
#include "ascend_hal/ascend_hal.h"
namespace Common {

class HalHelper {
public:
    static HalHelper &Instance(void);

    ChipType GetPlatformType(void) const;

    bool GetAicoreFreq(int64_t &freq) const;

    bool GetTaskSchedulerFreq(int64_t &freq) const;

    bool GetAiCoreNum(int64_t &aiCoreNum) const;

    bool IsSupportPlatform(void) const;

    bool IsSupportPlatform(ChipType chipType) const;

    void CheckGmType();

    void SetCurrentDeviceId(unsigned int deviceId);

    int GetCurrentDeviceId() const;

    GmType GetGmType() const;
private:
    GmType gmType_ = GmType::DEFAULT;
    unsigned int deviceId_ = 0;
    void *handleHal_ = nullptr;
    void *handleDcmi_ = nullptr;
    HalHelper();
    ~HalHelper();
    HalHelper(HalHelper const &) = delete;
    HalHelper &operator=(HalHelper const &) = delete;
    bool DcmiInit();
    bool GetCardIdDeviceIdFromLogicId(int *cardId, int *chipId, unsigned int logicId) const;
    bool SetGmType(int cardId, int deviceId, dcmi_gm_product_info_t &gmInfo) const;
};

}  // namespace Common

#endif  // __MSOPPROF_CORE_PLATFORM_HAL_HELPER_H__
