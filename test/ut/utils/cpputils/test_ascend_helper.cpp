/* -------------------------------------------------------------------------
 *  This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * ------------------------------------------------------------------------- */

#include <gtest/gtest.h>
#include <cstring>
#include <dlfcn.h>
#include <elf.h>
#include <fstream>
#include <stdlib.h>
#include "mockcpp/mockcpp.hpp"
#define private public
#include "ascend_helper.h"
#undef private
#include "filesystem.h"

using namespace Utility;

class AscendHelperTest : public testing::Test {
protected:
    void SetUp() override {
        const char* tmpAscendHomePath = getenv("ASCEND_HOME_PATH");
        const char* tmpLdLibraryPath = getenv("LD_LIBRARY_PATH");
        hasAscendHomePathEnv_ = tmpAscendHomePath != nullptr;
        hasLdLibraryPathEnv_ = tmpLdLibraryPath != nullptr;
        ascendHomePathEnv_ = (tmpAscendHomePath == nullptr) ? "" : tmpAscendHomePath;
        ldLibraryPathEnv_ = (tmpLdLibraryPath == nullptr) ? "" : tmpLdLibraryPath;
        unsetenv("ASCEND_HOME_PATH");
        unsetenv("LD_LIBRARY_PATH");
    }

    void TearDown() override {
        if (hasAscendHomePathEnv_) {
            setenv("ASCEND_HOME_PATH", ascendHomePathEnv_.c_str(), 1);
        } else {
            unsetenv("ASCEND_HOME_PATH");
        }
        if (hasLdLibraryPathEnv_) {
            setenv("LD_LIBRARY_PATH", ldLibraryPathEnv_.c_str(), 1);
        } else {
            unsetenv("LD_LIBRARY_PATH");
        }
        GlobalMockObject::verify();
    }
    bool hasAscendHomePathEnv_ {false};
    bool hasLdLibraryPathEnv_ {false};
    std::string ascendHomePathEnv_;
    std::string ldLibraryPathEnv_;
};

