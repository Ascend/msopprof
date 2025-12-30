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


#ifndef THIRD_PARTY_RUNTIME_H
#define THIRD_PARTY_RUNTIME_H
#include <cstdint>
extern "C" {
using rtStream = void *;
// error code
using rtError_t = uint32_t;
using rtMemType = uint32_t;
// l2 memory usage control
using rtSmDesc_t = void;
// related stream
using rtStream_t = void *;
typedef struct rtHostInputInfo {
    uint32_t addrOffset;
    uint32_t dataOffset;
} rtHostInputInfo_t;
typedef struct tagRtArgsEx {
    void *args;                     // args host mem addr
    rtHostInputInfo_t *hostInputInfoPtr;     // nullptr means no host mem input
    uint32_t argsSize;              // input + output + tiling addr size + tiling data size + host mem
    uint32_t tilingAddrOffset;      // tiling addr offset
    uint32_t tilingDataOffset;      // tiling data offset
    uint16_t hostInputInfoNum;      // hostInputInfo num
    uint8_t hasTiling;              // if has tiling: 0 means no tiling
    uint8_t isNoNeedH2DCopy;        // is no need host to device copy: 0 means need H2D copy,
    // others means doesn't need H2D copy.
    uint8_t reserved[4];
} rtArgsEx_t;
typedef struct rtArgsSizeInfo {
    void *infoAddr; /* info : atomicIndex|input num input offset|size|size */
    uint32_t atomicIndex;
} rtArgsSizeInfo_t;

typedef struct TagRtDevBinary {
    uint32_t magic; /**< magic number */
    uint32_t version; /**< version of binary */
    const void *data; /**< binary data */
    uint64_t length; /**< binary length */
} rtDevBinary_t;

typedef struct TagRtTaskCfgInfo {
    uint8_t qos;
    uint8_t partId;
    uint8_t schemMode; // rtschemModeType_t 0:normal;1:batch;2:sync
    uint8_t res[1]; // res
} rtTaskCfgInfo_t;
/// struct define
typedef enum TagRtMemcpyKind {
    RT_MEMCPY_HOST_TO_HOST = 0, /* host to host */
    RT_MEMCPY_HOST_TO_DEVICE, /* host to device */
    RT_MEMCPY_DEVICE_TO_HOST, /* device to host */
    RT_MEMCPY_DEVICE_TO_DEVICE, /* device to device */
    RT_MEMCPY_MANAGED, /* managed memory */
    RT_MEMCPY_ADDR_DEVICE_TO_DEVICE,
    RT_MEMCPY_HOST_TO_DEVICE_EX, // host to device ex (only used for 8 bytes)
    RT_MEMCPY_DEVICE_TO_HOST_EX, // device to host ex
    RT_MEMCPY_RESERVED,
} rtMemcpyKind_t;

// memory type
typedef enum TagRtMemType {
    RT_MEMORY_DEFAULT = 0x0, /* default memory on device */
    RT_MEMORY_HBM = 0x1, /* GM memory on device */
    RT_MEMORY_DDR = 0x2, /* DDR memory on device */
    RT_MEMORY_SPM = 0x3, /* shared physical memory on device */
    RT_MEMORY_P2P_HBM = 0x10,
    RT_MEMORY_P2P_DDR = 0x11,
    RT_MEMORY_DDR_NC = 0x20,
    RT_MEMORY_RESERVED,
} rtMemType_t;

typedef enum TagRtDeviceMode {
    RT_DEVICE_MODE_SINGLE_DIE = 0,
    RT_DEVICE_MODE_MULTI_DIE,
    RT_DEVICE_MODE_RESERVED
} rtDeviceMode;

typedef enum TagRtDeviceStatus {
    RT_DEVICE_STATUS_NORMAL = 0,
    RT_DEVICE_STATUS_ABNORMAL,
    RT_DEVICE_STATUS_END = 0xFFFF
} rtDeviceStatus;

constexpr uint32_t rtDevBinaryMagicElf = 0x43554245U;
constexpr uint32_t rtDevBinaryMagicElfAivec = 0x41415246U;
constexpr uint32_t rtDevBinaryMagicElfAicube = 0x41494343U;

rtError_t rtGetDevice(int32_t* devId);
rtError_t rtSetDevice(int32_t device);
rtError_t rtSetDeviceV2(int32_t devId, rtDeviceMode deviceMode);
rtError_t rtDeviceReset(int32_t device);
rtError_t rtSetDeviceEx(int32_t device);
rtError_t rtCtxCreate(void **createCtx, uint32_t flags, int32_t devId);
rtError_t rtCtxCreateV2(void **createCtx, uint32_t flags, int32_t devId, rtDeviceMode deviceMode);
rtError_t rtCtxCreateEx(void **ctx, uint32_t flags, int32_t device);
rtError_t rtDeviceStatusQuery(const uint32_t devId, rtDeviceStatus *deviceStatus);
rtError_t rtMalloc(void **devPtr, uint64_t size, rtMemType_t type);
rtError_t rtFree(void *devPtr);
rtError_t rtMallocHost(void **hostPtr, uint64_t size);
rtError_t rtFreeHost(void *hostPtr);
rtError_t rtMemcpy(void *dst, uint64_t destMax, const void *src, uint64_t count, rtMemcpyKind_t kind);
rtError_t rtMemcpyAsync(void *dst, uint64_t destMax, const void *src, uint64_t count,
                        rtMemcpyKind_t kind, rtStream_t stream);
rtError_t rtStreamCreate(rtStream_t *stream, int32_t priority);
rtError_t rtStreamDestroy(rtStream_t stream);
rtError_t rtDevBinaryUnRegister(void *handle);
rtError_t rtGetC2cCtrlAddr(uint64_t *addr, uint32_t *fftsLen);
rtError_t rtStreamSynchronize(rtStream_t stream);
rtError_t rtFunctionRegister(void *binHandle, const void *stubFunc,
                             const char *stubName, const void *devFunc, uint32_t funcMode);
rtError_t rtKernelLaunchWithHandleV2(void *hdl, const uint64_t tilingKey, uint32_t blockDim, rtArgsEx_t *argsInfo,
                                     rtSmDesc_t *smDesc, rtStream_t stm, const rtTaskCfgInfo_t *cfgInfo);
rtError_t rtKernelLaunchWithFlagV2(const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc,
                                   rtStream_t stm, uint32_t flags, const rtTaskCfgInfo_t *cfgInfo);
rtError_t rtRegisterAllKernel(const rtDevBinary_t *bin, void **hdl);
rtError_t rtKernelLaunch(const void *stubFunc, uint32_t blockDim,
                         void *args, uint32_t argsSize, rtSmDesc_t *smDesc, rtStream_t stream);
rtError_t rtDevBinaryRegister(const rtDevBinary_t *bin, void **handle);
rtError_t rtGetSocVersion(char *version, const uint32_t maxLen);
rtError_t rtProfSetProSwitch(void *data, uint32_t len);
rtError_t rtSetExceptionExtInfo(const rtArgsSizeInfo_t * const sizeInfo);
rtError_t rtGetVisibleDeviceIdByLogicDeviceId(const int32_t logicalDevId, int32_t *const visibleDevId);
}
#endif // THIRD_PARTY_RUNTIME_H
