#!/usr/bin/env python
# -*- coding: UTF-8 -*
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
import argparse
import copy
import hashlib
import inspect
import os
import re
import shutil
import subprocess
import sys
import time
import traceback
import xml.etree.ElementTree as ET

THIS_FILE_NAME = __file__
CURRENT_DIR = os.path.abspath(os.path.dirname(os.path.realpath(__file__)))
TOP_DIR = os.path.join(CURRENT_DIR, '../../')
TARGET_ENV = '${TARGET_ENV}'
OUTPUT = '${OUTPUT}'
SUCC = 0
FAIL = -1
LOG_E = "ERROR"
LOG_W = "WARNING"
LOG_I = "INFO"

def log_print_element(log_element):
    print("[" + log_element + "]", end=' ')
    return

def log_msg(log_level, log_msg, *log_paras):
    log_timestamp = time.strftime("%Y-%m-%d %H:%M:%S",time.localtime())
    line_no = inspect.currentframe().f_back.f_lineno

    log_print_element(log_timestamp)
    log_print_element(log_level)
    log_print_element(THIS_FILE_NAME)
    log_print_element(str(line_no))

    print(log_msg % log_paras)
    return

class Xmlparser(object):
    """
    功能描述: 解析xml配置文件。
    """
    def __init__(self, xml_file, delivery_dir, os_arch):
        self.xml_file = xml_file
        self.delivery_dir = delivery_dir
        self.default_config = {}
        self.target_env="{0}-{1}".format(os_arch, "linux")

        self._cache_dir_info = {}
        self._dir_install_list = []
        self._expand_content_list = []
        self._package_content_list = []

    @property
    def dir_install_list(self):
        return self._dir_install_list

    @property
    def expand_content_list(self):
        return self._expand_content_list

    @property
    def package_content_list(self):
        return self._package_content_list

    def parse(self):
        try:
            tree = ET.parse(self.xml_file)
            root = tree.getroot()
            self.default_config = list(root.iter('config'))[0].attrib
            self._parse_filelist_info(root)
        except Exception as e:
            log_msg(LOG_E, "xmlparse %s failed: %s!", self.xml_file, e)
            log_msg(LOG_I, "%s", traceback.format_exc())
            return FAIL
        return SUCC

    def _parse_filelist_info(self, xmlobj):
        self._parse_dir_infos(xmlobj.findall('dir_info'))
        self._parse_file_infos(xmlobj.findall('file_info'))

    def _parse_dir_infos(self, dir_infos):
        for item in dir_infos:
            dir_config = self.default_config.copy()
            dir_config.update(item.attrib)
            dir_config['module'] = dir_config['value']
            for sub_item in list(item.iter())[1:]:
                dir_info = dir_config.copy()
                dir_info.update(sub_item.attrib)
                dir_info = self.pre_parse(dir_info)
                self.add_dir_info(dir_info)

    def _parse_file_infos(self, file_infos):
        for item in file_infos:
            file_config = self.default_config.copy()
            file_config.update(item.attrib)
            file_config['module'] = file_config['value']
            for sub_item in list(item.iter())[1:]:
                file_info = file_config.copy()
                file_info.update(sub_item.attrib)
                file_info = self.pre_parse(file_info)

                configurable = file_info.get('configurable', 'FALSE').upper()
                file_info['configurable'] = configurable

                src_target = self.get_src_target(file_info=file_info)
                if os.path.isdir(src_target):
                    # 如果当前是文件夹，需要展开计算
                    expand_file_info_list, expand_dir_info_list = self.expand_dir(file_info)
                    self._expand_content_list.extend(expand_file_info_list)
                    self.add_dir_info_list(expand_dir_info_list)
                else:
                    # 如果配置了configurable，需要计算文件的hash值
                    if configurable == 'TRUE':
                        hash_value = self.make_hash(file_info)
                        file_info['hash'] = hash_value
                self._package_content_list.append(file_info)

    def get_src_target(self, file_info):
        """
        获取文件的实际路径
        :param file_info:
        :return:src_target
        """
        copy_type = file_info.get('copy_type', None)
        if copy_type == 'delivery':
            src_target = os.path.join(
                self.delivery_dir, file_info.get(
                    'src_path'), file_info.get('value')
            )
        elif copy_type == 'source':
            src_target = os.path.join(
                TOP_DIR, file_info.get('src_path'), file_info.get('value')
            )
        else:
            raise Exception("error copy_type=%s" % (copy_type))
        return src_target

    def expand_dir(self, file_info):
        """
        如果file_info中配置的路径是文件夹，需要展开到文件
        :param file_info:
        :return:
        """
        file_info_list = []
        dir_info_list = []
        src_target = self.get_src_target(file_info=file_info)
        if not os.path.isdir(src_target):
            file_info_list.append(file_info)
            return file_info_list, dir_info_list

        value_list = file_info.get('value').split('/')
        target_name = value_list[-1] if value_list[-1] else value_list[-2]

        # 这里把当前目录也加入到dir_info_list中
        dir_info_copy = file_info.copy()
        dir_info_copy['module'] = file_info['value']
        dir_info_copy['value'] = os.path.join(
            file_info.get('install_path', ''), target_name
        )

        # 被展开的当前目录不需要设置softlink
        dir_info_copy['install_softlink'] = 'NA'
        dir_info_list.append(dir_info_copy)

        for root, dirs, files in os.walk(src_target):
            if ".git" in dirs:
                dirs.remove(".git")
            dirs.sort()
            files.sort()
            for name in dirs:
                dirname = os.path.join(root, name)
                if os.path.islink(dirname):
                    # 如果是指向目录的软连接，则按照文件处理，无需在安装时创建目录，只需要卸载时删除就行
                    relative_filename = os.path.relpath(dirname, src_target)
                    relative_dir_name = os.path.split(relative_filename)[0]
                    copy_file_info = file_info.copy()
                    copy_file_info['value'] = name
                    copy_file_info['src_path'] = os.path.join(
                        file_info['src_path'], file_info['value'], relative_dir_name
                    )
                    copy_file_info['dst_path'] = os.path.join(
                        file_info['dst_path'], target_name, relative_dir_name
                    )
                    copy_file_info['install_path'] = os.path.join(
                        file_info.get('install_path', ''), target_name, relative_dir_name
                    )
                    copy_file_info['install_softlink'] = 'NA'
                    file_info_list.append(copy_file_info)
                    continue
                relative_dirname = os.path.relpath(dirname, src_target)
                dir_info_copy = file_info.copy()
                dir_info_copy['module'] = file_info['value']
                dir_info_copy['value'] = os.path.join(
                    file_info.get('install_path', ''), target_name, relative_dirname
                )
                dir_info_copy['install_softlink'] = 'NA'
                dir_info_list.append(dir_info_copy)
            for name in files:
                filename = os.path.join(root, name)
                relative_filename = os.path.relpath(filename, src_target)
                relative_dir_name = os.path.split(relative_filename)[0]
                copy_file_info = file_info.copy()
                # 将遍历到的子文件的相对目录加到对应的src_path,dst_path,install_path中
                copy_file_info['value'] = name
                copy_file_info['src_path'] = os.path.join(
                    file_info['src_path'], file_info['value'], relative_dir_name
                )
                copy_file_info['dst_path'] = os.path.join(
                    file_info['dst_path'], target_name, relative_dir_name
                )
                copy_file_info['install_path'] = os.path.join(
                    file_info.get('install_path', ''), target_name, relative_dir_name
                )
                file_info_list.append(copy_file_info)
        return file_info_list, dir_info_list

    def make_hash(self, file_info):
        """
        计算文件的hash(sha256)值
        :param file_info:
        :return: hash_value
        """
        src_target = self.get_src_target(file_info=file_info)
        sha256_hash = hashlib.sha256()
        with open(src_target, "rb") as f:
            sha256_hash.update(f.read())
        value = sha256_hash.hexdigest()
        return value

    def add_dir_info(self, dir_info):
        """
        往dir_install_list变量添加dir_info, 这里封装主要是为了避免重复添加
        :param dir_info:
        :return:
        """
        if dir_info['value'] in self._cache_dir_info:
            return False
        else:
            self._cache_dir_info[dir_info['value']] = dir_info
            self._dir_install_list.append(dir_info)
            return True

    def add_dir_info_list(self, dir_info_list):
        for dir_info in dir_info_list:
            self.add_dir_info(dir_info)

    def pre_parse(self, item_attr):
        """
        xml配置属性解析预处理

         转换配置属性中的$(TARGET_ENV)变量
        """
        # 配置属性中$(TARGET_ENV)变量替换
        if item_attr.get("value", None):
            item_attr["value"] = item_attr["value"].replace(
                TARGET_ENV, self.target_env)
        if item_attr.get("install_path", None):
            item_attr["install_path"] = item_attr["install_path"].replace(
                TARGET_ENV, self.target_env)
        if item_attr.get("install_softlink", None):
            item_attr["install_softlink"] = item_attr["install_softlink"].replace(
                TARGET_ENV, self.target_env)
        if item_attr.get("src_path", None):
            item_attr["src_path"] = item_attr["src_path"].replace(
                OUTPUT, self.delivery_dir)
        return item_attr


