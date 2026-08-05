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

# =============================================================================
# MindStudio devcontainer 初始化脚本
# =============================================================================
#
# 职责说明：
#   该脚本在 devcontainer 首次创建或重建后自动执行，负责完成用户级开发环境
#   初始化，确保开发者进入容器后即可直接进入编码、编译和调试状态。
#
# 与 devcontainer.json 的关系：
#   devcontainer.json 通过 postCreateCommand 调用本脚本：
#     "postCreateCommand": "bash -lc '/workspace/.devcontainer/post-create.sh'"
#   使用 login shell（-l）执行，目的是在初始化过程中自动加载镜像
#   /etc/profile.d/ 中的 CANN、GCC 11、Python 等环境脚本，确保后续配置
#   能够感知到这些工具链的存在。
#
# 执行顺序（按依赖关系排列）：
#   1. configure_yum_mirror    - 切换至 HTTP 协议的华为云 openEuler 软件源
#   2. fix_cache_ownership     - 修复 z_cache.sh 创建的缓存目录权限
#   3. fix_file_watcher_limit  - 提升 inotify max_user_watches 至 524288
#   4. configure_user_bin      - 建立用户级命令目录，重映射 npm prefix
#   5. ensure_shared_bin_path  - 补齐 root 用户的共享命令路径
#   6. configure_python311     - 在 shell 启动文件中启用 Python 3.11
#   7. sync_git_identity       - 从宿主同步 Git 用户名和邮箱
#   8. append_dev_hint_once    - 向 .bash_profile 追加常用开发命令提示
#   9. install_pre_commit_hook - 自动安装 pre-commit Git Hook
#   10. warmup_pre_commit_async - 后台预创建 pre-commit Hook 环境
#   11. ignore_vscode_settings - 隔离个人化 VS Code settings 修改
#   12. install_git_safe_pull_alias - 安装可处理 skip-worktree 文件的拉取命令
#   13. verify_ccache          - 验证 ccache 编译缓存挂载与权限
#   14. setup_clangd           - 检查/安装 clangd，验证编译数据库配置
#
# =============================================================================

# 不使用 set -e：各模块自行降级并记录告警，单项失败不应阻止进入容器。
# -u 和 pipefail 仍用于暴露未定义变量及管道中的隐蔽错误。
set -uo pipefail

# ---------------------------------------------------------------------------
# 工具函数
# ---------------------------------------------------------------------------

# 输出统一格式的信息日志，便于从 Dev Containers 启动日志中筛选。
log() {
    printf '[post-create] %s\n' "$*"
}

# 输出统一格式的告警日志到 stderr；告警默认不终止后续初始化。
warn() {
    printf '[post-create] warning: %s\n' "$*" >&2
}

# 以统一格式执行并展示初始化步骤。即使某一步失败也继续后续流程，避免单项
# 非关键配置阻止开发者进入容器；耗时用于识别初始化过程中的慢步骤。
run_step() {
    local step_number="$1"
    local total_steps="$2"
    local step_title="$3"
    local step_function="$4"
    local step_label
    local exit_code
    local started_at=$SECONDS
    local elapsed

    printf -v step_label '%02d/%02d' "$step_number" "$total_steps"
    printf '\n'
    log "------------------------------------------------------------------------"
    log "[STEP $step_label] START | $step_title"

    if "$step_function"; then
        elapsed=$((SECONDS - started_at))
        log "[STEP $step_label] DONE  | $step_title (${elapsed}s)"
    else
        exit_code=$?
        elapsed=$((SECONDS - started_at))
        warn "[STEP $step_label] FAILED | $step_title (${elapsed}s, exit: $exit_code)"
    fi

    # run_step 自身始终成功，确保某一步失败后仍执行剩余初始化任务。
    return 0
}

