#!/usr/bin/env bash
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

# git safe-pull 的实际执行脚本，由 post-create.sh 安装到
# $HOME/.local/bin/git-safe-pull，并注册为全局 Git alias。
#
# 处理流程：
#   1. 收集当前仓库全部 skip-worktree 文件。
#   2. 临时取消标记，使 Git 能识别并 stash 这些文件的本地修改。
#   3. 执行 git pull，并原样透传 --rebase 等调用参数。
#   4. pull 成功时丢弃临时 stash，以远端版本为准；失败时保留 stash。
#   5. 通过 EXIT trap 恢复 skip-worktree 标记。
#
# 限制：本命令只处理 skip-worktree 文件，不会自动暂存其它未提交修改；其它文件
# 仍遵循 git pull 的标准冲突与保护行为。

set -euo pipefail

# git ls-files -v 以行首 S 表示 skip-worktree。使用数组保存文件名，避免 xargs
# 或多层 shell 引号破坏带空格的路径。
files=()
while IFS= read -r line; do
    files+=("${line:2}")
done < <(git ls-files -v | sed -n '/^S /p')

if [ "${#files[@]}" -eq 0 ]; then
    # 没有特殊文件时不增加额外流程，直接用当前进程执行标准 pull。
    exec git pull "$@"
fi

# 无论 pull、stash drop 或其它步骤在哪一点退出，都尽力恢复索引标记。
restore_skip_worktree() {
    git update-index --skip-worktree -- "${files[@]}" 2>/dev/null || true
}
trap restore_skip_worktree EXIT

# 取消标记后，git stash 才能识别这些文件的本地修改。
git update-index --no-skip-worktree -- "${files[@]}"

# 对比操作前后的 refs/stash，区分“没有本地修改”与“确实创建了新 stash”。
stash_before=$(git rev-parse -q --verify refs/stash 2>/dev/null || true)
git stash push -q -m "devcontainer-safe-pull" -- "${files[@]}" || true
stash_after=$(git rev-parse -q --verify refs/stash 2>/dev/null || true)
stash_created=false
if [ -n "$stash_after" ] && [ "$stash_after" != "$stash_before" ]; then
    stash_created=true
fi

if ! git pull "$@"; then
    # pull 失败时不自动 pop，避免在冲突或未完成的 merge/rebase 上叠加修改。
    # 用户可在处理 Git 状态后按提示的提交 ID 手工恢复。
    if [ "$stash_created" = true ]; then
        echo "[safe-pull] pull 失败，本地个性化修改已保存在 stash：$stash_after" >&2
    fi
    exit 1
fi

# 新 stash 固定位于栈顶；pull 成功后使用远端版本并删除该临时条目。
if [ "$stash_created" = true ]; then
    git stash drop -q 'stash@{0}'
fi

printf '[safe-pull] skip-worktree 文件已恢复：%s\n' "${files[*]}"