def do_copy(target_conf={}, delivery_dir='', release_dir=''):
    '''
    功能描述: 根据拷贝类型来执行文件或目录拷贝
    参数:  target_conf, delivery_dir, release_dir
        target_conf:{'value': ,'copy_type': ,'src_path': ,'dst_path': }
    返回值: SUCC/FAIL
    '''
    copy_type = target_conf.get('copy_type')
    if copy_type == 'delivery':
        src_target = os.path.join(delivery_dir, target_conf.get(
            'src_path'), target_conf.get('value'))
    elif copy_type == 'source':
        src_target = os.path.join(TOP_DIR, target_conf.get(
            'src_path'), target_conf.get('value'))
    else:
        log_msg(LOG_E, "copy_type %s is not support for src_target %s!", copy_type, src_target)
        return FAIL
    value_list = target_conf.get('value').split('/')
    target_name = value_list[-1] if value_list[-1] else value_list[-2]
    dst_path = os.path.join(release_dir, target_conf.get('dst_path', ''))
    pkg_mod = target_conf.get('pkg_mod', '')
    rename = target_conf.get('rename')
    cmd = ''
    if not os.path.exists(dst_path):
        cmd += ' '.join(['mkdir', '-p', dst_path, '&&'])
    if rename:
        dst_path = os.path.join(dst_path, rename)
    cmd += ' '.join(['cp -rf', src_target, dst_path])
    status, output = subprocess.getstatusoutput(cmd)
    if status != SUCC:
        log_msg(LOG_E, "do_copy(%s) failed!", cmd)
        log_msg(LOG_I, "%s", output)
        return FAIL

    cmd = " chmod %s -R %s" % (pkg_mod,
                os.path.join(dst_path, target_name))
    if pkg_mod:
        status, output = subprocess.getstatusoutput(cmd)
    if status != SUCC:
        log_msg(LOG_E, "%s failed!.", cmd)
        log_msg(LOG_I, "%s", output)
        return FAIL
    pkg_softlink = target_conf.get('pkg_softlink')
    if pkg_softlink:
        source = os.path.join(dst_path, target_conf.get('value'))
        link_target = os.path.join(release_dir, pkg_softlink)
        return creat_softlink(source, link_target)
    return SUCC

