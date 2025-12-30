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

#include "ascend_hal.h"

/// fake implement of hal interfaces
drvError_t halGetDeviceInfo(uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value)
{
    return DRV_ERROR_NONE;
}

int prof_drv_start(unsigned int device_id, unsigned int channel_id, struct prof_start_para *start_para)
{
    return 0;
}

int prof_channel_read(unsigned int device_id, unsigned int channel_id, char *out_buf, unsigned int buf_size)
{
    return 0;
}

int prof_stop(unsigned int device_id, unsigned int channel_id)
{
    return 0;
}

int prof_channel_poll(struct prof_poll_info *out_buf, int num, int timeout)
{
    return 0;
}

int halProfDataFlush(unsigned int device_id, unsigned int channel_id, unsigned int *data_len)
{
    return 0;
}