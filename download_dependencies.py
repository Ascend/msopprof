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
import argparse


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

class WorkingDir:
    def __init__(self, path):
        self._path = path

    def __enter__(self):
        self._current = os.getcwd()
        os.chdir(self._path)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        os.chdir(self._current)
    
    
def exec_cmd(cmd):
    result = subprocess.run(cmd, capture_output=False, text=True, timeout=36000)
    if result.returncode != 0:
        logging.error("execute command %s failed, please check the log", " ".join(cmd))
        sys.exit(result.returncode)


def create_download_cmd(args):
    cmd = ["python3", "download_dependencies.py"]
    if args.revision is not None:
        cmd.extend(["-r", args.revision])
    cmd.extend(args.command)
    return cmd


def partition(items, predicate):
    t_items = []
    f_items = []
    for item in items:
        if predicate(item):
            t_items.append(item)
        else:
            f_items.append(item)
    return t_items, f_items


def update_builtin_submodule(args, submodule):
    logging.info(f"update submodule {submodule} of {os.getcwd()}")
    if args.revision is not None:
        with WorkingDir(submodule):
            exec_cmd(["git", "fetch", "--tags", "--all"])
            exec_cmd(["git", "checkout", args.revision])

    with WorkingDir(submodule):
        if not os.path.exists("download_dependencies.py") or not os.path.isfile("download_dependencies.py"):
            logging.info(f"download_dependencies.py not exists in {submodule}")
            return
        exec_cmd(create_download_cmd(args))


def update_submodule(args):
    logging.info("============ start download thirdparty code using git submodule ============")

    dependencies = build_test_dependencies if 'test' in args.command else build_tool_dependencies
    thirdparty_dependencies, builtin_dependencies = partition(dependencies, lambda x: "thirdparty" in x or "third-party" in x)
    if thirdparty_dependencies:
        exec_cmd(["git", "submodule", "update", "--init", "--progress", "--recursive", "--depth=1", "--jobs=4", *thirdparty_dependencies])
    if builtin_dependencies:
        exec_cmd(["git", "submodule", "update", "--init", "--progress", "--remote", "--depth=1", "--jobs=4", *builtin_dependencies])
    for dependency in builtin_dependencies:
        update_builtin_submodule(args, dependency)

    logging.info("============ download thirdparty code  success ============")


def create_arg_parser():
    parser = argparse.ArgumentParser(description='Download dependencies with optional testing')
    parser.add_argument('command', nargs='*', default=[],
                        choices=[[], 'local', 'test'],
                        help="Execution mode. Local stands for without download. Test stands for building test case.")
    parser.add_argument('-r', '--revision',
                        help="Build with specific revision or tag")
    return parser


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    parser = create_arg_parser()
    args = parser.parse_args()
    current_dir = os.path.abspath(os.path.dirname(os.path.realpath(__file__)))
    os.chdir(current_dir)

    # 解析入参是否为local，非local场景时按需更新代码；local场景不更新代码只使用本地代码
    if 'local' not in args.command:
        update_submodule(args)