def creat_softlink(source, target):
    '''
    功能描述: 创建软连接
    参数:  source, target
    返回值: SUCC/FAIL
    '''
    source = os.path.abspath(source.strip())
    target = os.path.abspath(target.strip())

    link_target_path = os.path.dirname(target)
    link_target_name = os.path.basename(target)
    relative_path = os.path.relpath(source, link_target_path)
    if os.path.isfile(target):
        cmd = 'rm -f {}'.format(target)
        status, output = subprocess.getstatusoutput(cmd)
        if status != SUCC:
            log_msg(LOG_E, "rm -f %s failed, %s" , target, output)
            return FAIL
    if os.path.isdir(target):
        log_msg(LOG_E, "%s is directory, can't add softlink", target)
        return FAIL
    if not os.path.exists(link_target_path):
        os.mkdir(link_target_path)
    tmp_dir = os.getcwd()
    os.chdir(link_target_path)
    os.symlink(relative_path, link_target_name)
    os.chdir(tmp_dir)
    return SUCC

def parse_install_info(target_config, operate_type):
    """
    功能描述: 根据配置解析生成安装信息
    参数:  target_config, operate_type
    返回值: install_info
    """
    install_info_list = []
    install_info_list.append(target_config.get('module', 'NA'))
    install_info_list.append(operate_type)
    value_list = target_config.get('value').split('/')
    target_rename = target_config.get('rename')
    if target_rename:
        target_name = target_rename
    else:
        target_name = value_list[-1] if value_list[-1] else value_list[-2]
    if operate_type == 'copy':
        install_info_list.append(
            os.path.join(target_config.get('dst_path'), target_name)
        )
        install_info_list.append(
            os.path.join(target_config.get('install_path', ''), target_name)
        )
    elif operate_type == 'mkdir':
        install_info_list.append('NA')
        install_info_list.append(target_config.get('value'))
    elif operate_type == 'del':
        install_info_list.append('NA')
        install_info_list.append(
            os.path.join(target_config.get('install_path', ''), target_name)
        )
    else:
        raise Exception("error operate_type=%s" % operate_type)

    install_info_list.append(target_config.get('install_mod', 'NA'))
    install_info_list.append(target_config.get('install_own', 'NA').replace('$', '\\\\$'))
    install_info_list.append(target_config.get('install_softlink', 'NA'))
    install_info_list.append(target_config.get('configurable', 'FALSE'))
    install_info_list.append(target_config.get('hash', 'NA'))
    install_info = ','.join(install_info_list)
    return install_info


