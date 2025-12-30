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

#ifndef THIRD_PARTY_ASCEND_HAL_H
#define THIRD_PARTY_ASCEND_HAL_H

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#define PROF_REAL 1
#define CHANNEL_AICORE (43)
#define CHANNEL_HWTS_LOG (45)    /* add for ts0 as hwts channel */
#define CHANNEL_TSFW_L2 (47)
#define CHANNEL_STARS_SOC_LOG_BUFFER (50)   /* add for ascend910B */
#define CHANNEL_FFTS_PROFILE_BUFFER_TASK (53)   /* add for ascend910B */

typedef enum {
    MODULE_TYPE_SYSTEM = 0,  /**< system info*/
    MODULE_TYPE_AICPU,       /** < aicpu info*/
    MODULE_TYPE_CCPU,        /**< ccpu_info*/
    MODULE_TYPE_DCPU,        /**< dcpu info*/
    MODULE_TYPE_AICORE,      /**< AI CORE info*/
    MODULE_TYPE_TSCPU,       /**< tscpu info*/
    MODULE_TYPE_PCIE,        /**< PCIE info*/
    MODULE_TYPE_VECTOR_CORE, /**< VECTOR CORE info*/
    MODULE_TYPE_COMPUTING = 0x8000, /* computing power info */
} DEV_MODULE_TYPE;

typedef enum prof_channel_type {
    PROF_TS_TYPE,
    PROF_PERIPHERAL_TYPE,
    PROF_CHANNEL_TYPE_MAX,
} PROF_CHANNEL_TYPE;

typedef struct prof_poll_info {
    unsigned int device_id;
    unsigned int channel_id;
} prof_poll_info_t;


typedef struct prof_start_para {
    PROF_CHANNEL_TYPE channel_type;     /* for ts and other device */
    unsigned int sample_period;
    unsigned int real_time;             /* real mode */
    void *user_data;                    /* ts data's pointer */
    unsigned int user_data_size;        /* user data's size */
} prof_start_para_t;

typedef enum tagDrvError {
    DRV_ERROR_NONE = 0,                /**< success */
} drvError_t;

typedef struct dcmi_gm_product_info {
    unsigned short manufacturer_id; // gm厂商类型
    unsigned char reserve[62];
} dcmi_gm_product_info_t;

typedef enum {
    INFO_TYPE_ENV = 0,
    INFO_TYPE_VERSION,
    INFO_TYPE_MASTERID,
    INFO_TYPE_CORE_NUM,
    INFO_TYPE_FREQUE,
    INFO_TYPE_OS_SCHED,
    INFO_TYPE_IN_USED,
    INFO_TYPE_ERROR_MAP,
    INFO_TYPE_OCCUPY,
    INFO_TYPE_ID,
    INFO_TYPE_IP,
    INFO_TYPE_ENDIAN,
    INFO_TYPE_P2P_CAPABILITY,
    INFO_TYPE_SYS_COUNT,
    INFO_TYPE_MONOTONIC_RAW,
    INFO_TYPE_CORE_NUM_LEVEL,
    INFO_TYPE_FREQUE_LEVEL,
    INFO_TYPE_FFTS_TYPE,
    INFO_TYPE_PHY_CHIP_ID,
    INFO_TYPE_PHY_DIE_ID,
    INFO_TYPE_PF_CORE_NUM,
    INFO_TYPE_PF_OCCUPY,
    INFO_TYPE_WORK_MODE,
    INFO_TYPE_UTILIZATION,
    INFO_TYPE_HOST_OSC_FREQUE,
    INFO_TYPE_DEV_OSC_FREQUE,
    INFO_TYPE_SDID,
    INFO_TYPE_SERVER_ID,
    INFO_TYPE_SCALE_TYPE,
    INFO_TYPE_SUPER_POD_ID,
    INFO_TYPE_ADDR_MODE,
    INFO_TYPE_RUN_MACH,
    INFO_TYPE_CURRENT_FREQ,
    INFO_TYPE_CONFIG,
    INFO_TYPE_UCE_VA,
    INFO_TYPE_HOST_KERN_LOG,
} DEV_INFO_TYPE;

__attribute__((visibility("default"))) drvError_t halGetDeviceInfo(uint32_t devId, int32_t moduleType,
                                                                   int32_t infoType, int64_t *value);

__attribute__((visibility("default"))) int prof_drv_start(unsigned int device_id, unsigned int channel_id,
                                                          struct prof_start_para *start_para);

__attribute__((visibility("default"))) int prof_channel_read(unsigned int device_id, unsigned int channel_id,
                                                             char *out_buf, unsigned int buf_size);

__attribute__((visibility("default"))) int prof_stop(unsigned int device_id,
                                                     unsigned int channel_id);

__attribute__((visibility("default"))) int prof_channel_poll(struct prof_poll_info *out_buf, int num, int timeout);

__attribute__((visibility("default"))) int halProfDataFlush(unsigned int device_id, unsigned int channel_id,
                                                            unsigned int *data_len);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // THIRD_PARTY_ASCEND_HAL_H
