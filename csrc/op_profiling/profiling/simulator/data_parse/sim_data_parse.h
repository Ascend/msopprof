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


#ifndef __MSOPPROF_PROFILING_OP_SIM_ANALYSIS_H__
#define __MSOPPROF_PROFILING_OP_SIM_ANALYSIS_H__

#include "common/prof_args.h"
#include "profiling/op_prof_data_parse.h"
#include "sim_defs.h"
#include "sim_dump_parser_task.h"
#include "parse/data_parser/sim_dump_parser.h"
#include "parse/data_parser/parser_utils/parse_pc_code.h"
#include "common/defs.h"

namespace Profiling {

float GetMicrosecond(ChipProductType chipType,  int64_t cycles, int roundParam = 3);

class SimDataParse : public DataParse {
public:
    SimDataParse(std::string socVersion, std::string exportPath, std::string coreId,
                 Common::ProfMetricsAbilityConfig metrics, bool dump = false);
    bool Execute(std::string dataPath) override;
    std::string socVersion_;
    std::string coreId_;

private:
    void DisposeTmp(const std::string &dumpPath) override;
    bool GetObjectOutPathAndCopyAicoreFile(const std::string &path, std::string &outPath) const;
    void HandleDumpLog(const std::string &tmpOutputPath) const;
    bool ParseKernelData(const std::string &path);
    void ParseKernelFile(const std::string &kernelDir, const std::string &kernelName,
                         const std::string &deviceId) override;
    bool ParseExportDumpFile(const std::string &dataPath);
    bool ParseMergeDumpData(const std::string &dumpPath, std::string &relocObjectPath,
                               const std::string &simulatorPath);
    bool CheckKernelFiles(const std::string &path, std::vector<std::string> &fileNames,
                          std::string &errorMsg) const;
    std::string exportPath_;
    bool dump_;
};
}

#endif  // __MSOPPROF_PROFILING_OP_SIM_ANALYSIS_H__
