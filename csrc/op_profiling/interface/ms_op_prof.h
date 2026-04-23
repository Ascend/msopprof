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

#ifndef __MSOPPROF_INTERFACE_MS_OP_PROF_H__
#define __MSOPPROF_INTERFACE_MS_OP_PROF_H__

#include "common/prof_args.h"
#include "common/defs.h"

namespace Interface {

bool ProfArgsInit(Common::ProfArgs &args, int argc, char *argv[], char *env[]);

void PrintDeviceHelp(Common::ChipType chipType);

void PrintVersion();

void PrintSimulatorHelp(void);

bool ProfilingRun(const Common::ProfArgs &args);

bool IsProcessRunning();

void SetExitMode();
} // namespace Interface

#endif  // __MSOPPROF_INTERFACE_MS_OP_PROF_H__
