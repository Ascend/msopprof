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
echo "***************Generate Coverage*****************"

CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
COVERAGE_PATH=${TOP_DIR}/build_ut/coverage

if [ -d "${COVERAGE_PATH}" ]; then
    rm -rf ${COVERAGE_PATH}
fi

mkdir -p ${COVERAGE_PATH}
cd ${TOP_DIR}/build_ut

LCOV_RC="--rc lcov_branch_coverage=1 --rc geninfo_no_exception_branch=1"

lcov -c -d test/ut/CMakeFiles/msopt_test.dir/ -o ./coverage/msopt_test.info -b ./coverage $LCOV_RC

lcov -r ./coverage/msopt_test.info '*/prof_stub*' -o ./coverage/msopt_test.info $LCOV_RC
lcov -r ./coverage/msopt_test.info '*/ascend_hal/*' -o ./coverage/msopt_test.info $LCOV_RC
lcov -r ./coverage/msopt_test.info '*/opensource/*' -o ./coverage/msopt_test.info $LCOV_RC
lcov -r ./coverage/msopt_test.info '*/test/*' -o ./coverage/msopt_test.info $LCOV_RC
lcov -r ./coverage/msopt_test.info '*c++*' -o ./coverage/msopt_test.info $LCOV_RC
lcov -r ./coverage/msopt_test.info '/usr/include/*' -o ./coverage/msopt_test.info $LCOV_RC

genhtml ./coverage/msopt_test.info -o ./coverage/report --branch-coverage

cd ${COVERAGE_PATH}
cp ../test_detail.xml ./report
tar -zcvf report.tar.gz ./report
