/* -------------------------------------------------------------------------
 *  This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include <cstring>
#include <iostream>
#include <unistd.h>
#include "cli_logo.h"

namespace Interface {

constexpr const char *kReset = "\033[0m";
constexpr const char *kDimGray = "\033[38;5;240m";
constexpr const char *kBoldWhite = "\033[1;97m";
constexpr const char *kHighlight = "\033[48;5;21;38;5;46m"; // green on blue

bool ShouldUseColorLogo() {
    if (!isatty(STDERR_FILENO)) {
        return false;
    }
    const char *term = std::getenv("TERM");
    return term && strcmp(term, "dumb") != 0 && strcmp(term, "unknown") != 0;
}

void PrintLogo() {
    if (!ShouldUseColorLogo()) {
        std::cerr << "=================================================================" << "\n"
                  << "                   >>>>>   MindStudio   <<<<<" << "\n"
                  << "    THE END-TO-END TOOLCHAIN TO UNLEASH HUAWEI ASCEND COMPUTE" << "\n"
                  << "=================================================================" << "\n"
                  << std::endl;
        return;
    }

    std::cerr << kDimGray << "=================================================================" << kReset << "\n"
              << kBoldWhite << "                   >>>>>  " << kHighlight << " MindStudio " << kReset << kBoldWhite
              << "  <<<<<" << kReset << "\n"
              << kBoldWhite << "    THE END-TO-END TOOLCHAIN TO UNLEASH HUAWEI ASCEND COMPUTE" << kReset << "\n"
              << kDimGray << "=================================================================" << kReset << "\n"
              << std::endl;
}
} // namespace Interface