def generate_filelist(file_content_list, delivery_path):
    """
    功能描述: 组装生成安装文件filelist.csv
    参数:  file_content_list, delivery_path
    返回值: SUCC/FAIL
    """
    content_list = [
        "module,operation,relative_path_in_pkg,relative_install_path,\
        permission,owner:group,softlink,configurable,hash"
    ]
    content_list.extend(file_content_list)
    file_str = '\n'.join(content_list)
    filelist_path = os.path.join(delivery_path, 'filelist.csv')
    try:
        with open(filelist_path, 'w') as file:
            file.write(file_str)
    except Exception as e:
        log_msg(LOG_E, "generate filelist.csv failed: %s!", e)
        return FAIL
    parser_script = os.path.join(
        TOP_DIR, 'package/script/parser_install.sh')
    shutil.copy(parser_script, delivery_path)
    return SUCC


def main(args=None):
    """
    功能描述: 执行打包流程(解析配置--->生成文件列表--->执行拷贝/打包动作)
    参数: args
    返回值: SUCC/FAIL
    """
    delivery_dir = args.delivery_path

    if not os.path.exists(delivery_dir):
        os.makedirs(delivery_dir)

    # 可以通過build_rule指定xml_file，而且优先级高于默认值
    if args.xml_file:
        pkg_xml_file = args.xml_file
    else:
        log_msg(LOG_E, "Input xml_file is None, please check paramters!")
        return FAIL

    xmlparser = Xmlparser(pkg_xml_file, delivery_dir, args.os_arch)
    if xmlparser.parse():
        sys.exit(FAIL)

    file_install_list = []
    for item in xmlparser.dir_install_list:
        file_install_list.append(parse_install_info(item, 'mkdir'))
    for item in xmlparser.package_content_list:
        file_install_list.append(parse_install_info(item, 'copy'))
    for item in xmlparser.expand_content_list:  # file_info中配置为文件夹，这里是被展开的文件,则需要单独删除
        file_install_list.append(parse_install_info(item, 'del'))

    # 生成filelist.csv
    if generate_filelist(file_install_list, delivery_dir):
        return FAIL

    release_dir = os.path.join(
        delivery_dir, xmlparser.default_config.get('name', ''))
    # 拷贝文件
    status = SUCC
    for item in xmlparser.package_content_list:
        if do_copy(item, delivery_dir, release_dir):
            status = FAIL
            continue
    return status


def args_prase():
    """
    功能描述 : 脚本入参解析
    参数 : 调用脚本的传参
    返回值 : 解析后的参数值
    """
    parser = argparse.ArgumentParser(description='This script is for spiltpackage repack processing.')
    parser.add_argument('-x', '--xml', metavar='xml_file', required=True, dest='xml_file', nargs='?', const='',
                        default='', help="This parameter define xml file")
    parser.add_argument('--delivery_path', metavar='delivery_path', required=True, dest='delivery_path', nargs='?', const='',
                        help="This parameter define the delivery path for all module." )
    parser.add_argument('-o', '--os_arch', metavar='os_arch', required=False, dest='os_arch', nargs='?', const='',
                        default=None, help="This parameter define the package's os_arch")
    return parser.parse_args()


if __name__ == "__main__":
    os.chdir(CURRENT_DIR)
    log_msg(LOG_I, "%s", " ".join(sys.argv))
    args = args_prase()
    try:
        status = main(args)
    except Exception as e:
        log_msg(LOG_E, "exception is occurred (%s)!", e)
        log_msg(LOG_I, "%s", traceback.format_exc())
        status = FAIL
    sys.exit(status)