# =============================================================================
# configure_yum_mirror —— 配置华为云 openEuler 软件源
# =============================================================================
#
# 背景与问题：
#   基础镜像默认使用 https://repo.openeuler.org，部分开发网络访问该地址较慢。
#   华为云 openEuler 镜像站提供相同的仓库目录结构，可直接保留当前发行版、
#   仓库分区和 $basearch 路径，仅替换站点前缀。
#
# 解决方案：
#   将 openEuler 官方源及 HTTPS 华为云源统一替换为：
#     http://mirrors.huaweicloud.com/openeuler
#   修改前保留一次原始 repo 文件备份；仅在配置发生变化时清理 dnf/yum 元数据。
#   本地 file:// GPG Key 配置保持不变，软件包签名校验仍然启用。
configure_yum_mirror() {
    local repo_dir="/etc/yum.repos.d"
    local mirror_base="http://mirrors.huaweicloud.com/openeuler"
    local repo_files=("$repo_dir"/*.repo)
    local repo_file
    local backup_file
    local changed=0

    if [ ! -e "${repo_files[0]}" ]; then
        warn "no yum repo files found in $repo_dir; skipping mirror configuration"
        return 0
    fi

    for repo_file in "${repo_files[@]}"; do
        if ! grep -Eq 'https?://repo\.openeuler\.org|https://mirrors\.huaweicloud\.com/openeuler|https?://repo\.huaweicloud\.com/openeuler' "$repo_file"; then
            continue
        fi

        backup_file="${repo_file}.post-create.bak"
        if [ ! -e "$backup_file" ]; then
            sudo cp -a "$repo_file" "$backup_file" || {
                warn "failed to back up yum repo file: $repo_file"
                return 1
            }
        fi

        sudo sed -E -i \
            -e "s#https?://repo\.openeuler\.org#${mirror_base}#g" \
            -e "s#https://mirrors\.huaweicloud\.com/openeuler#${mirror_base}#g" \
            -e "s#https?://repo\.huaweicloud\.com/openeuler#${mirror_base}#g" \
            "$repo_file" || {
                warn "failed to update yum repo file: $repo_file"
                return 1
            }
        changed=1
    done

    if ! grep -Eq '^[[:space:]]*baseurl=http://mirrors\.huaweicloud\.com/openeuler/' "${repo_files[@]}"; then
        warn "Huawei Cloud yum mirror was not found in active repo configuration"
        return 1
    fi

    if [ "$changed" -eq 1 ]; then
        if command -v dnf >/dev/null 2>&1; then
            sudo dnf clean all >/dev/null 2>&1 || warn "failed to clean dnf metadata"
        elif command -v yum >/dev/null 2>&1; then
            sudo yum clean all >/dev/null 2>&1 || warn "failed to clean yum metadata"
        fi
        log "yum mirror changed to $mirror_base"
    else
        log "yum mirror already uses $mirror_base"
    fi

    log "configure_yum_mirror succeeded"
}

# append_path_once：
#   幂等地将 $HOME/.local/bin 放到指定 shell 启动文件的 PATH 前端。
#   解决非 root 用户（mindstudio）无法写入 /usr/local/nodejs 等系统级目录的问题。
#   参数 $1: 目标 shell 启动文件路径（如 $HOME/.bashrc 或 $HOME/.bash_profile）
append_path_once() {
    local file="$1"
    local line='export PATH="$HOME/.local/bin:$PATH"'

    touch "$file"
    if ! grep -Fqx "$line" "$file"; then
        printf '\n%s\n' "$line" >> "$file"
    fi

    log "append_path_once succeeded: $file"
}

# append_python311_once：
#   幂等地将 Python 3.11 切换脚本注入到指定 shell 启动文件中。
#   通过 marker 注释判断是否已注入，避免重复写入。
#   注入内容：
#     1. 加载镜像的 /etc/profile.d/z_python_switch.sh（Python 版本切换基础设施）
#     2. 调用 use-python 3.11 激活 Python 3.11 环境
#   参数 $1: 目标 shell 启动文件路径
append_python311_once() {
    local file="$1"
    local marker_begin="# >>> mindstudio devcontainer python >>>"

    touch "$file"
    if grep -Fqx "$marker_begin" "$file"; then
        log "append_python311_once succeeded: $file"
        return 0
    fi

    cat >> "$file" <<'EOF'

# >>> mindstudio devcontainer python >>>
if [ -r /etc/profile.d/z_python_switch.sh ]; then
    . /etc/profile.d/z_python_switch.sh
fi
if [ -r /usr/local/bin/use-python ]; then
    . /usr/local/bin/use-python 3.11 >/dev/null 2>&1 || true
    export PY311_ENV_ENABLED=1
fi
# <<< mindstudio devcontainer python <<<
EOF

    log "append_python311_once succeeded: $file"
}

# =============================================================================
# configure_user_bin —— 用户级命令与 npm 全局路径重映射
# =============================================================================
#
# 背景与问题：
#   构建镜像中的 npm 全局安装目录默认指向 /usr/local/nodejs，非 root 用户
#   （mindstudio）无写入权限。如果直接在镜像内执行 npm install -g，会因权限
#   不足而失败。
#
# 解决方案：
#   1. 创建 $HOME/.local/bin 目录，作为用户级可执行文件存放路径。
#   2. 通过 npm config set prefix 将 npm 全局安装目录重映射到 $HOME/.local，
#      这样 npm install -g 会将包安装到用户可写的目录下。
#   3. 将 $HOME/.local/bin 写入 .bashrc 和 .bash_profile 的 PATH 前端，
#      确保用户级命令优先级高于系统级命令。
#
# 容器环境变量配合：
#   devcontainer.json 中设置了 containerEnv:
#     NPM_CONFIG_PREFIX=/home/mindstudio/.local
#   这个变量在 VS Code 远端会话中生效；本函数确保在交互式终端 Login Shell 中
#   同样生效，覆盖所有使用场景。
configure_user_bin() {
    mkdir -p "$HOME/.local/bin"

    if command -v npm >/dev/null 2>&1; then
        local current_prefix
        current_prefix=$(npm config get prefix 2>/dev/null || true)
        if [ "$current_prefix" != "$HOME/.local" ]; then
            npm config set prefix "$HOME/.local" || warn "failed to set npm prefix"
        fi
    else
        warn "npm is not available; skipping npm prefix setup"
    fi

    append_path_once "$HOME/.bashrc"
    append_path_once "$HOME/.bash_profile"

    log "configure_user_bin succeeded"
}

# =============================================================================
# ensure_shared_bin_path —— 跨用户命令路径补齐
# =============================================================================
#
# 背景与问题：
#   devcontainer.json 的 containerEnv 将 NPM_CONFIG_PREFIX 硬编码为
#   /home/mindstudio/.local，这意味着 npm install -g 始终安装到 mindstudio
#   用户目录。当宿主机是 root 时，会话用户可能解析为 root（$HOME=/root），
#   其 $HOME/.local/bin 指向 /root/.local/bin，与 npm 实际安装目标不重合，
#   导致 claude 等 npm 全局命令不可见。
#
# 解决方案：
#   幂等地将 /home/mindstudio/.local/bin 追加到 root 用户的 .bashrc 和
#   .bash_profile 中，确保无论以哪个用户登录，都能找到 npm -g 安装的命令。
#   同时对已经安装的命令（如 claude）创建 /usr/local/bin 软链接作为兜底。
#
# 幂等性保证：
#   通过 grep -Fqx 检测目标行是否已存在，避免重复写入。
ensure_shared_bin_path() {
    local shared_bin="/home/mindstudio/.local/bin"
    local line="export PATH=\"$shared_bin:\$PATH\""

    # 为 root 用户补齐 mindstudio bin 路径（幂等）
    # 兼容 postCreateCommand 以 root 或非 root 身份运行的场景
    if [ -d "/root" ]; then
        for rc in "/root/.bashrc" "/root/.bash_profile"; do
            if [ "$(id -u)" = "0" ]; then
                # 当前是 root，直接操作
                touch "$rc" 2>/dev/null || continue
                if ! grep -Fqx "$line" "$rc"; then
                    printf '\n%s\n' "$line" >> "$rc"
                    log "added shared bin path to $rc"
                fi
            else
                # 当前是非 root，通过 sudo 操作
                sudo touch "$rc" 2>/dev/null || continue
                if ! sudo grep -Fqx "$line" "$rc"; then
                    printf '\n%s\n' "$line" | sudo tee -a "$rc" >/dev/null
                    log "added shared bin path to $rc"
                fi
            fi
        done
    fi

    # 兜底：将已安装的常用命令软链到系统路径，覆盖 sudo / 纯 root 等不读取
    # mindstudio rc 文件的场景
    if [ -d "$shared_bin" ]; then
        for cmd in claude; do
            if [ -x "$shared_bin/$cmd" ] && [ ! -e "/usr/local/bin/$cmd" ]; then
                if [ "$(id -u)" = "0" ]; then
                    ln -sf "$shared_bin/$cmd" "/usr/local/bin/$cmd" 2>/dev/null || true
                else
                    sudo ln -sf "$shared_bin/$cmd" "/usr/local/bin/$cmd" 2>/dev/null || true
                fi
                log "linked $cmd to /usr/local/bin/$cmd"
            fi
        done
    fi

    log "ensure_shared_bin_path succeeded"
}

# =============================================================================
# configure_python311 —— Python 3.11 环境激活
# =============================================================================
#
# 背景与问题：
#   MindStudio 构建镜像预装了多个 Python 版本，3.11 是当前开发环境的默认版本。
#   镜像内 /etc/profile.d/z_python_switch.sh 和 /usr/local/bin/use-python
#   提供了版本切换能力，但这些脚本只在 Login Shell 中自动生效。如果 shell
#   启动文件中缺少这些调用，非 Login Shell 或子进程中可能使用错误的 Python 版本。
#
# 解决方案：
#   在 .bashrc 和 .bash_profile 中注入 Python 3.11 切换逻辑，覆盖 Login Shell
#   和非 Login Shell 两种场景：
#   - .bashrc：覆盖 VS Code 集成终端（非 Login Shell）
#   - .bash_profile：覆盖 SSH / 外部终端（Login Shell）
#
# 幂等性保证：
#   通过 marker 注释 (# >>> mindstudio devcontainer python >>>) 检测是否已
#   注入，避免重复写入导致环境变量被多次定义。
configure_python311() {
    append_python311_once "$HOME/.bashrc"
    append_python311_once "$HOME/.bash_profile"

    log "configure_python311 succeeded"
}

# =============================================================================
# sync_git_identity —— 宿主 Git 身份同步
# =============================================================================
#
# 背景与问题：
#   容器内的 Git 配置是全新的，如果不做身份同步，开发者在容器内的 commit 会
#   缺少正确的 author 信息，导致提交记录与开发者身份脱钩。但出于安全考虑，
#   不应该将宿主整个 $HOME 目录暴露到容器中（避免密钥、token 等敏感文件泄漏）。
#
# 解决方案：
#   1. initialize.sh 在宿主机复制 ~/.gitconfig 到被 Git 忽略的快照文件。
#   2. devcontainer.json 通过 mounts 将该快照文件只读挂载到容器内的
#      /tmp/host-gitconfig。
#   3. 本函数从 /tmp/host-gitconfig 读取 user.name 和 user.email，
#      写入容器全局 Git 配置 (git config --global)。
#
# 降级策略：
#   - 宿主没有 ~/.gitconfig 时，initializeCommand 生成空的 .host-gitconfig。
#     本函数检测到空文件或缺少字段时只告警，不阻塞容器创建。
#   - 快照可能包含其它 Git 配置，但本函数只读取 user.name 和 user.email，
#     不会把 credential、alias、include 等设置写入容器全局配置。
sync_git_identity() {
    local host_gitconfig="/tmp/host-gitconfig"
    local git_name=""
    local git_email=""

    if [ ! -s "$host_gitconfig" ]; then
        warn "host gitconfig is empty or missing; skipping git identity sync"
        log "sync_git_identity succeeded"
        return 0
    fi

    git_name="$(git config -f "$host_gitconfig" --get user.name 2>/dev/null || true)"
    git_email="$(git config -f "$host_gitconfig" --get user.email 2>/dev/null || true)"

    if [ -n "$git_name" ]; then
        git config --global user.name "$git_name" || warn "failed to sync git user.name"
    else
        warn "host gitconfig has no user.name"
    fi

    if [ -n "$git_email" ]; then
        git config --global user.email "$git_email" || warn "failed to sync git user.email"
    else
        warn "host gitconfig has no user.email"
    fi

    log "sync_git_identity succeeded"
}

# =============================================================================
# install_pre_commit_hook —— pre-commit 自动安装
# =============================================================================
#
# 背景与问题：
#   仓库已有 .pre-commit-config.yaml 配置，但需要开发者手工执行
#   `pre-commit install` 才能生效。在传统开发模式中，这一步容易被遗漏或忘记，
#   导致提交时未触发质量检查，低质量代码进入仓库。
#
# 解决方案：
#   容器初始化时自动执行 pre-commit install，将 pre-commit Hook 安装到
#   .git/hooks/pre-commit，确保每次 git commit 时自动触发。
#
# 降级策略：
#   1. pre-commit 命令不存在时只告警，不阻塞容器创建。
#      原因：纯 Python 工具有可能不需要 pre-commit，不应因工具缺失阻止进入容器。
#   2. 当前目录不是 Git 仓库时只告警，不阻塞容器创建。
#      原因：非 Git 场景（如镜像内临时工作区）不应因 .git 缺失而失败。
#   3. 安装失败时只告警，不影响容器正常使用。
install_pre_commit_hook() {
    # pre-commit CLI 工具和 pre_commit Python 模块是分开的：
    # - CLI（/usr/local/bin/pre-commit）用于执行 pre-commit install 等管理命令
    # - Python 模块用于 git hook 运行时（hook 模板调用 /usr/bin/python3 -mpre_commit）
    # 两者都需要存在，否则 git commit 时会报 "No module named pre_commit"
    if ! command -v pre-commit >/dev/null 2>&1; then
        warn "pre-commit is not available; skipping hook installation"
        log "install_pre_commit_hook succeeded"
        return 0
    fi

    if ! python3 -c "import pre_commit" 2>/dev/null; then
        log "pre_commit Python module not found; installing..."
        python3 -m pip install pre-commit || {
            warn "failed to install pre_commit Python module; skipping hook installation"
            log "install_pre_commit_hook succeeded"
            return 0
        }
    fi

    if ! git rev-parse --git-dir >/dev/null 2>&1; then
        warn "workspace is not a git repository; skipping pre-commit hook installation"
        log "install_pre_commit_hook succeeded"
        return 0
    fi

    pre-commit install || warn "pre-commit hook installation failed"

    log "install_pre_commit_hook succeeded"
}

# =============================================================================
# warmup_pre_commit_async —— 后台预创建 pre-commit Hook 环境
# =============================================================================
#
# 背景与问题：
#   pre-commit 首次运行时需要下载 Hook 仓库并创建各自的运行环境，耗时较长。
#   如果等到开发者第一次提交时才执行，会明显阻塞提交流程。
#
# 解决方案：
#   pre-commit Hook 安装完成后，在后台执行 `pre-commit install-hooks`，并与
#   后续 clangd 安装等初始化任务并行。该命令只准备 Hook 环境，不执行检查，
#   也不会修改工作区文件。后台进程的标准输入、输出和错误均与
#   postCreateCommand 分离，避免阻塞容器初始化完成。
#
# 资源与并发控制：
#   1. 使用 nice 和 ionice 降低 CPU、磁盘调度优先级，减少对交互操作的影响。
#   2. flock 可用时持有非阻塞锁，防止脚本重复执行后同时启动多个预热任务。
#   3. 详细结果写入缓存目录中的日志，预热失败不影响容器正常使用。
warmup_pre_commit_async() {
    if ! command -v pre-commit >/dev/null 2>&1; then
        warn "pre-commit is not available; skipping async warmup"
        return 0
    fi

    local repo_root
    repo_root=$(git rev-parse --show-toplevel 2>/dev/null || true)
    if [ -z "$repo_root" ]; then
        warn "workspace is not a Git repository; skipping async warmup"
        return 0
    fi

    if [ ! -f "$repo_root/.pre-commit-config.yaml" ]; then
        warn "pre-commit config is not available; skipping async warmup"
        return 0
    fi

    local warmup_dir="$HOME/.cache"
    local log_file="$warmup_dir/pre-commit-warmup.log"
    local lock_file="$warmup_dir/pre-commit-warmup.lock"
    mkdir -p "$warmup_dir"

    (
        # 忽略启动 shell 结束时可能发送的 HUP，并断开所有终端输入输出。
        trap '' HUP
        cd "$repo_root" || exit 0

        if command -v flock >/dev/null 2>&1; then
            exec 9>"$lock_file"
            if ! flock -n 9; then
                log "pre-commit environment warmup is already running"
                exit 0
            fi
        fi

        # ionice 调整失败时继续运行；nice 在常规 Linux 环境中始终可用。
        if command -v ionice >/dev/null 2>&1; then
            ionice -c 3 -p "$$" 2>/dev/null || true
        fi

        # 确保 $HOME/.local/bin 在 PATH 前端，使用与用户交互式 shell
        # 一致的 pre-commit 二进制（Python 3.11），避免版本不匹配导致
        # 预热环境与用户实际使用的 Python 版本不一致而重新下载。
        export PATH="$HOME/.local/bin:$PATH"

        log "pre-commit environment warmup started"
        if nice -n 10 pre-commit install-hooks; then
            log "pre-commit environment warmup succeeded"
        else
            warn "pre-commit environment warmup failed"
        fi
    ) </dev/null >>"$log_file" 2>&1 &

    local warmup_pid
    warmup_pid=$!
    log "pre-commit environment warmup started in background (pid: $warmup_pid, log: $log_file)"
}

# =============================================================================
# fix_cache_ownership —— 修复缓存目录权限
# =============================================================================
#
# 背景与问题：
#   镜像内置的 /etc/profile.d/z_cache.sh 在 login shell 启动时探测 bind mount
#   路径（/workspace、/home/mindstudio 等），并自动创建 .cache/ccache 和
#   .cache/uv 目录。但 z_cache.sh 可能在 updateRemoteUserUID 完成 UID 重映射
#   之前被触发（例如通过 docker run 而非 devcontainer），或由 root 身份的
#   进程触发，导致这些目录在宿主机上显示为 root:root，与普通文件权限不一致。
#
#   典型症状：
#   - 容器内 ls -la ~/.cache → mindstudio:mindstudio（UID 已被重映射）
#   - 宿主机 ls -la ~/.cache → root:root（原始 UID 0 未变）
#
# 解决方案：
#   在 post-create 阶段主动检测并修复这些缓存目录的 owner，确保与当前用户
#   一致。优先修复当前 shell 实际导出的 CCACHE_DIR、UV_CACHE_DIR；这可覆盖
#   /home/<宿主用户> 被额外挂载进容器的场景。缓存父目录也必须可写，供
#   pre-commit、pip 等工具创建各自的同级缓存。固定路径作为兜底，覆盖
#   z_cache.sh 可能探测的工作区和容器用户目录。
#
# 幂等性保证：
#   仅在目录存在且 owner 不匹配时才执行 chown，避免不必要的文件系统操作。
fix_cache_ownership() {
    local dirs_to_fix=(
        "${HOME}/.cache"
        "${CCACHE_DIR:-}"
        "${UV_CACHE_DIR:-}"
        "/home/mindstudio/.cache"
        "/workspace/.cache/ccache"
        "/workspace/.cache/uv"
        "/home/mindstudio/.cache/ccache"
        "/home/mindstudio/.cache/uv"
    )

    for d in "${dirs_to_fix[@]}"; do
        [ -n "$d" ] || continue
        if [ -d "$d" ]; then
            local owner
            owner=$(stat -c '%U' "$d" 2>/dev/null || true)
            if [ "$owner" != "$USER" ]; then
                log "fixing ownership of $d ($owner → $USER)"
                sudo chown -R "$USER:$USER" "$d" 2>/dev/null || \
                    warn "failed to chown $d"
            fi
        fi
    done

    log "fix_cache_ownership succeeded"
}

# =============================================================================
# append_dev_hint_once —— 常用开发命令提示
# =============================================================================
#
# 目的：
#   在每次打开终端时展示常用开发命令的快捷提示，降低新开发者的学习成本，
#   让所有人都能快速知道如何编译项目和运行单元测试。
#
# 展示时机：
#   写入 $HOME/.bash_profile，在每次 Login Shell 启动时显示。
#
# 幂等性保证：
#   通过 marker 注释 (# >>> mindstudio devcontainer dev-hint >>>) 检测
#   是否已追加，避免每次容器重建都重复写入。
append_dev_hint_once() {
    local file="$HOME/.bash_profile"
    local marker="# >>> mindstudio devcontainer dev-hint >>>"

    touch "$file"
    if grep -Fqx "$marker" "$file"; then
        return 0
    fi

    cat >> "$file" <<'EOF'

# >>> mindstudio devcontainer dev-hint >>>
printf '──────────────────────────────────────────────────────────────────────\n'
printf '\033[1;33m 🔥 常用开发命令 (Common Development Commands):\033[0m\n'
printf '   \033[1;36m•\033[0m 编译项目 :  \033[1;32mpython3 build.py       # 结果生成到 artifacts 目录\033[0m\n'
printf '   \033[1;36m•\033[0m 单元测试 :  \033[1;32mpython3 build.py test\033[0m\n'
printf '\n'
printf '\033[1;36m 💡 建议先执行一次 \033[1;32mpython3 build.py\033[0m\033[1;36m，既可验证代码可编译通过，\033[0m\n'
printf '\033[1;36m    同时顺带生成 compile_commands.json，让 clangd 代码跳转开箱即用。\033[0m\n'
printf '──────────────────────────────────────────────────────────────────────\n'
# <<< mindstudio devcontainer dev-hint <<<
EOF
}

# =============================================================================
# fix_file_watcher_limit —— 增大 inotify 文件监听上限
# =============================================================================
#
# 背景与问题：
#   Linux 内核默认的 fs.inotify.max_user_watches 通常为 8192。VS Code 的
#   File Watcher 会为工作区中每个被监听的文件消耗一个 inotify watch，大型项目
#   （含 node_modules、build 产物、源代码）很容易超过该限制，导致 VS Code 报错：
#   "Unable to watch for file changes. Please follow the instructions link to
#   resolve this issue."
#
#   这会令 VS Code 的文件变更探测能力降级，表现为：
#   - 源代码变更无法触发搜索/语法高亮刷新
#   - Git 面板不能实时反映未暂存变更
#   - 部分扩展（如 clangd）无法检测文件变化而重建索引
#
# 解决方案：
#   将 max_user_watches 提升到 524288（VS Code 推荐的典型值）。由于容器使用了
#   --privileged 运行参数，具备修改内核参数的权限，可直接通过 sysctl 或向
#   /proc/sys/fs/inotify/max_user_watches 写入目标值完成调整。
#
# 降级策略：
#   如果 sysctl 和 /proc 写入均失败（极少数受限环境），只输出告警，不阻断容器创建。
fix_file_watcher_limit() {
    local desired=524288
    local current
    current=$(cat /proc/sys/fs/inotify/max_user_watches 2>/dev/null || echo 0)

    if [ "$current" -ge "$desired" ]; then
        log "inotify max_user_watches is already ${current} (>= ${desired})"
        return 0
    fi

    log "inotify max_user_watches is ${current}, increasing to ${desired}..."

    # 优先通过 sysctl 设置
    if command -v sysctl >/dev/null 2>&1; then
        if sudo sysctl -w fs.inotify.max_user_watches="$desired" >/dev/null 2>&1; then
            log "inotify max_user_watches increased to ${desired} (via sysctl)"
            return 0
        fi
    fi

    # 回退：直接写 /proc
    if sudo sh -c "echo ${desired} > /proc/sys/fs/inotify/max_user_watches" 2>/dev/null; then
        log "inotify max_user_watches increased to ${desired} (via /proc)"
        return 0
    fi

    warn "failed to increase inotify max_user_watches; VS Code file watching may not work correctly"
}

# =============================================================================
# ignore_vscode_settings —— 隔离个人化 VS Code Settings 修改
# =============================================================================
#
# 背景与问题：
#   .vscode/settings.json 是工作区级别的 VS Code 配置文件，已纳入 Git 版本管理。
#   开发者在使用过程中可能需要根据个人偏好微调某些设置（如字体大小、主题等），
#   这些个人化修改如果频繁出现在 git status 中会造成噪声，并可能在合入 MR 时
#   引入不必要的配置冲突。
#
# 解决方案：
#   通过 git update-index --skip-worktree 标记 .vscode/settings.json，减少个人化
#   修改在 git status 中产生的噪声。该标记只影响当前工作区，不会传播到远端。
#
# 注意：
#   - skip-worktree 不是 .gitignore 的替代，文件仍然被 Git 追踪；本地修改通常
#     不显示在 git status 中，但远端也修改该文件时，普通 pull 仍可能拒绝覆盖。
#   - 如果需要更新仓库中的 settings.json 模板，建议在本地另 clone 一份代码仓
#     或在 gitcode 页面上直接修改提交，避免本地个人偏好被误提交。
ignore_vscode_settings() {
    git update-index --skip-worktree .vscode/settings.json 2>/dev/null || true

    log "ignore_vscode_settings succeeded"
}

# =============================================================================
# install_git_safe_pull_alias —— 安装 git safe-pull 别名
# =============================================================================
#
# 背景与问题：
#   ignore_vscode_settings 对 .vscode/settings.json 设置了 skip-worktree，
#   这会让 git status 忽略该文件的本地修改。但当远程仓库也更新了同一个文件时，
#   git pull 会因为 "Your local changes would be overwritten by merge" 而失败。
#   普通开发者不了解 git update-index，排查和修复门槛较高。
#
# 解决方案：
#   安装一个全局 Git 别名 git safe-pull，自动处理 skip-worktree 文件：
#     1. 找出所有被 skip-worktree 标记的文件
#     2. 临时取消这些标记
#     3. 暂存 (stash) 本地修改
#     4. 执行正常的 git pull（支持所有 git pull 参数）
#     5. pull 成功时丢弃临时 stash，采用远端版本
#     6. pull 失败时保留 stash，避免本地修改丢失
#     7. 无论成功或失败，均重新设置 skip-worktree 标记
#
# 使用方式：
#   git safe-pull              # 等价于 git pull
#   git safe-pull --rebase     # 等价于 git pull --rebase
#
# 实现说明：
#   具体逻辑放在 .devcontainer/git-safe-pull.sh，避免复杂 Git shell alias 的多层
#   引号破坏参数。每次 post-create 都重新安装脚本并刷新 alias，以覆盖旧容器中
#   已存在的错误版本。
install_git_safe_pull_alias() {
    local source_script="/workspace/.devcontainer/git-safe-pull.sh"
    local target_script="$HOME/.local/bin/git-safe-pull"

    if [ ! -f "$source_script" ]; then
        warn "safe-pull source script not found: $source_script"
        return 0
    fi

    install -m 0755 "$source_script" "$target_script" || {
        warn "failed to install git-safe-pull"
        return 0
    }
    git config --global alias.safe-pull '!git-safe-pull'

    log "install_git_safe_pull_alias succeeded"
}

# =============================================================================
# verify_ccache —— ccache 编译缓存状态检查
# =============================================================================
#
# 背景与问题：
#   devcontainer.json 通过 mounts 将宿主机 ccache 目录持久化挂载到容器内，
#   使增量编译缓存不受容器重建影响。但如果挂载目录权限不正确或为空，
#   开发者可能不会注意到，导致每次都是全量编译，浪费大量时间。
#
# 解决方案：
#   1. 检查 CCACHE_DIR 目录是否存在且可写。
#   2. 目录不存在时尝试 mkdir 创建（兜底）。
#   3. 目录存在但不可写时尝试 sudo chown 修复（Docker bind mount 自动创建的
#      源目录可能属于 root）。
#   4. 报告当前缓存大小，让开发者了解缓存状态。
#   5. 如果 ccache 命令可用，输出缓存命中率统计。
#
# 降级策略：
#   - mkdir / chown 失败不阻塞，最后仍不可写时告警。
#   - 宿主机未预先创建 ccache 目录时，Docker 自动创建 root 所有的空目录，
#     本函数尝试 chown 修复权限。
verify_ccache() {
    local ccache_dir="${CCACHE_DIR:-$HOME/.cache/ccache}"

    # 目录不存在时尝试创建（Docker bind mount 通常已自动创建，兜底用）
    if [ ! -d "$ccache_dir" ]; then
        mkdir -p "$ccache_dir" 2>/dev/null || true
    fi

    # 目录存在但不可写时，尝试 chown 修复（Docker 自动创建的 bind mount
    # 源目录可能属于 root，需要改为当前用户才能写入）
    if [ -d "$ccache_dir" ] && [ ! -w "$ccache_dir" ]; then
        log "ccache dir ($ccache_dir) is not writable, attempting chown..."
        sudo chown -R "$(id -u):$(id -g)" "$ccache_dir" 2>/dev/null || true
    fi

    if [ -d "$ccache_dir" ] && [ -w "$ccache_dir" ]; then
        local cache_size
        cache_size=$(du -sh "$ccache_dir" 2>/dev/null | cut -f1)
        log "ccache dir ready, current size: ${cache_size:-0}"
        if command -v ccache >/dev/null 2>&1; then
            ccache -s 2>/dev/null | head -3 || true
        fi
    else
        warn "ccache dir ($ccache_dir) missing or not writable; incremental build cache may not work"
    fi
}

# =============================================================================
# setup_clangd —— clangd C++ 语义引擎检查与安装
# =============================================================================
#
# 背景与问题：
#   clangd 是 C++ 代码智能跳转、补全和诊断的核心引擎。VS Code 通过
#   clangd 扩展调用 clangd 后台进程，读取 compile_commands.json 来理解
#   代码的编译上下文（头文件路径、宏定义、编译选项等）。
#
#   clangd 未安装时，C++ 文件的符号跳转 (F12)、自动补全和内联诊断将不可用，
#   严重影响 C++ 开发体验。
#
# 解决方案：
#   1. 检查 clangd 是否已安装（镜像可能已预装）。
#   2. 若未安装，通过 sudo dnf install -y clang-tools-extra 自动安装。
#      clang-tools-extra 是包含 clangd 的官方 RPM 包。
#   3. 安装后验证 .clangd 配置文件是否存在，以及 build/compile_commands.json
#      编译数据库是否就绪。
#
# 冷启动说明：
#   首次进入容器时 build/ 目录下尚未执行过 CMake 配置，compile_commands.json
#   不存在属于预期状态。此时脚本只输出提示，不阻断容器创建。开发者执行一次
#   "构建 (Debug 模式)" 或 "构建 (Release 模式)" 后，编译数据库即生成，clangd
#   即可恢复完整的语义能力。
#
# 降级策略：
#   dnf 安装失败时（如离线环境），clangd 功能不可用但不阻断容器创建。
setup_clangd() {
    log "checking clangd setup..."

    if command -v clangd >/dev/null 2>&1; then
        log "clangd found in PATH: $(clangd --version 2>/dev/null | head -1)"
        _check_clangd_config
        return 0
    fi

    log "installing clangd via dnf..."
    sudo dnf install -y clang-tools-extra 2>/dev/null
    log "clangd installed: $(clangd --version 2>/dev/null | head -1)"

    _check_clangd_config
}

# _check_clangd_config:
#   内部辅助函数，验证 clangd 运行所需的两项前置条件：
#   1. /workspace/.clangd 配置文件是否存在
#      该文件固定指向 CompilationDatabase: build/，是 clangd 读取编译命令的入口。
#   2. /workspace/build/compile_commands.json 编译数据库是否存在
#      该文件由 CMake 生成，包含每个 .cpp 文件的完整编译命令。
#   两者在首次创建容器时可能都不存在，属于预期冷启动状态。
_check_clangd_config() {
    if [ -f /workspace/.clangd ]; then
        log ".clangd config file found"
    else
        warn ".clangd config file missing; clangd may not work correctly"
    fi

    if [ ! -f /workspace/build/compile_commands.json ]; then
        warn "compile_commands.json not found in build/; run cmake with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON to generate it"
    else
        log "compile_commands.json found"
    fi
}

# 输出醒目的完成标识，便于开发者在 Dev Containers 启动日志中快速确认状态。
print_ready_banner() {
    local green=$'\033[1;32m'
    local reset=$'\033[0m'

    cat <<EOF

+------------------------------------------------------------------------------+
|                                                                              |
|${green}       ____  _____    _    ______   __                                        ${reset}|
|${green}      |  _ \| ____|  / \  |  _ \ \ / /                                        ${reset}|
|${green}      | |_) |  _|   / _ \ | | | \ V /                                         ${reset}|
|${green}      |  _ <| |___ / ___ \| |_| || |                                          ${reset}|
|${green}      |_| \_\_____/_/   \_\____/ |_|                                          ${reset}|
|                                                                              |
|  +------------------------------------------------------------------------+  |
|  |  DEV CONTAINER                                          STATUS: ${green}READY${reset}  |  |
|  +------------------------------------------------------------------------+  |
|                                                                              |
|     ENVIRONMENT   ${green}READY${reset} FOR DEVELOPMENT                                      |
|     WORKSPACE     /workspace                                                 |
|                                                                              |
|     Initialization complete. Start coding, building, and shipping.           |
|                                                                              |
+------------------------------------------------------------------------------+

EOF
}

# =============================================================================
# 主执行流程
# =============================================================================
#
# 按依赖顺序执行：先修复目录权限，再写用户配置，最后安装 Git 辅助能力。
# run_step 为每一步输出序号、状态和耗时，非关键项失败不阻止容器启动。

total_steps=14
log "Dev Container initialization started ($total_steps steps)"

run_step 1  "$total_steps" "Configure Huawei Cloud yum mirror"    configure_yum_mirror
run_step 2  "$total_steps" "Fix cache directory ownership"       fix_cache_ownership
run_step 3  "$total_steps" "Configure file watcher limit"        fix_file_watcher_limit
run_step 4  "$total_steps" "Configure user command paths"        configure_user_bin
run_step 5  "$total_steps" "Configure shared command paths"      ensure_shared_bin_path
run_step 6  "$total_steps" "Configure Python 3.11"                configure_python311
run_step 7  "$total_steps" "Synchronize Git identity"            sync_git_identity
run_step 8  "$total_steps" "Install developer shell hints"       append_dev_hint_once
run_step 9  "$total_steps" "Install pre-commit Git Hook"          install_pre_commit_hook
run_step 10 "$total_steps" "Start pre-commit environment warmup"  warmup_pre_commit_async
run_step 11 "$total_steps" "Configure workspace Git settings"    ignore_vscode_settings
run_step 12 "$total_steps" "Install Git safe-pull command"        install_git_safe_pull_alias
run_step 13 "$total_steps" "Verify ccache"                        verify_ccache
run_step 14 "$total_steps" "Configure clangd"                     setup_clangd

log "post-create setup finished"
print_ready_banner
