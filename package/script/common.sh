#!/bin/bash
INSTALL_INFO_KEY_ARRAY=("UserName" "UserGroup" "Install_Path_Param")
MODULE_NAME="mindstudio-opprof"
LEVEL_ERROR="ERROR"
LEVEL_WARN="WARNING"
LEVEL_INFO="INFO"
USERNAME=$(id -un)
USERGROUP=$(id -gn)
SHELL_DIR=$(cd "$(dirname "$0")" || exit; pwd)

ARCH=$(cat $SHELL_DIR/../scene.info | grep arch | cut -d"=" -f2)
OS=$(cat $SHELL_DIR/../scene.info | grep os | cut -d"=" -f2)

MINDSTUDIO_ARRAY=("mindstudio-opprof")
LINK_ARRAY=("bin" "lib64")

export log_file=""

function log() {
    local content=`echo "$@" | cut -d" " -f2-`
    local cur_date=`date +"%Y-%m-%d %H:%M:%S"`

    echo "[${MODULE_NAME}] [${cur_date}] [$1]: $content" >> "${log_file}"
}

function log_and_print() {
    local content=`echo "$@" | cut -d" " -f2-`
    local cur_date=`date +"%Y-%m-%d %H:%M:%S"`

    echo "[${MODULE_NAME}] [${cur_date}] [$1]: $content"
    echo "[${MODULE_NAME}] [${cur_date}] [$1]: $content" >> "${log_file}"
}

function print_log() {
    local content=`echo "$@" | cut -d" " -f2-`
    local cur_date=`date +"%Y-%m-%d %H:%M:%S"`

    echo "[${MODULE_NAME}] [${cur_date}] [$1]: $content"
}

function init_log() {
    local _log_path="/var/log/ascend_seclog"
    local _log_file="ascend_install.log"

    if [ $(id -u) -ne 0 ]; then
        local _home_path=`eval echo "~"`
        _log_path="${_home_path}${_log_path}"
    fi

    log_file="${_log_path}/${_log_file}"

    create_folder "${_log_path}" "${USERNAME}:${USERGROUP}" 750
    if [ $? -ne 0 ]; then
        print_log $LEVEL_WARN "Create ${_log_path} failed."
    fi

    if [ -L "${log_file}" ] || [ ! -f "${log_file}" ]; then
        rm -rf "${log_file}" >/dev/null 2>&1
    fi
    create_file "${log_file}" "${USERNAME}:${USERGROUP}" 640
    if [ $? -ne 0 ]; then
        print_log $LEVEL_WARN "Create ${log_file} failed."
    fi
}

function start_log() {
    free_log_space
    local cur_date=`date +"%Y-%m-%d %H:%M:%S"`

    echo "[${MODULE_NAME}] [${cur_date}] [INFO]: Start Time: $cur_date"
    echo "[${MODULE_NAME}] [${cur_date}] [INFO]: Start Time: $cur_date" >> "${log_file}"
}

function exit_log() {
    local cur_date=`date +"%Y-%m-%d %H:%M:%S"`

    echo "[${MODULE_NAME}] [${cur_date}] [INFO]: End Time: $cur_date"
    echo "[${MODULE_NAME}] [${cur_date}] [INFO]: End Time: $cur_date" >> "${log_file}"
    exit $1
}

function free_log_space() {
    local file_size=$(stat -c %s "${log_file}")
    # mindstudio install log file will be limited in 20M
    if [ "${file_size}" -gt $((1024 * 1024 * 20)) ]; then
        local ibs=512
        local delete_size=$((${file_size} / 3 / ${ibs}))
        dd if="$log_file" of="${log_file}_tmp" bs="${ibs}" skip="${delete_size}" > /dev/null 2>&1
        mv "${log_file}_tmp" "${log_file}"
        chmod 640 ${log_file}
    fi
}

function update_install_param() {
    local _key=$1
    local _val=$2
    local _file=$3
    local _param

    if [ ! -f "${_file}" ]; then
        exit 1
    fi

    for key_param in "${INSTALL_INFO_KEY_ARRAY[@]}"; do
        if [ ${key_param} != ${_key} ]; then
            continue
        fi
        _param=`grep -r "${_key}=" "${_file}"`
        if [ "x${_param}" = "x" ]; then
            echo "${_key}=${_val}" >> "${_file}"
        else
            sed -i "/^${_key}=/c ${_key}=${_val}" "${_file}"
        fi
        break
    done
}

function get_install_param() {
    local _key=$1
    local _file=$2
    local _param

    if [ ! -f "${_file}" ];then
        exit 1
    fi

    for key_param in "${INSTALL_INFO_KEY_ARRAY[@]}"; do
        if [ ${key_param} != ${_key} ]; then
            continue
        fi
        _param=`grep -r "${_key}=" "${_file}" | cut -d"=" -f2-`
        break
    done
    echo "${_param}"
}