namespace {
std::string MakeSimulatorLibraryTestDir()
{
    char path[] = "/tmp/msopprof_simulator_library_XXXXXX";
    char *result = mkdtemp(path);
    return result == nullptr ? "" : result;
}

void CreateEmptyFile(const std::string &path)
{
    std::ofstream output(path, std::ios::out | std::ios::trunc);
    output.close();
}

void CreateSimulatorLibrarySet(const std::string &directory)
{
    ASSERT_TRUE(MkdirRecusively(directory));
    CreateEmptyFile(directory + "/libruntime_camodel.so");
    CreateEmptyFile(directory + "/libpem_davinci.so");
}

Elf64_Xword AppendElfString(std::vector<char> &stringTable, const std::string &value) {
    Elf64_Xword offset = stringTable.size();
    stringTable.insert(stringTable.end(), value.begin(), value.end());
    stringTable.emplace_back('\0');
    return offset;
}

void CreateSimulatorLinkedElf(const std::string &path, const std::string &libraryPath, Elf64_Sxword pathTag,
    const std::string &neededLibrary = "libruntime_camodel.so") {
    constexpr size_t dynamicOffset = 0x200;
    constexpr size_t stringTableOffset = 0x300;
    constexpr Elf64_Addr loadAddress = 0x400000;
    std::vector<char> stringTable(1, '\0');
    Elf64_Xword neededOffset = AppendElfString(stringTable, neededLibrary);
    Elf64_Xword pathOffset = AppendElfString(stringTable, libraryPath);
    std::vector<Elf64_Dyn> dynamicEntries(5);
    dynamicEntries[0].d_tag = DT_STRTAB;
    dynamicEntries[0].d_un.d_ptr = loadAddress + stringTableOffset;
    dynamicEntries[1].d_tag = DT_STRSZ;
    dynamicEntries[1].d_un.d_val = stringTable.size();
    dynamicEntries[2].d_tag = DT_NEEDED;
    dynamicEntries[2].d_un.d_val = neededOffset;
    dynamicEntries[3].d_tag = pathTag;
    dynamicEntries[3].d_un.d_val = pathOffset;
    dynamicEntries[4].d_tag = DT_NULL;

    std::vector<char> binary(stringTableOffset + stringTable.size(), 0);
    Elf64_Ehdr header{};
    memcpy(header.e_ident, ELFMAG, SELFMAG);
    header.e_ident[EI_CLASS] = ELFCLASS64;
    header.e_ident[EI_DATA] = ELFDATA2LSB;
    header.e_ident[EI_VERSION] = EV_CURRENT;
    header.e_type = ET_DYN;
    header.e_machine = EM_X86_64;
    header.e_version = EV_CURRENT;
    header.e_ehsize = sizeof(Elf64_Ehdr);
    header.e_phoff = sizeof(Elf64_Ehdr);
    header.e_phentsize = sizeof(Elf64_Phdr);
    header.e_phnum = 2;
    memcpy(binary.data(), &header, sizeof(header));

    Elf64_Phdr loadHeader{};
    loadHeader.p_type = PT_LOAD;
    loadHeader.p_offset = 0;
    loadHeader.p_vaddr = loadAddress;
    loadHeader.p_filesz = binary.size();
    loadHeader.p_memsz = binary.size();
    memcpy(binary.data() + header.e_phoff, &loadHeader, sizeof(loadHeader));

    Elf64_Phdr dynamicHeader{};
    dynamicHeader.p_type = PT_DYNAMIC;
    dynamicHeader.p_offset = dynamicOffset;
    dynamicHeader.p_vaddr = loadAddress + dynamicOffset;
    dynamicHeader.p_filesz = dynamicEntries.size() * sizeof(Elf64_Dyn);
    dynamicHeader.p_memsz = dynamicHeader.p_filesz;
    memcpy(binary.data() + header.e_phoff + sizeof(Elf64_Phdr), &dynamicHeader, sizeof(dynamicHeader));
    memcpy(binary.data() + dynamicOffset, dynamicEntries.data(), dynamicHeader.p_filesz);
    memcpy(binary.data() + stringTableOffset, stringTable.data(), stringTable.size());

    std::ofstream output(path, std::ios::out | std::ios::binary | std::ios::trunc);
    output.write(binary.data(), binary.size());
    output.close();
}

void CreateKernelElf(const std::string &path, Elf64_Half machine, Elf64_Word flags) {
    Elf64_Ehdr header{};
    memcpy(header.e_ident, ELFMAG, SELFMAG);
    header.e_ident[EI_CLASS] = ELFCLASS64;
    header.e_ident[EI_DATA] = ELFDATA2LSB;
    header.e_machine = machine;
    header.e_flags = flags;
    std::ofstream output(path, std::ios::out | std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char *>(&header), sizeof(header));
}
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetAscendHomePath
* |  用例名  | test_GetAscendHomePath_env_not_set_expect_return_false
* | 用例描述 | 测试ASCEND_HOME_PATH环境变量未设置，返回false
*/
TEST_F(AscendHelperTest, test_GetAscendHomePath_env_not_set_expect_return_false)
{
    std::string path;
    bool ret = GetAscendHomePath(path);
    ASSERT_FALSE(ret);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetAscendHomePath
* |  用例名  | test_GetAscendHomePath_invalid_path_expect_return_false
* | 用例描述 | 测试ASCEND_HOME_PATH环境变量路径不存在，返回false
*/
TEST_F(AscendHelperTest, test_GetAscendHomePath_invalid_path_expect_return_false)
{
    setenv("ASCEND_HOME_PATH", "test/ut/resources/dump/invalid", 1);
    std::string path;
    bool ret = GetAscendHomePath(path);
    ASSERT_FALSE(ret);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetAscendHomePath
* |  用例名  | test_GetAscendHomePath_valid_path_expect_return_true
* | 用例描述 | 测试ASCEND_HOME_PATH环境变量路径存在，返回true
*/
TEST_F(AscendHelperTest, test_GetAscendHomePath_valid_path_expect_return_true)
{
    std::string tmpPath = "test/ut/resources/dump/valid";
    MkdirRecusively(tmpPath);
    setenv("ASCEND_HOME_PATH", tmpPath.c_str(), 1);
    std::string path;
    bool ret = GetAscendHomePath(path);
    ASSERT_TRUE(ret);
    std::experimental::filesystem::remove_all(tmpPath);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSimulators
* |  用例名  | test_GetSimulators_env_not_set_expect_return_false
* | 用例描述 | 测试ASCEND_HOME_PATH环境变量未设置，获取simulators失败
*/
TEST_F(AscendHelperTest, test_GetSimulators_env_not_set_expect_return_false)
{
    MOCKER(&GetAscendHomePath)
        .stubs()
        .will(returnValue(false));
    std::vector<std::string> sims;
    bool ret = GetSimulators(sims);
    ASSERT_FALSE(ret);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSimulators
* |  用例名  | test_GetSimulators_list_dir_fail_expect_return_false
* | 用例描述 | 测试ListDir遍历目录失败，返回false
*/
TEST_F(AscendHelperTest, test_GetSimulators_list_dir_fail_expect_return_false)
{
    std::string tmpPath = "test/ut/resources/dump/valid";
    MkdirRecusively(tmpPath);
    setenv("ASCEND_HOME_PATH", tmpPath.c_str(), 1);
    std::vector<std::string> sims;
    bool ret = GetSimulators(sims);
    ASSERT_FALSE(ret);
    std::experimental::filesystem::remove_all(tmpPath);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSimulators
* |  用例名  | test_GetSimulators_no_ascend_dir_expect_return_true
* | 用例描述 | 测试ListDir遍历目录成功，但无Ascend开头目录，返回true，sim为空
*/
TEST_F(AscendHelperTest, test_GetSimulators_no_ascend_dir_expect_return_true)
{
    std::string tmpPath = "test/ut/resources/dump/valid";
    MkdirRecusively(tmpPath + "/tools/simulator/sim");
    setenv("ASCEND_HOME_PATH", tmpPath.c_str(), 1);
    std::vector<std::string> sims;
    bool ret = GetSimulators(sims);
    ASSERT_TRUE(ret);
    ASSERT_EQ(sims.size(), 0);
    std::experimental::filesystem::remove_all(tmpPath);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSimulators
* |  用例名  | test_GetSimulators_success_with_ascend_dir_expect_return_true
* | 用例描述 | 测试ListDir遍历目录成功，过滤出Ascend开头目录，返回true，sim不为空
*/
TEST_F(AscendHelperTest, test_GetSimulators_with_ascend_dir_expect_return_true)
{
    std::string tmpPath = "test/ut/resources/dump/valid";
    MkdirRecusively(tmpPath + "/tools/simulator/Ascend1");
    MkdirRecusively(tmpPath + "/tools/simulator/Ascend2");
    setenv("ASCEND_HOME_PATH", tmpPath.c_str(), 1);
    std::vector<std::string> sims;
    bool ret = GetSimulators(sims);
    ASSERT_TRUE(ret);
    ASSERT_EQ(sims.size(), 2);
    std::experimental::filesystem::remove_all(tmpPath);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSocVersionFromEnvVar
* |  用例名  | test_GetSocVersionFromEnvVar_ascend_home_path_not_set_expect_return_false
* | 用例描述 | 测试ASCEND_HOME_PATH环境变量未设置，返回false
*/
TEST_F(AscendHelperTest, test_GetSocVersionFromEnvVar_ascend_home_path_not_set_expect_return_false)
{
    std::string soc;
    bool ret = GetSocVersionFromEnvVar(soc);
    ASSERT_FALSE(ret);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSocVersionFromEnvVar
* |  用例名  | test_GetSocVersionFromEnvVar_ld_library_path_not_set_expect_return_false
* | 用例描述 | 测试LD_LIBRARY_PATH环境变量未设置，返回false
*/
TEST_F(AscendHelperTest, test_GetSocVersionFromEnvVar_ld_library_path_not_set_expect_return_false)
{
    setenv("ASCEND_HOME_PATH", "test/ut/resources/dump/valid", 1);
    std::string soc;
    bool ret = GetSocVersionFromEnvVar(soc);
    ASSERT_FALSE(ret);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSocVersionFromEnvVar
* |  用例名  | test_GetSocVersionFromEnvVar_path_not_match_expect_return_false
* | 用例描述 | 测试LD_LIBRARY_PATH不匹配正则，返回false
*/
TEST_F(AscendHelperTest, test_GetSocVersionFromEnvVar_path_not_match_expect_return_false)
{
    setenv("ASCEND_HOME_PATH", "test/ut/resources/dump/valid", 1);
    setenv("LD_LIBRARY_PATH", "test/ut/resources/dump/valid", 1);
    std::string soc;
    bool ret = GetSocVersionFromEnvVar(soc);
    ASSERT_FALSE(ret);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSocVersionFromEnvVar
* |  用例名  | test_GetSocVersionFromEnvVar_match_soc_success_expect_return_true
* | 用例描述 | 测试匹配Ascend开头版本号成功，返回true
*/
TEST_F(AscendHelperTest, test_GetSocVersionFromEnvVar_match_soc_success_expect_return_true)
{
    MOCKER(&GetAscendHomePath)
        .stubs()
        .will(returnValue(true));
    setenv("ASCEND_HOME_PATH", "test/ut/resources/dump/valid", 1);
    setenv("LD_LIBRARY_PATH", "test/ut/resources/dump/valid/Ascend910B1/lib", 1);
    std::string soc;
    bool ret = GetSocVersionFromEnvVar(soc);
    ASSERT_TRUE(ret);
    ASSERT_EQ(soc, "Ascend910B1");
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSocVersionFromEnvVar
* |  用例名  | test_GetSocVersionFromEnvVar_match_dav_success_expect_return_true
* | 用例描述 | 测试匹配dav_xxxx映射版本成功，返回true
*/
TEST_F(AscendHelperTest, test_GetSocVersionFromEnvVar_match_dav_success_expect_return_true)
{
    MOCKER(&GetAscendHomePath)
        .stubs()
        .will(returnValue(true));
    setenv("ASCEND_HOME_PATH", "test/ut/resources/dump/valid", 1);
    setenv("LD_LIBRARY_PATH", "test/ut/resources/dump/valid/dav_3510/lib", 1);
    std::string soc;
    bool ret = GetSocVersionFromEnvVar(soc);
    ASSERT_TRUE(ret);
    ASSERT_EQ(soc, "Ascend950PR_9599");
}

TEST_F(AscendHelperTest, test_GetSocVersionFromEnvVar_match_dav_camodel_success_expect_return_true) {
    MOCKER(&GetAscendHomePath).stubs().will(returnValue(true));
    setenv("ASCEND_HOME_PATH", "test/ut/resources/dump/valid", 1);
    setenv("LD_LIBRARY_PATH", "test/ut/resources/dump/valid/dav_3510/camodel", 1);
    std::string soc;
    bool ret = GetSocVersionFromEnvVar(soc);
    ASSERT_TRUE(ret);
    ASSERT_EQ(soc, "Ascend950PR_9599");
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSoFromEnvVar
* |  用例名  | test_GetSoFromEnvVar_env_not_set_expect_return_empty
* | 用例描述 | 测试LD_LIBRARY_PATH环境变量未设置，返回空字符串
*/
TEST_F(AscendHelperTest, test_GetSoFromEnvVar_env_not_set_expect_return_empty)
{
    std::string so = GetSoFromEnvVar("libascend_hal.so");
    ASSERT_TRUE(so.empty());
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSoFromEnvVar
* |  用例名  | test_GetSoFromEnvVar_env_set_error_expect_return_empty
* | 用例描述 | 测试LD_LIBRARY_PATH环境变量设置错误，返回空字符串
*/
TEST_F(AscendHelperTest, test_GetSoFromEnvVar_env_set_error_expect_return_empty)
{
    setenv("LD_LIBRARY_PATH", "/usr/lib64", 1);
    std::string soName = "";
    MOCKER(&Realpath)
        .stubs()
        .will(returnValue(soName));
    std::string so = GetSoFromEnvVar("libascend_hal.so");
    ASSERT_TRUE(so.empty());
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSoFromEnvVar
* |  用例名  | test_GetSoFromEnvVar_expect_return_empty_valid_path
* | 用例描述 | 测试LD_LIBRARY_PATH环境变量设置，返回正确so路径
*/
TEST_F(AscendHelperTest, test_GetSoFromEnvVar_expect_return_empty_valid_path)
{
    setenv("LD_LIBRARY_PATH", "/usr/lib64", 1);
    std::string soName = "/usr/lib64/libascend_hal.so";
    MOCKER(&Realpath)
        .stubs()
        .will(returnValue(soName));
    std::string so = GetSoFromEnvVar("libascend_hal.so");
    ASSERT_EQ(so, "/usr/lib64/libascend_hal.so");
}

TEST_F(AscendHelperTest, test_GetSimulatorLibrarySource_search_order_and_unknown_directory)
{
    std::string root = MakeSimulatorLibraryTestDir();
    ASSERT_FALSE(root.empty());
    std::string libDir = root + "/dav_3510/lib";
    std::string camodelDir = root + "/dav_3510/camodel";
    std::string unknownDir = root + "/dav_3510/custom";
    CreateSimulatorLibrarySet(libDir);
    CreateSimulatorLibrarySet(camodelDir);
    CreateSimulatorLibrarySet(unknownDir);

    ASSERT_EQ(GetSimulatorLibrarySource(camodelDir + ":" + libDir), SimulatorLibrarySource::CAMODEL);
    ASSERT_EQ(GetSimulatorLibrarySource(libDir + ":" + camodelDir), SimulatorLibrarySource::LIB);
    ASSERT_EQ(GetSimulatorLibrarySource(unknownDir), SimulatorLibrarySource::UNKNOWN);
    std::experimental::filesystem::remove_all(root);
}

TEST_F(AscendHelperTest, test_GetSimulatorLibrarySource_resolves_symlink_and_rejects_mixed_libraries)
{
    std::string root = MakeSimulatorLibraryTestDir();
    ASSERT_FALSE(root.empty());
    std::string camodelDir = root + "/dav_3510/camodel";
    CreateSimulatorLibrarySet(camodelDir);
    std::string latestDir = root + "/latest";
    ASSERT_EQ(symlink(camodelDir.c_str(), latestDir.c_str()), 0);
    ASSERT_EQ(GetSimulatorLibrarySource(latestDir), SimulatorLibrarySource::CAMODEL);

    std::string runtimeDir = root + "/mixed/camodel";
    std::string pemDir = root + "/mixed/lib";
    ASSERT_TRUE(MkdirRecusively(runtimeDir));
    ASSERT_TRUE(MkdirRecusively(pemDir));
    CreateEmptyFile(runtimeDir + "/libruntime_camodel.so");
    CreateEmptyFile(pemDir + "/libpem_davinci.so");
    ASSERT_EQ(GetSimulatorLibrarySource(runtimeDir + ":" + pemDir), SimulatorLibrarySource::UNKNOWN);
    std::experimental::filesystem::remove_all(root);
}

TEST_F(AscendHelperTest, test_GetAscend950SimulatorLibPath_detects_rpath_and_runpath_lib) {
    std::string root = MakeSimulatorLibraryTestDir();
    ASSERT_FALSE(root.empty());
    std::string binaryDir = root + "/bin";
    std::string libDir = root + "/dav_3510/lib";
    ASSERT_TRUE(MkdirRecusively(binaryDir));
    CreateSimulatorLibrarySet(libDir);
    std::string rpathBinary = binaryDir + "/rpath_operator";
    CreateSimulatorLinkedElf(rpathBinary, "$ORIGIN/../dav_3510/lib", DT_RPATH);

    std::string detectedSearchPath;
    ASSERT_TRUE(GetAscend950SimulatorLibPath(rpathBinary, detectedSearchPath));
    ASSERT_EQ(GetSimulatorRuntimePath(detectedSearchPath), Realpath(libDir + "/libruntime_camodel.so"));

    std::string runpathBinary = binaryDir + "/runpath_operator";
    CreateSimulatorLinkedElf(runpathBinary, "$ORIGIN/../dav_3510/lib", DT_RUNPATH);
    ASSERT_TRUE(GetAscend950SimulatorLibPath(runpathBinary, detectedSearchPath));
    ASSERT_EQ(GetSimulatorRuntimePath(detectedSearchPath), Realpath(libDir + "/libruntime_camodel.so"));
    std::experimental::filesystem::remove_all(root);
}

TEST_F(AscendHelperTest, test_GetAscend950SimulatorLibPath_rejects_unrelated_or_non_a5_binary) {
    std::string root = MakeSimulatorLibraryTestDir();
    ASSERT_FALSE(root.empty());
    std::string libDir = root + "/dav_3510/lib";
    CreateSimulatorLibrarySet(libDir);
    std::string detectedSearchPath;

    std::string unrelatedBinary = root + "/unrelated_operator";
    CreateSimulatorLinkedElf(unrelatedBinary, libDir, DT_RPATH, "libm.so.6");
    ASSERT_FALSE(GetAscend950SimulatorLibPath(unrelatedBinary, detectedSearchPath));
    ASSERT_TRUE(detectedSearchPath.empty());

    std::string otherLibDir = root + "/dav_2201/lib";
    CreateSimulatorLibrarySet(otherLibDir);
    std::string otherSimulatorBinary = root + "/other_simulator_operator";
    CreateSimulatorLinkedElf(otherSimulatorBinary, otherLibDir, DT_RPATH);
    ASSERT_FALSE(GetAscend950SimulatorLibPath(otherSimulatorBinary, detectedSearchPath));

    std::string invalidBinary = root + "/invalid_operator";
    CreateEmptyFile(invalidBinary);
    ASSERT_FALSE(GetAscend950SimulatorLibPath(invalidBinary, detectedSearchPath));
    std::experimental::filesystem::remove_all(root);
}

TEST_F(AscendHelperTest, test_DetectAscend950Kernel_distinguishes_read_result_and_a5_flags) {
    std::string root = MakeSimulatorLibraryTestDir();
    ASSERT_FALSE(root.empty());
    bool isAscend950 = false;

    // 两种 flags 分别覆盖 CANN 内置 kernel 和当前 ASC 编译器生成的 dav-3510 kernel。
    std::string builtInKernel = root + "/builtin.o";
    CreateKernelElf(builtInKernel, 0x1029, 0x990000);
    ASSERT_TRUE(DetectAscend950Kernel(builtInKernel, isAscend950));
    EXPECT_TRUE(isAscend950);

    std::string compiledKernel = root + "/compiled.o";
    CreateKernelElf(compiledKernel, 0x1029, 0x9a0000);
    ASSERT_TRUE(DetectAscend950Kernel(compiledKernel, isAscend950));
    EXPECT_TRUE(isAscend950);

    // 其他合法 flags 读取成功但不判定为 A5，以保持旧平台及未知编译产物的兼容行为。
    std::string otherKernel = root + "/other.o";
    CreateKernelElf(otherKernel, 0x1029, 0x940000);
    ASSERT_TRUE(DetectAscend950Kernel(otherKernel, isAscend950));
    EXPECT_FALSE(isAscend950);

    std::string invalidKernel = root + "/invalid.o";
    CreateEmptyFile(invalidKernel);
    EXPECT_FALSE(DetectAscend950Kernel(invalidKernel, isAscend950));
    EXPECT_FALSE(isAscend950);
    std::experimental::filesystem::remove_all(root);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSimulatorDirName
* |  用例名  | test_GetSimulatorDirName_ascend910_93_map_to_dav_2201
* | 用例描述 | 测试Ascend910_93系列soc版本映射到dav_2201仿真器目录
*/
TEST_F(AscendHelperTest, test_GetSimulatorDirName_ascend910_93_map_to_dav_2201)
{
    ASSERT_EQ(GetSimulatorDirName("Ascend910_9391"), "dav_2201");
    ASSERT_EQ(GetSimulatorDirName("Ascend910_9362"), "dav_2201");
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSimulatorDirName
* |  用例名  | test_GetSimulatorDirName_ascend950_map_to_dav_3510
* | 用例描述 | 测试Ascend950系列soc版本映射到dav_3510仿真器目录
*/
TEST_F(AscendHelperTest, test_GetSimulatorDirName_ascend950_map_to_dav_3510)
{
    ASSERT_EQ(GetSimulatorDirName("Ascend950PR_9599"), "dav_3510");
    ASSERT_EQ(GetSimulatorDirName("Ascend950DT_9573"), "dav_3510");
}

TEST_F(AscendHelperTest, test_GetSimulatorLibrarySearchPath_ascend950_uses_soc_camodel) {
    std::string root = MakeSimulatorLibraryTestDir();
    ASSERT_FALSE(root.empty());
    setenv("ASCEND_HOME_PATH", root.c_str(), 1);
    setenv("LD_LIBRARY_PATH", "/tmp/tools/simulator/dav_3510/lib", 1);

    ASSERT_EQ(GetSimulatorLibrarySearchPath("Ascend950PR_9599"), root + "/tools/simulator/dav_3510/camodel");
    std::experimental::filesystem::remove_all(root);
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSimulatorDirName
* |  用例名  | test_GetSimulatorDirName_other_soc_passthrough
* | 用例描述 | 测试其余soc版本与dav目录名原样透传
*/
TEST_F(AscendHelperTest, test_GetSimulatorDirName_other_soc_passthrough)
{
    ASSERT_EQ(GetSimulatorDirName("Ascend910B4"), "Ascend910B4");
    ASSERT_EQ(GetSimulatorDirName("Ascend310P3"), "Ascend310P3");
    ASSERT_EQ(GetSimulatorDirName("dav_2201"), "dav_2201");
    ASSERT_EQ(GetSimulatorDirName(""), "");
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSocVersionBySimDir
* |  用例名  | test_GetSocVersionBySimDir_dav_dir_map_to_default_soc
* | 用例描述 | 测试dav仿真器目录名映射回默认Ascend型号名
*/
TEST_F(AscendHelperTest, test_GetSocVersionBySimDir_dav_dir_map_to_default_soc)
{
    ASSERT_EQ(GetSocVersionBySimDir("dav_2002"), "Ascend310P1");
    ASSERT_EQ(GetSocVersionBySimDir("dav_2201"), "Ascend910B1");
    ASSERT_EQ(GetSocVersionBySimDir("dav_3510"), "Ascend950PR_9599");
}

/**
* |  用例集  | AscendHelper
* | 测试函数 | GetSocVersionBySimDir
* |  用例名  | test_GetSocVersionBySimDir_unknown_name_passthrough
* | 用例描述 | 测试未收录的目录名与Ascend型号名原样透传
*/
TEST_F(AscendHelperTest, test_GetSocVersionBySimDir_unknown_name_passthrough)
{
    ASSERT_EQ(GetSocVersionBySimDir("Ascend910B4"), "Ascend910B4");
    ASSERT_EQ(GetSocVersionBySimDir("dav_9999"), "dav_9999");
    ASSERT_EQ(GetSocVersionBySimDir(""), "");
}
