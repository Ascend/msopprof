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


#ifndef MSOPT_ARG_NORMALIZE_H
#define MSOPT_ARG_NORMALIZE_H

#include "common/prof_args.h"
namespace Parser {
class ArgNormalize {
public:
    using NormalizeFunc = bool (ArgNormalize::*)(Common::ProfArgs &, std::string &) const;
    ArgNormalize(void);
    bool Normalize(Common::ProfArgs &config, std::string &msg) const;
private:
    bool NormalizeKernelName(Common::ProfArgs &config, std::string &msg) const;
    bool NormalizeMstxMessage(Common::ProfArgs &config, std::string &msg) const;
    bool NormalizeApp(Common::ProfArgs &config, std::string &msg) const;
    bool NormalizeOutput(Common::ProfArgs &config, std::string &msg) const;
    bool NormalizeExport(Common::ProfArgs &config, std::string &msg) const;
    bool NormalizeConfig(Common::ProfArgs &config, std::string &msg) const;
    bool NormalizeCustomInput(Common::ProfArgs &config, std::string &msg) const;
    bool NormalizeOptionPath(std::string &configOpt, std::string &msg, std::string msgOption) const;

    std::vector<NormalizeFunc> normalizeFunc_;
};
} // namespace Parser

#endif // MSOPT_ARG_NORMALIZE_H
