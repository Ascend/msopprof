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

import argparse
import hashlib
import json
import logging
import shutil
import subprocess
import sys
import tempfile
import traceback
from pathlib import Path

class DependencyManager:
    """
    依赖下载管理：根据 dependencies.json 拉取源码仓(Git submodule)与二进制包(artifacts)。

    用法:
        python download_dependencies.py                  下载生产依赖：源码仓(Git submodule) + 二进制包(artifacts)
        python download_dependencies.py test             下载测试依赖：源码仓(Git submodule) + 二进制包(artifacts)
        python download_dependencies.py local            跳过所有下载：直接返回不处理        
        python download_dependencies.py -r <revision>    指定内部源码仓的 Git 分支/标签/commit

    参数说明:
        - 参数: command : 执行模式: 为空时下载生产依赖, test 为下载测试依赖, local 为跳过下载。
        - 参数: -r, --revision : 指定 Git 修订版本或标签用于依赖检出。
    """

    def __init__(self, args):
        self.args, self.root = args, Path(__file__).resolve().parent
        self.config = json.loads((self.root / "dependencies.json").read_text())
        self.mode = "test" if "test" in args.command else "prod"
        self.is_local = "local" in args.command

    def _exec_shell_cmd(self, cmd, cwd=None, msg=None):
        if msg: logging.info(msg)
        try:
            subprocess.run(cmd, cwd=cwd, check=True)
        except Exception as e:
            logging.error(f"Error executing command: {' '.join(cmd)}")
            raise e

    def _download_submodule_recursively(self, path):
        mod_dir = self.root / path
        script = mod_dir / "download_dependencies.py"
        if not script.exists(): return

        logging.info(f"Download submodule: {path}")
        if self.args.revision:
            self._exec_shell_cmd(["git", "fetch", "--tags", "--all"], cwd=mod_dir)
            self._exec_shell_cmd(["git", "checkout", self.args.revision], cwd=mod_dir)

        cmd = [sys.executable, script.name] + (
            ["-r", self.args.revision] if self.args.revision else []) + self.args.command
        self._exec_shell_cmd(cmd, cwd=mod_dir)

    def proc_submodule(self, submodules):
        logging.info("=== Download git submodules start ===")
        third = [m for m in submodules if "third" in m]
        builtin = [m for m in submodules if m not in third]
        base = ["git", "submodule", "update", "--init", "--progress", "--depth=1", "--jobs=4"]

        if third:
            self._exec_shell_cmd(base + ["--recursive"] + third, msg="Fetching third-party submodules...")
        if builtin:
            self._exec_shell_cmd(base + ["--remote"] + builtin, msg="Fetching built-in submodules...")
            for m in builtin:
                self._download_submodule_recursively(m)
        logging.info("=== Download git submodules end ===")

    def proc_artifact(self, artifacts, spec):
        logging.info("=== Download artifacts start ===")
        for name in artifacts:
            target = self.root / spec[name]["path"]
            if target.exists() and any(target.iterdir()):
                logging.info(f"Skip existing: {name}")
                continue

            url, sha = spec[name]["url"], spec[name].get("sha256")
            with tempfile.TemporaryDirectory() as td:
                archive_path = Path(td) / Path(url).name
                self._exec_shell_cmd(["curl", "-Lfk", "--retry", "5", "--retry-delay", "2",
                                      "-o", str(archive_path), url], msg=f"Download {name} ...")
                if sha and hashlib.sha256(archive_path.read_bytes()).hexdigest() != sha:
                    sys.exit(f"SHA256 mismatch for {name}")

                extract_path = Path(td) / "extract"
                try:
                    extract_path.mkdir(parents=True, exist_ok=True)
                    self._exec_shell_cmd(["tar", "-xf", str(archive_path), "-C", str(extract_path)],
                                         msg=f"Unzip {name}, please wait...")
                except Exception as e:
                    logging.warning(f"tar exec fail, falling back to shutil.unpack_archive, err:{e}")
                    shutil.unpack_archive(archive_path, extract_path)  # tar解压更快，如果失败，再用此接口保护解压

                items = list(extract_path.iterdir())
                source = items[0] if len(items) == 1 and items[0].is_dir() else extract_path  # 处理常见“单一顶层目录”情况

                if target.exists():
                    shutil.rmtree(target)
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.move(str(source), str(target))
                logging.info(f"Download {name} successfully!")
        logging.info("=== Download artifacts end ===")

    def run(self):
        if self.is_local:
            logging.info("Local mode: Skipping downloads.")
            return

        submodules = self.config["dependency_sets"][self.mode].get("submodules", [])
        artifacts = self.config["dependency_sets"][self.mode].get("artifacts", [])
        spec = self.config.get("artifact_spec", {})

        if artifacts:
            self.proc_artifact(artifacts, spec)

        if submodules:
            self.proc_submodule(submodules)

def main():
    logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(levelname)s] %(message)s')
    parser = argparse.ArgumentParser(description='Download project dependencies (git submodules and artifacts) based on dependencies.json')
    parser.add_argument('command', nargs='*', default=[], choices=[[], 'local', 'test'],
                        help='Execution mode: omit to download prod dependencies, "local" to skip downloads, "test" to download test dependencies')
    parser.add_argument('-r', '--revision', help="Specify Git revision for internal dependent repo.")
    try:
        DependencyManager(parser.parse_args()).run()
        logging.info("")
        logging.info("=" * 50)
        logging.info("  ALL DEPENDENCIES DOWNLOADED SUCCESSFULLY!   ")
        logging.info("=" * 50)
        logging.info("")
    except Exception as _:
        logging.error(f"Unexpected error: {traceback.format_exc()}")
        sys.exit(1)

if __name__ == "__main__":
    main()
