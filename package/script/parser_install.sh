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
# ================================================================================
# run包安装解析公共脚本
# 解析filelist.csv文件，完成目录创建，文件复制，权限设置，文件删除等操作。

# 缓存mod文件名
STASH_MOD_PATH="stash_mod.txt"
# stash_mod文件权限
STASH_FILE_MOD="600"
RESET_MOD="750"

# 写日志
log() {
    local _cur_date="$(date +"%Y-%m-%d %H:%M:%S")"
    local _log_type="${1}"
    local _content="${2}"
    local _format="[Common] [${_cur_date}] [${_log_type}]: ${_content}"

    if [ "${_log_type}" = "INFO" ]; then
        echo "${_format}"
    elif [ "${_log_type}" = "WARNING" ]; then
        echo "${_format}"
    elif [ "${_log_type}" = "ERROR" ]; then
        echo "${_format}"
    elif [ "${_log_type}" = "DEBUG" ]; then
        echo "${_format}" 1> /dev/null
    fi
}

# 检查路径是否为绝对路径
__check_abs_path() {
    local path="$1"

    if [ "${path#/}" != "${path}" ]; then
        # 绝对路径
        return 0
    fi
    return 1
}

__set_abs_path() {
    local install_path="$1"
    local path="$2"
    local varname="$3"

    __check_abs_path "${path}"
    if [ $? -ne 0 ]; then
        eval "${varname}=\"${install_path}/${path}\""
    else
        eval "${varname}=\"${path}\""
    fi
}

# 返回列表长度
__length_list() {
    local list="$1"
    local var="$2"
    local list_item
    local cnt=0

    for list_item in ${list}; do
        cnt=$((cnt+1))
    done

    eval "${var}=\"${cnt}\""
}