function change_mode() {
    local _mode=$1
    local _path=$2
    local _type=$3

    if [ ! x"${install_for_all}" = "x" ] && [ ${install_for_all} = y ]; then
        _mode="$(expr substr ${_mode} 1 2)$(expr substr ${_mode} 2 1)"
    fi
    if [ ${_type} = "dir" ]; then
        find "${_path}" -type d -exec chmod ${_mode} {} \; 2> /dev/null
    elif [ ${_type} = "file" ]; then
        find "${_path}" -type f -exec chmod ${_mode} {} \; 2> /dev/null
    fi
}

function change_file_mode() {
    local _mode=$1
    local _path=$2
    change_mode ${_mode} "${_path}" file
}

function change_dir_mode() {
    local _mode=$1
    local _path=$2
    change_mode ${_mode} "${_path}" dir
}

function create_file() {
    local _file=$1

    if [ ! -f "${_file}" ]; then
        touch "${_file}"
        [ $? -ne 0 ] && return 1
    fi

    chown -hf "$2" "${_file}"
    [ $? -ne 0 ] && return 1
    change_file_mode "$3" "${_file}"
    [ $? -ne 0 ] && return 1
    return 0
}

function create_folder() {
    # 1. 定义局部变量（加双引号兼容空格路径）
    local _path="$1"          # 目标文件夹路径（必填）
    local _owner="$2"         # 要设置的属主/属组（如 root:root，可选）
    local _mode="$3"          # 要设置的权限（如 755，可选）
    local is_created=0        # 标记：是否本次创建了文件夹（0=未创建，1=已创建）

    # 2. 校验核心参数：路径不能为空
    if [ -z "${_path}" ]; then
        echo "错误：文件夹路径不能为空" >&2
        return 1
    fi

    # 3. 检查文件夹是否不存在，不存在则创建
    if [ ! -d "${_path}" ]; then
        # 递归创建文件夹（取消静默，保留错误输出便于排查）
        mkdir -p "${_path}"
        if [ $? -ne 0 ]; then
            echo "错误：创建文件夹 ${_path} 失败" >&2
            return 1
        fi
        is_created=1  # 标记：本次成功创建了文件夹
    fi

    # 4. 仅当本次创建了文件夹时，才执行属组/权限变更
    if [ ${is_created} -eq 1 ]; then
        # 4.1 设置所属用户/组（仅传了_owner参数时执行）
        if [ -n "${_owner}" ]; then
            chown -hf "${_owner}" "${_path}"
            if [ $? -ne 0 ]; then
                echo "错误：为新建文件夹 ${_path} 设置属主 ${_owner} 失败" >&2
                return 1
            fi
        fi

        # 4.2 设置文件夹权限（仅传了_mode参数时执行，替换原自定义函数为原生chmod）
        if [ -n "${_mode}" ]; then
            chmod -f "${_mode}" "${_path}"
            if [ $? -ne 0 ]; then
                echo "错误：为新建文件夹 ${_path} 设置权限 ${_mode} 失败" >&2
                return 1
            fi
        fi
    fi

    # 5. 所有操作成功（无论是否新建文件夹，只要无错误就返回0）
    return 0
}

function is_dir_empty() {
    local _path=$1
    local _file_num

    if [ -z ${_path} ]; then
        return 1
    fi

    if [ ! -d ${_path} ]; then
        return 1
    fi
    _file_num=`ls "${_path}" | wc -l`
    if [ ${_file_num} -eq 0 ]; then
        return 0
    fi
    return 1
}

function check_install_path_valid() {
    local install_path="$1"
    # 黑名单设置，不允许//，...这样的路径
    if echo "${install_path}" | grep -Eq '/{2,}|\.{3,}'; then
        return 1
    fi
    # 白名单设置，只允许常见字符
    if echo "${install_path}" | grep -Eq '^~?[a-zA-Z0-9./_-]*$'; then
        return 0
    else
        return 1
    fi
}

function check_dir_permission() {
    local _path=$1

    if [ -z ${_path} ]; then
        log_and_print $LEVEL_ERROR "The dir path is empty."
        exit 1
    fi

    if [ "$(id -u)" -eq 0 ]; then
        return 0
    fi

    if [ -d ${_path} ] && [ ! -w ${_path} ]; then
        return 1
    fi

    return 0
}

