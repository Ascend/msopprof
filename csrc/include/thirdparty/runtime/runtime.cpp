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

#include "runtime.h"

__attribute__((visibility("default"))) rtError_t rtSetDevice(int32_t device)
{
    return 0;
}

__attribute__((visibility("default"))) rtError_t rtDeviceReset(int32_t device)
{
    return 0;
}

__attribute__((visibility("default"))) rtError_t rtMalloc(void **devPtr, uint64_t size, rtMemType_t type)
{
    return 0;
}

__attribute__((visibility("default"))) rtError_t rtFree(void *devPtr)
{
    return 0;
}
__attribute__((visibility("default"))) rtError_t rtMallocHost(void **hostPtr, uint64_t size)
{
    return 0;
}
__attribute__((visibility("default"))) rtError_t rtFreeHost(void *hostPtr)
{
    return 0;
}

__attribute__((visibility("default"))) rtError_t rtMemcpy(void *dst, uint64_t destMax, const void *src,
                                                          uint64_t count, rtMemcpyKind_t kind)
{
    return 0;
}

__attribute__((visibility("default"))) rtError_t rtStreamCreate(rtStream_t *stream, int32_t priority)
{
    return 0;
}

__attribute__((visibility("default"))) rtError_t rtStreamSynchronize(rtStream_t stream)
{
    return 0;
}

__attribute__((visibility("default"))) rtError_t rtStreamDestroy(rtStream_t stream)
{
    return 0;
}

__attribute__((visibility("default"))) rtError_t rtDevBinaryRegister(const rtDevBinary_t *bin, void **handle)
{
    return 0;
}

__attribute__((visibility("default"))) rtError_t rtDevBinaryUnRegister(void *handle)
{
    return 0;
}

__attribute__((visibility("default"))) rtError_t rtFunctionRegister(void *binHandle, const void *stubFunc,
    const char *stubName, const void *devFunc, uint32_t funcMode)
{
    return 0;
}

__attribute__((visibility("default"))) rtError_t rtKernelLaunch(const void *stubFunc, uint32_t blockDim, void *args,
                                                                uint32_t argsSize, rtSmDesc_t *smDesc, void *stream)
{
    return 0;
}

__attribute__((visibility("default"))) rtError_t rtGetC2cCtrlAddr(uint64_t *addr, uint32_t *fftsLen)
{
    return 0;
}

__attribute__((visibility("default"))) rtError_t rtKernelLaunchWithHandleV2(void *hdl, const uint64_t tilingKey,
                                                                            uint32_t blockDim, rtArgsEx_t *argsInfo,
                                                                            rtSmDesc_t *smDesc, rtStream_t stm,
                                                                            const rtTaskCfgInfo_t *cfgInfo)
{
    return 0;
}

__attribute__((visibility("default"))) rtError_t rtRegisterAllKernel(const rtDevBinary_t *bin, void **hdl)
{
    return 0;
}