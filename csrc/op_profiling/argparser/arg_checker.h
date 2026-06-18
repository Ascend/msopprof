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

#ifndef __MSOPPROF_ARGPARSER_ARG_CHECKER_H__
#define __MSOPPROF_ARGPARSER_ARG_CHECKER_H__

#include <vector>

#include "common/prof_args.h"

namespace Parser {
using AicMetricsSupportMap = std::unordered_map<std::string, std::vector<ChipProductType>>;

class ArgChecker {
public:
    using CheckFunc = bool (ArgChecker::*)(Common::ProfArgs const&, std::string &) const;

    explicit ArgChecker(const std::string &runMode);
    bool Check(Common::ProfArgs const &config, std::string &msg) const;

private:
    bool CheckRunModeValid(Common::ProfArgs const &config, std::string &msg) const;
    bool CheckApplicationValid(Common::ProfArgs const &args, std::string &msg) const;
    bool CheckOutputPathValid(Common::ProfArgs const &config, std::string &msg) const;
    bool CheckKernelNameValid(Common::ProfArgs const &config, std::string &msg) const;
    bool CheckLaunchCount(const Common::ProfArgs &config, std::string &msg) const;
    bool CheckLaunchSkipBeforeMatch(const Common::ProfArgs &config, std::string &msg) const;
    bool CheckReplayMode(const Common::ProfArgs &config, std::string &msg) const;
    bool CheckExportPathValid(Common::ProfArgs const &config, std::string &msg) const;
    bool CheckKillAdvance(const Common::ProfArgs &config, std::string &msg) const;
    bool CheckMstx(Common::ProfArgs const &config, std::string &msg) const;
    bool CheckMstxInclude(Common::ProfArgs const &config, std::string &msg) const;
    bool CheckAicMetrics(const Common::ProfArgs &config, std::string &msg) const;
    bool CheckSimSocVersion(const Common::ProfArgs &config, std::string &msg) const;
    bool CheckWarmUp(const Common::ProfArgs &config, std::string &msg) const;
    bool CheckCoreId(const Common::ProfArgs &config, std::string &msg) const;
    bool CheckTimeout(const Common::ProfArgs &config, std::string &msg) const;
    bool CheckMetrics(const std::vector<std::string> &metricsVec, const ChipProductType &productType,
                      const AicMetricsSupportMap &supports, std::string &msg) const;
    bool CheckDump(const Common::ProfArgs &config, std::string &msg) const;
    bool CheckInstrTimelinePipe(const Common::ProfArgs &config, std::string &msg) const;
    bool CheckCustomInput(Common::ProfArgs const &config, std::string &msg) const;
    bool CheckDeviceChipSupport(
        const std::string &argName, const std::vector<ChipProductType> &supportTypes, std::string &msg) const;

private:
    std::vector<CheckFunc> checkers_;
};

} // namespace Interface

#endif  // __MSOPPROF_ARGPARSER_ARG_CHECKER_H__
