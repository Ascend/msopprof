#!/bin/bash
# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

SHELL_DIR="$(dirname "${BASH_SOURCE:-$0}")"
INSTALL_PATH="$(cd "${SHELL_DIR}" && pwd)"
TOTAL_RET="0"

uninstall_package() {
    local path="$1"
    local ret

    cd "${INSTALL_PATH}/${path}"
    ./uninstall.sh
    ret="$?" && [ ${ret} -ne 0 ] && TOTAL_RET="1"
    return ${ret}
}

if [ ! "$*" = "" ]; then
    cur_date=$(date +"%Y-%m-%d %H:%M:%S")
    echo "[$cur_date] [ERROR]: $*, parameter is not supported."
    exit 1
fi

exit ${TOTAL_RET}
