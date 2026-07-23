#!/bin/bash
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

# ---------------------------------------------------------------------------
# initialize.sh - devcontainer initializeCommand
#
# 在宿主侧、容器创建之前执行，完成三件事：
# 1. 拉取最新镜像（镜像名从 devcontainer.json 的 image 字段读取，无需重复维护）
# 2. 准备宿主 ~/.gitconfig 快照，供容器读取 Git 用户名和邮箱
# 3. 创建宿主 uv 缓存目录，避免 bind mount 的 source 路径不存在
#
# 本脚本由 Dev Containers 在宿主机执行，不应依赖容器内路径或工具。
# ---------------------------------------------------------------------------

set -euo pipefail

# 脚本所在目录即 .devcontainer/
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ---- 拉取最新镜像 ----
# 使用 python3 过滤 // 和 /* */ 注释后解析 JSON，比 sed+jq 更鲁棒地
# 处理单行注释和多行注释块。
IMAGE="$(python3 -c "
import re, json
with open('$SCRIPT_DIR/devcontainer.json') as f:
    text = re.sub(r'//.*$|/\*[\s\S]*?\*/', '', f.read(), flags=re.MULTILINE)
    print(json.loads(text)['image'])
")"
echo "==> Pulling image: $IMAGE"
docker pull "$IMAGE"

# ---- 准备 Git 配置快照 ----
# 快照文件位于仓库的 .devcontainer/ 下并被 .gitignore 排除。当前实现复制完整
# 配置文件；容器内 post-create.sh 只读取 user.name 和 user.email，不会把其它
# 配置写入容器全局 Git 配置。
if [ -f "$HOME/.gitconfig" ]; then
  cp "$HOME/.gitconfig" "$SCRIPT_DIR/.host-gitconfig"
else
  : > "$SCRIPT_DIR/.host-gitconfig"
fi

# ---- 准备缓存挂载源 ----
# devcontainer.json 将该目录 bind mount 到 /home/mindstudio/.cache。
# bind mount 的 source 必须在 docker create 前存在；initializeCommand 以宿主
# 当前用户执行，因此新建目录天然归当前用户所有。
install -d -m 0755 "$HOME/.cache/uv"
install -d -m 0755 "$HOME/.cache/ccache"
