#!/usr/bin/python3
# -*- coding: utf-8 -*-
# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2025 Huawei Technologies Co.,Ltd.
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

import os
import sys
import logging
import subprocess
import multiprocessing
import argparse
import tarfile
import glob
import shutil


def exec_cmd(cmd):
    result = subprocess.run(cmd, capture_output=False, text=True, timeout=3600)
    if result.returncode != 0:
        logging.error("execute command %s failed, please check the log", " ".join(cmd))
        sys.exit(result.returncode)


build_test_dependencies = [
    "thirdparty/json",
    "thirdparty/securec",
    "thirdparty/googletest",
    "thirdparty/mockcpp",
    "thirdparty/boost",
    "thirdparty/llvm-project",
    "msopscommon"
]

build_tool_dependencies = [
    "thirdparty/json",
    "thirdparty/securec",
    "thirdparty/makeself",
    "thirdparty/llvm-project",
    "msopscommon"
]


def update_msopscommon(args):
    if args.revision is None:
        exec_cmd(["git", "submodule", "update", "--init", "--depth=1", "--jobs=4", "msopscommon"])
    else:
        os.chdir("msopscommon")
        exec_cmd(["git", "fetch", "--tags"])
        exec_cmd(["git", "checkout", args.revision])
        os.chdir("..")
    if 'test' not in args.command:
        os.chdir("msopscommon")
        exec_cmd(["git", "submodule", "update", "--init", "--depth=1", "--jobs=4", "thirdparty/json"])


def update_submodle(args):
    logging.info("============ start download thirdparty code using git submodule ============")
    if 'test' in args.command:
        exec_cmd(["git", "submodule", "update", "--init", "--recursive", "--depth=1", "--jobs=4", *build_test_dependencies])
    else:
        exec_cmd(["git", "submodule", "update", "--init", "--depth=1", "--jobs=4", *build_tool_dependencies])
    update_msopscommon(args)
    logging.info("============ download thirdparty code  success ============")


def execute_build(build_path, cmake_cmd, make_cmd):
    if not os.path.exists(build_path):
        os.makedirs(build_path, mode=0o755)
    os.chdir(build_path)
    exec_cmd(cmake_cmd)
    exec_cmd(make_cmd)


def execute_test(build_path, test_cmd):
    os.chdir(build_path)
    if test_cmd != []:
        os.environ['MSOPT_UT_RESOURCE'] = os.getcwd() + "test/ut/resources"
        os.environ['LD_LIBRARY_PATH'] = os.getcwd() + os.pathsep + os.environ.get('LD_LIBRARY_PATH', '')
        exec_cmd(test_cmd)


def create_arg_parser():
    parser = argparse.ArgumentParser(description='Build script with optional testing')
    parser.add_argument('command', nargs='*', default=[],
                        choices=[[], 'local', 'test'],
                        help='Command to execute (python build.py [ |local|test])')
    parser.add_argument('-r', '--revision',
                        help="Build with specific revision or tag")
    return parser


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    parser = create_arg_parser()
    args = parser.parse_args()
    
    currentDir = os.path.abspath(os.path.dirname(os.path.realpath(__file__)))
    os.chdir(currentDir)
    cpuCores = multiprocessing.cpu_count()

    build_path = os.path.join(currentDir, "build")
    target = "all"
    cmake_cmd = ["cmake", "../cmake"]
    make_cmd = ["make", "-j", str(cpuCores)]
    test_cmd = []

    # ut使用单独的目录构建，与build区分开，避免相互影响，并传入对应的参数
    if 'test' in args.command:
        build_path = os.path.join(currentDir, "build_ut")
        cmake_cmd = ["cmake", "..", "-DBUILD_TESTS=ON"]
        make_cmd.append("install")
        test_cmd = ["./test/ut/msopt_test", "--gtest_output=xml:test_detail.xml"]
 
    # 解析入参是否为local，local场景使用本地代码构建，不更新子仓
    if 'local' not in args.command:
        update_submodle(args)

    # 执行构建并打run包
    execute_build(build_path, cmake_cmd, make_cmd)
    # 执行测试
    execute_test(build_path, test_cmd)
