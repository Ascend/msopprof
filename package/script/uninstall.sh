#!/bin/bash
USERNAME=$(id -un)
USERGROUP=$(id -gn)

SHELL_DIR=$(cd "$(dirname "$0")" || exit; pwd)
COMMON_PARSER_PATH=${SHELL_DIR}"/parser_install.sh"
COMMON_SHELL_PATH=${SHELL_DIR}"/common.sh"
FILELIST_CSV_PATH=${SHELL_DIR}"/filelist.csv"


function print_usage() {
    local _ret=$1

    echo "Usage: $0 [Options]"
    echo "Options:"
    echo "    --help | -h   : Print out this help message"
    echo "    --quiet       : Quiet uninstall mode, skip human-computer interactions"
    exit ${_ret}
}

function real_install_path() {
    if [ ! -f ${install_file} ]; then
        log_and_print $LEVEL_WARN "Install file ${install_file} doesn't exist."
        return 1
    fi
    install_path=$(get_install_param "Install_Path_Param" ${install_file})
    if [ -z "${install_path}" ]; then
        log_and_print $LEVEL_WARN "Install path is empty from 'ascend_install.info'."
        return 1
    fi
    install_path=$(cd "${install_path}" && pwd)
    if [ $? -ne 0 ]; then
        log_and_print $LEVEL_WARN "Install path ${install_path} doesn't exist which read from 'ascend_install.info'."
        return 1
    fi
    local _path=$(dirname ${install_path})
    return 0
}

function check_dir_permission() {
    local _path=$1

    if [ -z ${_path} ]; then
        log_and_print $LEVEL_ERROR "The dir path is empty, uninstall failed."
        exit_log 1
    fi
    if [ ! -d "${_path}" ]; then
        log_and_print $LEVEL_ERROR "The dir path ${_path} does not exist, uninstall failed."
        exit_log 1
    fi
    if [ "$(id -u)" -eq 0 ]; then
        return
    fi
    if [ ! -r ${_path} ] || [ ! -w ${_path} ] || [ ! -x ${_path} ]; then
        log_and_print $LEVEL_ERROR "The user $USERNAME should have read, write and executable permission for ${_path}."
        exit_log 1
    fi
}

function uninstall() {
    install_file=$(cd ${SHELL_DIR}"/.." && pwd)/ascend_install.info

    real_install_path
    [ $? -ne 0 ] && return 1
    check_dir_permission ${install_path}
    check_dir_permission ${install_path}/share/info/mindstudio-opprof

    uninstall_tool
    if [ $? -ne 0 ]; then
        log_and_print ${LEVEL_ERROR} "${MODULE_NAME} uninstall failed."
        return 1
    fi
    rm -f ${install_file}

    remove_uninstall_file_if_no_content ${install_path}/cann_uninstall.sh
    
    remove_empty_dir ${install_path}/share/info/mindstudio-opprof
    remove_empty_dir ${install_path}/share/info
    remove_empty_dir ${install_path}/share
    remove_empty_dir ${install_path}/${ARCH}-${OS}/include
    remove_empty_dir ${install_path}
    remove_empty_dir $(dirname ${install_path})
    log_and_print $LEVEL_INFO "${MODULE_NAME} uninstall success!"
}

source ${COMMON_SHELL_PATH}

install_file=""
quiet_flag=n

start_log

log_and_print $LEVEL_INFO "LogFile: $log_file"
log_and_print $LEVEL_INFO "InputParams: $*"

while true; do
    case "$1" in
    --quiet)
        quiet_flag=y
        shift
        ;;
    --help | -h)
        print_usage 0
        shift
        ;;
    -*)
        echo "Unsupported parameters: $1"
        print_usage 1
        ;;
    *)
        break
        ;;
    esac
done

uninstall
exit_log 0
