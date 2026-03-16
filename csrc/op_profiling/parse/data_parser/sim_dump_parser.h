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


#ifndef MSOPT_SIM_DUMP_PARSER_H
#define MSOPT_SIM_DUMP_PARSER_H

#include <map>
#include "parse/data_center/data_center.h"
#include "parse/plugin/plugin_manager.h"
#include "common/defs.h"
#include "thread_pool.h"
#include "parse/data_visualizer/sim_visualizer_config.h"
#include "parse/data_parser/parser_utils/parse_pc_code.h"

namespace Profiling {
namespace Parse {
void AnalySisFileByCore(Profiling::Parse::DataCenter &dataCenter, const SimParseContext &simParseContext,
    const CoreNameAndPreFixPair &coresNamePair, uint32_t parseThread);

void CalCulateDetail(Parse::DataCenter &dataCenter, const ChipProductType &chipProductType, uint32_t calThread);

bool ParseDumpFile(const SimParseContext &simParseContext,
    std::shared_ptr<Profiling::Parse::DataCenter> &dataCenterPtr,
    std::shared_ptr<Pc2CodeMap> &pc2codePtr, uint32_t poolSize);

bool CalCulate(const SimParseContext &simParseContext, std::map<std::string,
    std::shared_ptr<Profiling::Parse::DataCenter>> &dataMap,
    std::shared_ptr<Profiling::Parse::DataCenter> &dataCenterPtr,
    std::shared_ptr<Pc2CodeMap> &pc2codePtr, uint32_t poolSize);

void CalCulate(std::map<std::string, std::shared_ptr<Profiling::Parse::DataCenter>> &dataMap,
               const std::shared_ptr<ParsePcCode> &pc2code, bool isNeedGetPc2Code, ChipProductType chipType);

void Visualizedata(const std::string &outputPath, ChipProductType chipType, Pc2CodeMap &pc2code,
    std::shared_ptr<Profiling::Parse::DataCenter> &dataCneterPtr, uint32_t systemCores);

bool CombineCoreData(const std::map<std::string, std::shared_ptr<Profiling::Parse::DataCenter>> &dataCenterMap,
    const ChipProductType &chipProductType, std::shared_ptr<Profiling::Parse::DataCenter> &dataCenterPtr);

bool CombineCoreData(const std::map<std::string, std::shared_ptr<Profiling::Parse::DataCenter>> &dataCenterMap,
                     const Profiling::Parse::DataCenter &icacheDataCenter,
                     std::shared_ptr<Profiling::Parse::DataCenter> &dataCenterPtr);

std::set<uint64_t> CollectPcFromCore(const std::shared_ptr<Profiling::Parse::DataCenter> &dataCenterPtr);
}
}
#endif // MSOPT_SIM_DUMP_PARSER_H