# 获取列表索引值
__index_list() {
    local list="$1"
    shift
    local idx="$1"
    local var="$2"
    local list_item
    local cnt=0

    if [ $# -eq 0 ]; then
        return 0
    fi

    for list_item in ${list}; do
        if [ ${1} -eq ${cnt} ]; then
            eval "${2}=\"${list_item}\""
            shift 2
            if [ $# -eq 0 ]; then
                return 0
            fi
        fi
        cnt=$((cnt+1))
    done
}

# 解析filelist.csv脚本
parse_filelist() {
    local install_path="$1"
    local operate_type="$2"
    local filelist_path="$3"
    local filter_type="$4"
    local operate_type_list="$operate_type"

    if [ ! -f "$filelist_path" ]; then
        log "ERROR" "$filelist_path does not exist!"
        exit 1
    fi

    filelist=$(awk -v operate_type_list="${operate_type_list}" '
        BEGIN{
            FS= ","
        }
        {
            if (operate_type_list !~ $2) next

            print $3,$4,$5,$6,$7,$8,$9
        }' "$filelist_path")
}

# 迭代遍历filelist中的条目，执行一些操作
foreach_filelist() {
    local filter_type="$1"
    local exec_func="$2"
    local install_path="$3"
    local operate_type="$4"
    local filelist_path="$5"
    local sort_filelist="$6"  # 排序filelist，可选参数为[no,reverse]
    local exec_mode="$7"  # 执行模式，可选参数为[normal,concurrency]
    local array
    local len_array

    parse_filelist "${install_path}" "${operate_type}" "${filelist_path}" "${filter_type}"

    if [ "${sort_filelist}" = "reverse" ]; then
        # 对第二列文件路径做倒序排列，保证先删除子文件夹，再删除父文件夹
        filelist=$(echo "$filelist" | sort -k2 -r)
    fi

    if [ "${exec_mode}" = "concurrency" ] || [ "${exec_mode}" = "normal" ]; then
        # 并发模式或正常模式
        echo > /dev/null
    else
        log "ERROR" "exec_mode param wrong! exec_mode=${exec_mode}."
        exit 1
    fi

    while read line; do
        array=${line}
        __length_list "${array}" "len_array"
        if [ ${len_array} -eq 0 ]; then
            continue
        fi
        "${exec_func}" "${install_path}" "${line}"
    done << EOF
${filelist}
EOF

    if [ "${exec_mode}" = "concurrency" ]; then
        wait
    fi
}

# 解析stash_mod.txt
parse_stashmod() {
    local stashmod_path="$1"

    if [ ! -f "${stashmod_path}" ]; then
        log "ERROR" "stash mod ${stashmod_path} does not exist!"
        exit 1
    fi

    stashmod_list=$(awk '
        BEGIN{
            FS=":"
        }
        {
            print $1,$2
        }' "${stashmod_path}")
}

# 迭代遍历stash_mod中的条目，执行操作
foreach_stashmod() {
    local exec_func="$1"
    local install_path="$2"
    local sort_type="$3"  # 排序方式，可选参数为[no,reverse]
    local array
    local len_array
    local line

    parse_stashmod "${install_path}/${STASH_MOD_PATH}"

    if [ "${sort_type}" = "reverse" ]; then
        # 对第二列文件路径做倒序排列，保证先处理子文件夹，再处理父文件夹
        stashmod_list=$(echo "${stashmod_list}" | sort -k1 -r)
    elif [ "${sort_type}" = "no" ]; then
        # 不排序
        echo > /dev/null
    else
        log "ERROR" "sort_type param wrong! sort_type=${sort_type}"
        exit 1
    fi

    while read line; do
        array=${line}
        __length_list "${array}" "len_array"
        if [ ${len_array} -eq 0 ]; then
            continue
        fi
        "${exec_func}" "${install_path}" "${line}"
    done << EOF
${stashmod_list}
EOF
}

# 修改各文件及目录的权限
change_mod() {
    local path="$1"
    local mod="$2"
    local install_for_all="$3"

    if [ -L "${path}" ] || [ "${mod}" == "NA" ]; then
        return 0
    fi

    # 如果设置了install_for_all，则安装时other权限跟group权限对齐
    if [ "${install_for_all}" = "y" ]; then
        local len_mod=${#mod}
        local new_mod="${mod%?}"
        local new_mod="${new_mod}${new_mod#${new_mod%?}}"
        chmod "${new_mod}" "${path}"
    else
        chmod "${mod}" "${path}"
    fi
    if [ $? -ne 0 ]; then
        log "ERROR" "chmod ${path} failed!"
        exit 1
    fi
}

# 修改各文件及目录的属性
change_own() {
    local path="$1"
    local owner="$2"

    if [ "${owner}" == "NA" ]; then
        return 0
    fi

    if [ "${owner}" = "\$username:\$usergroup" ]; then
        chown -h "${USERNAME}:${USERGROUP}" "${path}"
    else
        chown -h "${owner}" "${path}"
    fi
    if [ $? -ne 0 ]; then
        log "ERROR" "chown ${path} failed!"
        exit 1
    fi
}

# 准备并且检查软链接路径
prepare_and_check_softlink_path() {
    local softlink_abs="$1"
    local softlink_dir="$(dirname "${softlink_abs}")"

    if [ ! -d "${softlink_dir}" ]; then
        mkdir -p "${softlink_dir}"
    fi

    # 如果目标路径是个软链接，则移除
    if [ -L "${softlink_abs}" ]; then
        rm -f "${softlink_abs}"
        if [ $? -ne 0 ]; then
            log "ERROR" "remove softlink ${softlink_abs} failed! (create relative softlink)"
            exit 1
        fi
    fi

    # 不允许目标路径已经是一个目录，防止软链接到错误的位置。
    if [ -d "${softlink_abs}" ]; then
        log "ERROR" "softlink existed dir ${softlink_abs}!"
        exit 1
    fi
}

# 使用绝对路径创建软连接
create_abs_softlink() {
    local softlink_abs="$2"
    local link_target

    prepare_and_check_softlink_path "${softlink_abs}"

    link_target=$(
        cd "$(dirname "$1")" || return
        pwd
    )/$(basename "$1")

    ln -sf "${link_target}" "$2"
    if [ $? -ne 0 ]; then
        log "ERROR" "$2 softlink to ${link_target} failed!"
        exit 1
    fi
}

# 使用相对路径创建软连接
create_rel_softlink() {
    local install_path="$1"
    local target="$2"
    local softlink="$3"
    local softlink_abs
    local softlink_dir
    local softlink_name
    local softlink_rel

    softlink_abs="${install_path}/${softlink}"
    softlink_dir="$(dirname "${softlink_abs}")"
    softlink_name="$(basename "${softlink_abs}")"

    prepare_and_check_softlink_path "${softlink_abs}"

    softlink_rel=$(echo | awk -v target="${target}" -v softlink="${softlink}" '
        BEGIN {
            len_t = split(target, target_list, "/")
            len_s = split(softlink, softlink_list, "/")

            if (len_t < len_s) {
                len_min = len_t
            } else {
                len_min = len_s
            }

            i = 1
            while (i <= len_min && target_list[i] == softlink_list[i]) {
                ++i
            }
            --i

            uplevel = len_s - i - 1
            rel_prefix = ""
            if (uplevel > 0) {
                for (j = 0; j < uplevel; ++j) {
                    rel_prefix = rel_prefix "../"
                }
            }

            rel_suffix = target_list[i+1]
            for (j = i+2; j <= len_t; ++j) {
                rel_suffix = rel_suffix "/" target_list[j]
            }
            print rel_prefix rel_suffix
        }
    ')

    cd "${softlink_dir}"

    ln -sf "${softlink_rel}" "${softlink_name}"
    if [ $? -ne 0 ]; then
        cd - > /dev/null
        log "ERROR" "${softlink} softlink to ${target} failed!"
        exit 1
    fi

    cd - > /dev/null
}

# 创建软连接
create_softlink() {
    local install_path="$1"
    local target="$2"
    local softlink="$3"
    local target_abs
    local softlink_abs

    if [ "${softlink}" = "NA" ]; then
        return 0
    fi

    __check_abs_path "${target}"
    if [ $? -eq 0 ]; then
        __set_abs_path "${install_path}" "${softlink}" "softlink_abs"
        create_abs_softlink "${target}" "${softlink_abs}"
        return 0
    fi

    __check_abs_path "${softlink}"
    if [ $? -eq 0 ]; then
        __set_abs_path "${install_path}" "${target}" "target_abs"
        create_abs_softlink "${target_abs}" "${softlink}"
        return 0
    fi

    create_rel_softlink "${install_path}" "${target}" "${softlink}"
}

# 创建stash_mod.txt文件
create_stash_mod() {
    local install_path="$1"

    rm -f "${install_path}/${STASH_MOD_PATH}"
    touch "${install_path}/${STASH_MOD_PATH}"
    chmod ${STASH_FILE_MOD} "${install_path}/${STASH_MOD_PATH}"
}

# 删除stash_mod.txt文件
remove_stash_mod() {
    local install_path="$1"

    rm -f "${install_path}/${STASH_MOD_PATH}"
}

# 获取文件权限
get_file_mod() {
    local path="$1"

    stat -c %a "${path}"
}

# 创建文件夹
create_folder() {
    local install_path="$1"
    local target="$2"
    local softlink="$3"
    local target_abs

    __set_abs_path "${install_path}" "${target}" "target_abs"

    if [ ! -d "${target_abs}" ]; then
        mkdir -p "${target_abs}"
        if [ $? -ne 0 ]; then
            log "ERROR" "${target_abs} mkdir failed!"
            exit 1
        fi
        change_mod "${target_abs}" "${RESET_MOD}" ""
    fi

    if [ "${softlink}" != "NA" ]; then
        create_softlink "${install_path}" "${target}" "${softlink}"
    fi
}

# 创建目录
create_dirs() {
    local install_path="$1"
    local line="$2"
    local target
    local target_abs
    local softlink

    __index_list "${line}" 1 "target" 4 "softlink"

    __set_abs_path "${install_path}" "${target}" "target_abs"

    if [ -L "${target_abs}" ] ; then
        rm -f "${target_abs}"
        log "WARNING" "${target_abs} is an existing soft-link, deleted."
    fi
    create_folder "${install_path}" "${target}" "${softlink}"
}

# 修改目录的权限和属组
reset_mod_dirs() {
    local install_path="$1"
    local line="$2"
    local target
    local target_abs

    __index_list "${line}" 1 "target"

    __set_abs_path "${install_path}" "${target}" "target_abs"

    # 目录不存在时跳过
    if [ ! -d "${target_abs}" ]; then
        return 0
    fi
    # 只处理目录，没有处理目录的软链接
    change_mod "${target_abs}" "${RESET_MOD}" ""
}

# 修改目录的权限和属组，暂存原文件权限
reset_mod_dirs_with_stash_mod() {
    local install_path="$1"
    local line="$2"
    local target
    local target_abs

    __index_list "${line}" 1 "target"

    __set_abs_path "${install_path}" "${target}" "target_abs"

    if [ ! -d "${target_abs}" ]; then
        return 0
    fi

    local mod="$(get_file_mod "${target_abs}")"
    echo "${target}:${mod}" >> "${install_path}/${STASH_MOD_PATH}"

    # 只处理目录，没有处理目录的软链接
    change_mod "${target_abs}" "${RESET_MOD}" ""
}

# 修改目录的权限和属组
restore_stash_mod() {
    local install_path="$1"
    local line="$2"
    local target
    local target_abs
    local mod

    __index_list "${line}" 0 "target" 1 "mod"

    __set_abs_path "${install_path}" "${target}" "target_abs"

    if [ ! -d "${target_abs}" ]; then
        return 0
    fi
    # 只处理目录，没有处理目录的软链接
    change_mod "${target_abs}" "${mod}" "${INSTALL_FOR_ALL}"
}

# 执行创建目录动作
do_create_dirs() {
    local install_path="$1"
    local filelist_path="$2"
    local package="$3"

    # 先重置目录权限配置，防止软链接时缺少权限
    foreach_filelist "NA" "reset_mod_dirs" "$install_path" "mkdir" "$filelist_path" "no" "normal"
    foreach_filelist "NA" "create_dirs" "$install_path" "mkdir" "$filelist_path"  "no" "normal"
}

# 设置目录权限
do_reset_mod_dirs() {
    local install_path="$1"
    local filelist_path="$2"
    local package="$3"

    if [ ! -z "${package}" ]; then
        create_stash_mod "${install_path}"
        foreach_filelist "NA" "reset_mod_dirs_with_stash_mod" "${install_path}" "mkdir" "${filelist_path}" "no" "normal"
    else
        foreach_filelist "NA" "reset_mod_dirs" "${install_path}" "mkdir" "${filelist_path}" "no" "normal"
    fi
}

# 拷贝文件
copy_file() {
    local install_path="$1"
    local source="$2"
    local target="$3"
    local softlink="$4"
    local target_abs

    __set_abs_path "${install_path}" "${target}" "target_abs"

    if [ ! -e "${source}" ]; then
        log "ERROR" "${source} does not exist!"
        exit 1
    fi

    local target_dir="$(dirname "${target_abs}")"
    cp -rf "${source}" "${target_dir}"
    if [ $? -ne 0 ]; then
        log "ERROR" "copy ${source} failed!"
        exit 1
    fi

    if [ "${softlink}" != "NA" ]; then
        create_softlink "${install_path}" "${target}" "${softlink}"
    fi
}

# 复制文件
copy_files() {
    local install_path="$1"
    local line="$2"
    local source
    local target
    local target_abs
    local softlink
    local configurable
    local hash_value

    __index_list "${line}" 0 "source" 1 "target" 4 "softlink" 5 "configurable" 6 "hash_value"

    __set_abs_path "${install_path}" "${target}" "target_abs"

    local target_dir="$(dirname "${target_abs}")"
    if [ ! -d "${target_dir}" ]; then
        mkdir -p "${target_dir}"
    fi

    # 如果目标文件已经存在，而且是配置文件，则不执行覆盖操作
    if [ -e "${target_abs}" ] && [ "${configurable}" = "TRUE" ]; then
        return 0
    fi

    copy_file "${install_path}" "${source}" "${target}" "${softlink}"
}

# 执行拷贝动作
do_copy_files() {
    local install_path="$1"
    local filelist_path="$2"
    local package="$3"

    foreach_filelist "NA" "copy_files" "$install_path" "copy" "$filelist_path" "no" "normal"
}

# 修改权限和属组
change_mod_and_own(){
    local target="$1"
    local mod="$2"
    local own="$3"
    local install_for_all="$4"

    change_mod "${target}" "${mod}" "${install_for_all}"
    change_own "${target}" "${own}"
}

# 修改文件的权限和属组
change_mod_and_own_files() {
    local install_path="$1"
    local line="$2"
    local target
    local mod
    local own

    __index_list "${line}" 1 "target" 2 "mod" 3 "own"
    __check_abs_path "${target}"
    if [ $? -ne 0 ]; then
        target="${install_path}/${target}"
    fi
    if [ -d "${target}" ]; then
        return 0
    fi
    # 只处理文件，没有处理文件的软链接
    change_mod_and_own "${target}" "${mod}" "${own}" "${INSTALL_FOR_ALL}" &
}

# 修改目录的权限和属组
change_mod_and_own_dirs() {
    local install_path="$1"
    local line="$2"
    local target
    local mod
    local own

    __index_list "${line}" 1 "target" 2 "mod" 3 "own"

    __check_abs_path "${target}"
    if [ $? -ne 0 ]; then
        target="${install_path}/${target}"
    fi
    if [ ! -d "${target}" ]; then
        return 0
    fi
    # 只处理目录，没有处理目录的软链接
    change_mod_and_own "${target}" "${mod}" "${own}" "${INSTALL_FOR_ALL}"
}

# 修改文件和目录的权限
do_chmod_file_dir() {
    local install_path="$1"
    local filelist_path="$2"
    local package="$3"
    
    foreach_filelist "NA" "change_mod_and_own_files" "$install_path" "copy del" "$filelist_path" "no" "concurrency"
    foreach_filelist "NA" "change_mod_and_own_dirs" "$install_path" "mkdir" "$filelist_path" "reverse" "normal"
}

# 删除软连接
remove_softlink() {
    local softlink="$1"

    if [ -z "${softlink}" ]; then
        return 0
    fi

    if [ "${softlink}" != "NA" ] && [ -L "${softlink}" ]; then
        rm -f "${softlink}"
        if [ $? -ne 0 ]; then
            log "ERROR" "remove ${softlink} failed!"
            exit 1
        fi
    fi
}

remove_file() {
    local target="$1"
    local softlink="$2"

    if [ -e "${target}" ] || [ -L "${target}" ]; then
        rm -f "${target}"
        if [ $? -ne 0 ]; then
            log "ERROR" "remove ${target} failed!"
            exit 1
        fi
    fi
    remove_softlink "${softlink}"
}

remove_dir() {
    local dir="$1"

    if [ -e "${dir}" ] || [ -L "${dir}" ]; then
        rm -rf "${dir}"
        if [ $? -ne 0 ]; then
            log "ERROR" "remove ${dir} failed!"
            exit 1
        fi
    fi
}

# 删除安装文件
remove_install_files() {
    local install_path="$1"
    local line="$2"
    local target
    local softlink
    local configurable
    local hash_value

    __index_list "${line}" 1 "target" 4 "softlink" 5 "configurable" 6 "hash_value"

    if [ "${target}" != "NA" ]; then
        __check_abs_path "${target}"
        if [ $? -ne 0 ]; then
            target="${install_path}/${target}"
        fi
        if [ -d "${target}" ] && [ ! -L "${target}" ]; then
            return 0
        fi
        if [ "${softlink}" != "NA" ]; then
            __check_abs_path "${softlink}"
            if [ $? -ne 0 ]; then
                softlink="${install_path}/${softlink}"
            fi
        fi
        # 配置文件不删除
        if [ "${configurable}" = "TRUE" ]; then
            echo "${hash_value} ${target}" | sha256sum --check &> /dev/null
            if [ $? -ne 0 ]; then
                log "WARNING" "${target} has been modified!"
                return 0
            fi
        fi
        remove_file "${target}" "${softlink}" &
    fi
}

# 删除安装文件夹生成的软链接
remove_install_dirs_softlink() {
    local install_path="$1"
    local line="$2"
    local target
    local softlink

    __index_list "${line}" 1 "target" 4 "softlink"

    if [ "$target" != "NA" ]; then
        if [ "${softlink}" != "NA" ]; then
            __check_abs_path "${softlink}"
            if [ $? -ne 0 ]; then
                softlink="${install_path}/${softlink}"
            fi
        fi
        if [ -d "${softlink}/" ]; then
            local sub_num=$(ls -A "${softlink}/" | wc -l)
            [ ${sub_num} -gt 0 ] && return
        fi
        remove_softlink "${softlink}"
    fi
}

remove_install_dirs() {
    local install_path="$1"
    local line="$2"
    local target

    __index_list "${line}" 1 "target"

    if [ "${target}" != "NA" ]; then
        __check_abs_path "${target}"
        if [ $? -ne 0 ]; then
            target="${install_path}/${target}"
        fi
        if [ ! -d "${target}" ]; then
            return 0
        fi
        local file_nums=$(ls -A "${target}" | wc -l)
        if [ ${file_nums} -ne 0 ]; then
            return 0
        fi
        remove_dir "${target}"
    fi
}

# 删除安装文件与目录等
do_remove() {
    local install_path="$1"
    local filelist_path="$2"
    local package="$3"
    local func_after_remove_01
    local func_after_remove_02

    local tmp_path="/tmp"
    local tmp_filelist_path=$(mktemp "$tmp_path/filelist_XXXXXX" || exit 1)
    cp -f "$filelist_path" "$tmp_filelist_path"

    if [ -f "${install_path}/${STASH_MOD_PATH}" ]; then
        # 删除文件后，恢复目录权限配置
        func_after_remove_01="foreach_stashmod \"restore_stash_mod\" \"${install_path}\" \"reverse\""
        func_after_remove_02="remove_stash_mod \"${install_path}\""
    fi

    foreach_filelist "NA" "remove_install_files" "$install_path" "copy del" "$tmp_filelist_path" "no" "concurrency"
    foreach_filelist "NA" "remove_install_dirs" "$install_path" "mkdir" "$tmp_filelist_path" "reverse" "normal"
    foreach_filelist "NA" "remove_install_dirs_softlink" "$install_path" "mkdir" "$tmp_filelist_path" "no" "normal"

    eval "${func_after_remove_01}"
    eval "${func_after_remove_02}"
    rm -f "$tmp_filelist_path"
}

print_usage() {
    local _ret=$1

    echo "Usage: $0 [Options] --package=<package> [ --username=<user> --usergroup=<group> ] install_path filelist_path"
    echo "Options:"
    echo "    --help       | -h      : Print out this help message"
    echo "    --copy       | -c      : Copy the install content"
    echo "    --mkdir      | -m      : Create the install folder, and set dir right"
    echo "    --makedir    | -d      : Create the install folder, not set dir right"
    echo "    --chmoddir   | -o      : Set dir right"
    echo "    --restoremod | -e      : Restore dir right"
    echo "    --remove     | -r      : Remove the install content"
    echo "    --package=<package>    : specify package"
    echo "    --username=<user>      : specify user"
    echo "    --usergroup=<group>    : specify group"
    echo "    --install_for_all=<bool>  : Install for all user"
    exit ${_ret}
}

# 正式命令
formal_commands() {
    local install_path="$1"
    local filelist_path="$2"

    case "${OPERATE_TYPE}" in
    "makedir")
        do_create_dirs "$install_path" "$filelist_path" "${PACKAGE}"
        ;;
    "copy")
        do_copy_files "$install_path" "$filelist_path" "${PACKAGE}"
        ;;
    "chmoddir")
        do_chmod_file_dir "$install_path" "$filelist_path" "${PACKAGE}"
        ;;
    "restoremod")
        do_reset_mod_dirs "$install_path" "$filelist_path" "${PACKAGE}"
        ;;
    "remove")
        do_remove "$install_path" "$filelist_path" "${PACKAGE}"
        ;;
    esac
}

# 正式命令并加锁
formal_commands_with_lock() {
    local install_path="$1"
    local lockfile="${install_path}/ascend.lock"

    if [ ! -d "${install_path}" ]; then
        # 兼容覆盖安装场景，包安装目录被删除情况。
        mkdir -p "${install_path}"
    fi

    (
        flock -n 9 || exit 1
        formal_commands "$@"
    ) 9> "${lockfile}"

    if [ $? -ne 0 ]; then
        log "ERROR" "get ${install_path} lockfile failed!"
        exit 1
    fi

    rm -f "${lockfile}"
}


# 全局变量
# change_mod_and_own_files与change_mod_and_own_dirs函数中会使用
INSTALL_FOR_ALL=""

OPERATE_TYPE=""
PACKAGE=""
USERNAME=""
USERGROUP=""

while true; do
    case "$1" in
    --copy | -c)
        OPERATE_TYPE="copy"
        shift
        ;;
    --makedir | -d)
        OPERATE_TYPE="makedir"
        shift
        ;;
    --chmoddir | -o)
        OPERATE_TYPE="chmoddir"
        shift
        ;;
    --restoremod | -e)
        OPERATE_TYPE="restoremod"
        shift
        ;;
    --remove | -r)
        OPERATE_TYPE="remove"
        shift
        ;;
    --username=*)
        USERNAME=$(echo "$1" | cut -d"=" -f2)
        shift
        ;;
    --usergroup=*)
        USERGROUP=$(echo "$1" | cut -d"=" -f2)
        shift
        ;;
    --install_for_all=*)
        INSTALL_FOR_ALL=$(echo "$1" | cut -d"=" -f2)
        shift
        ;;
    --package=*)
        PACKAGE=$(echo "$1" | cut -d"=" -f2)
        shift
        ;;
    -h | --help)
        print_usage 0
        ;;
    -*)
        echo Unrecognized input options : "$1"
        print_usage 1
        ;;
    *)
        break
        ;;
    esac
done

if [ $# -lt 2 ]; then
    log "ERROR" "It's too few input params: $*"
    exit 1
fi

formal_commands_with_lock "$@"

exit 0

