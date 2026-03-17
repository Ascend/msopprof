/* -------------------------------------------------------------------------
 *  This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
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

#ifndef __CPPUTILS_FILESYSTEM_H__
#define __CPPUTILS_FILESYSTEM_H__

#include <iterator>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <vector>
#include <regex>
#include <sstream>
#include <experimental/filesystem>
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <linux/limits.h>

#include "log.h"
#include "ustring.h"
#include "securec.h"
#include "data_format.h"

namespace Utility {

constexpr mode_t DIR_DEFAULT_MOD = 0750;
constexpr char const *PATH_SEP = "/";
constexpr const char USER_HOME = '~';
constexpr int FILE_NAME_LENGTH_LIMIT = 255;
constexpr int DIR_NAME_LENGTH_LIMIT = 2048;
constexpr size_t MAX_JSON_FILE_SIZE = 10 * 1024 * 1024;
constexpr size_t INPUT_BINARY_FILE_MAX_SIZE = 10uL * 1024 * 1024 * 1024;
constexpr size_t INPUT_CPP_FILE_MAX_SIZE = 10 * 1024 * 1024;
constexpr size_t INPUT_DUMP_FILE_MAX_SIZE = 1024 * 1024 * 1024;
constexpr size_t INPUT_DUMP_FILE_MIN_SIZE = 500;
constexpr size_t INPUT_FILE_LIMIT_SIZE = 1024 * 1024 * 1024;
constexpr uint64_t INPUT_FILE_MAX_SIZE = UINT64_MAX;
constexpr size_t MAX_NUM_TRAVERSED_FILES = 1024 * 1024 * 1024;
constexpr uint64_t MAX_NUM_FOR_GET_LINE = UINT64_MAX;
constexpr double MAX_MEMORY_USAGE_RATIO = 0.5;
const char WRAP {'\n'};
constexpr size_t MILLISECOND_LENGTH = 4;
constexpr int MAX_DEPTH = 64;
enum class TimeAccuracy {
    SECOND,
    MILLISECOND
};

inline bool GetCurrentWorkingDir(std::string &cwd)
{
    char buf[PATH_MAX] = {'\0'};
    if (getcwd(buf, sizeof(buf)) == nullptr) {
        LogWarn("Get current working dir failed.");
        return false;
    }
    cwd = std::string(buf);
    return true;
}

inline bool ChangeWorkingDir(std::string const &dir)
{
    if (chdir(dir.c_str()) != 0) {
        return false;
    }

    return true;
}

template<typename Iterator>
inline bool ListDir(std::string const &path, Iterator it)
{
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        return false;
    }
    for (struct dirent *ent = readdir(dir); ent != nullptr; ent = readdir(dir)) {
        *it = (ent->d_name);
        ++it;
    }
    closedir(dir);

    return true;
}

inline std::string JoinPath(const std::vector<std::string> &pathVectorList)
{
    return Join(pathVectorList.begin(), pathVectorList.end(), PATH_SEP);
}

inline bool GetLastFile(const std::string &filePath, std::string &file)
{
    size_t right = filePath.size();
    if (right - 1 == filePath.rfind('/')) {
        right = filePath.rfind('/', right - 1);
    }
    size_t left = filePath.rfind('/', right - 1);
    if (left == std::string::npos) {
        return false;
    }
    file = filePath.substr(left + 1, right - left - 1);
    return true;
}

inline bool IsExist(const std::string &checkPath)
{
    struct stat fileStat;
    return stat(checkPath.c_str(), &fileStat) == 0;
}

inline bool IsDir(std::string const &checkPath)
{
    struct stat st;
    return stat(checkPath.c_str(), &st) == 0 && (st.st_mode & S_IFDIR) != 0;
}

inline bool IsReadable(const std::string &checkPath)
{
    struct stat fileStat;
    return (stat(checkPath.c_str(), &fileStat) == 0) && (fileStat.st_mode & S_IRUSR) != 0;
}

inline bool IsWritable(const std::string &checkPath)
{
    struct stat fileStat;
    return (stat(checkPath.c_str(), &fileStat) == 0) && (fileStat.st_mode & S_IWUSR) != 0;
}

inline bool IsExecutable(const std::string &checkPath)
{
    struct stat fileStat;
    return (stat(checkPath.c_str(), &fileStat) == 0) && (fileStat.st_mode & S_IXUSR) != 0;
}

inline bool IsRootUser()
{
    constexpr __uid_t root = 0;
    return getuid() == root;
}

inline bool PathCharacterCheck(const std::string &path)
{
    std::regex re{"(\\.|\\\\|/|:|_|-|\\s|[~0-9a-zA-Z])+"};
    return std::regex_match(path, re);
}

template<typename T>
inline bool WriteFileByStream(const std::string &filePath, const T &data,
                              std::fstream::openmode openMode1 = std::fstream::out,
                              std::fstream::openmode openMode2 = std::fstream::trunc,
                              mode_t fileAuthority = Utility::SAVE_DATA_FILE_AUTHORITY)
{
    std::ofstream targetFile(filePath, openMode1 | openMode2);
    if (!targetFile.is_open()) {
        LogWarn("Can not open [%s]", filePath.c_str());
        return false;
    }
    targetFile << data;
    targetFile.close();
    chmod(filePath.c_str(), fileAuthority);
    return true;
}

template<typename T>
inline bool WriteFileByBinary(const std::string &filePath, const T &data, uint64_t dataSize,
                              std::fstream::openmode openMode1 = std::fstream::out,
                              std::fstream::openmode openMode2 = std::fstream::binary)
{
    std::ofstream targetFile(filePath, openMode1 | openMode2);
    if (!targetFile.is_open()) {
        LogWarn("Can not open [%s]", filePath.c_str());
        return false;
    }
    targetFile.write(data, dataSize);
    targetFile.close();
    chmod(filePath.c_str(), Utility::SAVE_DATA_FILE_AUTHORITY);
    return true;
}

inline void RemoveAll(const std::string& filePath)
{
    using namespace std::experimental::filesystem;
    remove_all(filePath);
}

inline std::string GetLastFile(const std::string &path)
{
    if (path.empty()) {
        return "";
    }
    std::experimental::filesystem::path fsPath(path);
    return fsPath.filename().string();
}

inline std::string GetFileFaPath(const std::string &path, const std::string &fileName)
{
    std::size_t found = path.find_last_of(fileName);
    if (found == std::string::npos) {
        LogWarn("Can not find file name [%s] in path [%s]", fileName.c_str(), path.c_str());
        return "";
    }
    return path.substr(0, found - fileName.length());
}

inline void WriteWithoutAppend(const std::string &path, std::vector<std::string> &lineVec)
{
    std::ofstream writer(path, std::ios::out | std::ios::trunc);
    if (writer.is_open()) {
        for (const auto &line : lineVec) {
            writer << line << std::endl;
        }
        writer.close();
    }
}

inline void ReadFileSaveString(const std::string &filePath, std::string &output)
{
    int readFd = open(filePath.data(), O_RDONLY);
    static constexpr std::size_t bufLen = 1024;
    char buf[bufLen] = {'\0'};
    ssize_t nBytes = 0;
    while ((nBytes = read(readFd, buf, bufLen)) > 0) {
        output.append(buf, static_cast<std::size_t>(nBytes));
    }
    close(readFd);
}

inline void ReadFileSaveStringVec(const std::string &filePath, std::vector<std::string> &output)
{
    std::ifstream reader(filePath);
    std::string line;
    if (reader.is_open()) {
        while (getline(reader, line)) {
            output.push_back(line);
        }
        reader.close();
    }
}

inline std::string Realpath(const std::string &rawPath)
{
    char tmp[PATH_MAX] = {'\0'};
    auto rpath = realpath(rawPath.c_str(), tmp);
    if (rpath) {
        return std::string(rpath);
    }
    return "";
}

std::string RandomizeDir(std::string const &path);
bool ReadFile(const std::string &filePath, uint8_t *buffer, size_t bufferSize);
bool CheckFileSizeValid(const std::string &path, size_t threshold);
size_t GetFileSize(const std::string &filePath);
void WriteBinaryFile(const std::string &filePath, const char *buffer, size_t size);
bool ReadBinaryFile(const std::string &filePath, std::vector<char> &buffer);
std::string GetAbsolutePath(std::string const &path);
bool IsSoftLink(const std::string &path);
bool MkdirRecusively(std::string const &path, mode_t mode = DIR_DEFAULT_MOD);
bool CheckInputFileValid(const std::string &path, const std::string &fileType,
                         size_t threshold = INPUT_BINARY_FILE_MAX_SIZE, std::string paramName = "",
                         LogLv logLv = LogLv::ERROR);
std::string CheckAndReadFile(const std::string &path, const std::string &fileType,
                             size_t threshold = INPUT_BINARY_FILE_MAX_SIZE, std::string paramName = "",
                             LogLv logLv = LogLv::ERROR);
bool GetFileNames(const std::string& path, std::vector<std::string> &fileNames, int maxFileCount = 10000);
bool GetFileLines(std::string path, std::vector<std::string> &fileLines);
bool FilesNumGreaterThanX(const std::string &path, uint32_t x);
void CopyFolder(const std::string &fromPath, const std::string &toPath, const std::string &outputRootPath,
    const std::function<void(const std::string&, const std::string&)>& func = nullptr, int depth = MAX_DEPTH);
bool CopyFile(const std::string &srcPath, const std::string &destPath);
bool GetFileSuffix(const std::string &file, std::string &suffix);
bool Mkdir(std::string const &path, mode_t mode = DIR_DEFAULT_MOD, bool ignoreExist = true);
bool CheckFolder(const std::string &path, std::string &errorMsg, bool ignoreEmpty = false,
    const std::function<bool(const std::string&, std::vector<std::string>&, std::string&)>& fileChecker = nullptr);
bool IsSoftLinkRecursively(const std::string &path);
bool CheckPermission(const std::string& filePath);
uint64_t GetSystemAvailableMemory();
std::string FindExecutableCommand(const std::string &command);
bool PathLenCheckValid(const std::string &checkPath);
bool CheckOwnerPermission(std::string &path, std::string &msg);
bool RollbackPath(std::string &path, uint32_t rollNum);
std::time_t GenerateTimeStamp(std::string& str, TimeAccuracy acc = TimeAccuracy::SECOND);
void ReadFileByMMap(const std::string &fileName, std::vector<std::string> &fileLine);
bool ReadBinFileByMultiStruct(const std::string& filePath, const size_t& fileSize, const size_t& structSize,
                              std::vector<char>& binData);
bool AppendLinesToFile(const std::string& fileName, const std::vector<std::string> &lines);
void SearchDirRecursive(const std::string &searchPath, const std::string &targetDirName, std::vector<std::string> &targetPath, int deep);
} // namespace Gather

#endif  // __CPPUTILS_FILESYSTEM_H__