function create_relative_softlink() {
    local _src_path="$1"
    local _des_path="$2"

    local _des_dir_name=$(dirname $_des_path)
    _src_path=$(readlink -f ${_src_path})
    if [ ! -f "$_src_path" -a ! -d "$_src_path" -a ! -L "$_src_path" ]; then
        return
    fi
    _src_path=$(get_relative_path $_des_dir_name $_src_path)
    if [ -L "${_des_path}" ]; then
        delete_softlink "${_des_path}"
    fi
    ln -sf "${_src_path}" "${_des_path}"
    if [ $? -ne 0 ]; then
        print_log $LEVEL_ERROR "${_src_path} softlink to ${_des_path} failed!"
        return 1
    fi
}

function delete_softlink() {
    local _path="$1"
    # 如果目标路径是个软链接，则移除
    if [ -L "${_path}" ]; then
        local _parent_path=$(dirname ${_path})
        if [ ! -w ${_parent_path} ]; then
            chmod u+w ${_parent_path}
            rm -f "${_path}"
            if [ $? -ne 0 ]; then
                print_log $LEVEL_ERROR "remove softlink ${_path} failed!"
                exit 1
            fi
            chmod u-w ${_parent_path}
        else
            rm -f "${_path}"
            if [ $? -ne 0 ]; then
                print_log $LEVEL_ERROR "remove softlink ${_path} failed!"
                exit 1
            fi
        fi
    fi
}

function create_install_path() {
    local _install_path=$1

    if [ ! -d "${_install_path}" ]; then
        local _ppath=$(dirname ${_install_path})
        while [[ ! -d ${_ppath} ]];do
            _ppath=$(dirname ${_ppath})
        done

        check_dir_permission "${_ppath}"
        if [ $? -ne 0 ]; then
            chmod u+w -R ${_ppath}
            [ $? -ne 0 ] && exit_log 1
        fi

        create_folder "${_install_path}" $USERNAME:$USERGROUP 750
        [ $? -ne 0 ] && exit_log 1
    else
        check_dir_permission "${_install_path}"
        if [ $? -ne 0 ]; then
            chmod u+w -R ${_install_path}
        fi
    fi
}

function remove_empty_dir() {
    if [ -d "$1" ] && [ -z "$(ls -A $1 2>/dev/null)" ] && [[ ! "$1" =~ ^/+$ ]]; then
        if [ ! -w $(dirname $1) ]; then
            chmod u+w $(dirname $1)
            rm -rf "$1"
            if [ $? != 0 ]; then
                print_log $LEVEL_ERROR "delete directory $1 fail"
                exit 1
            fi
            chmod u-w $(dirname $1)
        else
            rm -rf "$1"
            if [ $? != 0 ]; then
                print_log $LEVEL_ERROR "delete directory $1 fail"
                exit 1
            fi
        fi
    fi
}

function get_relative_path() {
    local _relative_to_path=$1
    local _des_path=$2
    echo $(realpath --relative-to=$_relative_to_path $_des_path)
}


# 删除uninstall.sh文件，如果已经没有uninstall_package调用
function remove_uninstall_file_if_no_content() {
    local _file="$1"
    local _num

    if [ ! -f "${_file}" ]; then
        return 0
    fi

    _num=$(grep "^uninstall_package " ${_file} | wc -l)
    if [ ${_num} -eq 0 ]; then
        rm -f "${_file}" > /dev/null 2>&1
        if [ $? -ne 0 ]; then
            log_and_print $LEVEL_WARN "Delete file:${_file} failed, please delete it by yourself."
        fi
    fi
}

# 删除uninstall.sh文件中的uninstall_package函数调用
function remove_uninstall_package() {
    local _file="$1"

    if [ -f "${_file}" ]; then
        sed -i "/uninstall_package \"mindstudio-opprof\/script\"/d" "${_file}"
        if [ $? -ne 0 ]; then
            log_and_print $LEVEL_ERROR "remove ${_file} uninstall_package command failed!"
            exit 1
        fi
    fi
}


function uninstall_tool() {
    # when normal user uninstall package, shell need to restore dir permission
    "$COMMON_PARSER_PATH" --restoremod --package=mindstudio-opprof --username="unknown" --usergroup="unknown" \
        "${install_path}" "${FILELIST_CSV_PATH}"
    if [ $? -ne 0 ]; then
        log_and_print $LEVEL_ERROR "Restore directory written permission failed."
        return 1
    fi

    "$COMMON_PARSER_PATH" --remove --package=mindstudio-opprof "${install_path}" "${FILELIST_CSV_PATH}"
    if [ $? -ne 0 ]; then
        log_and_print $LEVEL_ERROR "ERR_NO:0X0090;ERR_DES: Remove mindstudio-opprof files failed in ${install_path}."
        return 1
    fi
    log $LEVEL_INFO "Remove mindstudio-sanitizer files succeed in ${install_path}!"
    return 0
}
init_log
