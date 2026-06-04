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


#include <cstdlib>
#include <random>
#include <cstring>
#include <vector>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <limits>
#include <sys/mman.h>
#include <array>
#include <dirent.h>
#include <sys/types.h>
#include "number_operation.h"
#include "filesystem.h"

namespace Utility {
std::string RandomizeDir(std::string const &path)
{
    auto sepPos = path.find_last_not_of(PATH_SEP);
    std::string pathWithoutTail = path.substr(0, sepPos + 1);

    /// generate time stamp
    std::string buf;
    GenerateTimeStamp(buf);

    /// generate random alpha sequence using std::random_device (backed by /dev/urandom on Linux)
    constexpr std::size_t seqLen = 16;
    constexpr std::size_t nAlpha = 26;
    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, nAlpha - 1);
    std::string randomSeq;
    for (std::size_t i = 0; i < seqLen; ++i) {
        randomSeq.push_back(static_cast<char>('A' + dist(rd)));
    }

    /// randomize dir constructed with <path>_<time-stamp>_<random-seq>
    return pathWithoutTail + "_" + buf + "_" + randomSeq;
}

bool ReadFile(const std::string &filePath, uint8_t *buffer, size_t bufferSize)
{
    if (buffer == nullptr) {
        LogError("Read file [%s] failed for buffer is null", filePath.c_str());
        return false;
    }
    size_t fileSize = GetFileSize(filePath);
    if (fileSize != bufferSize) {
        LogError("The size of file [%s] is not correct", filePath.c_str());
        return false;
    }
    std::ifstream file(filePath, std::ios::binary);
    if (file.fail()) {
        LogError("Can not open file [%s] for reading", filePath.c_str());
        return false;
    }
    file.read(reinterpret_cast<char *>(buffer), bufferSize);
    file.close();
    return true;
}

size_t GetFileSize(const std::string &filePath)
{
    if (!IsExist(filePath)) {
        return 0;
    }
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) != 0 || !S_ISREG(fileStat.st_mode)) {
        return 0;
    }
    struct stat sBuf;
    stat(filePath.data(), &sBuf);
    size_t filesize = static_cast<size_t>(sBuf.st_size);
    return filesize;
}

bool CheckFileSizeValid(const std::string &path, size_t threshold)
{
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) != 0 || !S_ISREG(fileStat.st_mode)) {
        return false;
    }
    return GetFileSize(path) <= threshold;
}

void WriteBinaryFile(const std::string &filePath, const char *buffer, size_t size)
{
    std::ofstream outfile(filePath, std::ifstream::binary);
    if (!outfile.is_open()) {
        LogWarn("can not open file [%s]", filePath.c_str());
        return;
    }
    chmod(filePath.c_str(), Utility::SAVE_DATA_FILE_AUTHORITY);
    outfile.write(buffer, size);
    outfile.close();
}

bool ReadBinaryFile(const std::string &filePath, std::vector<char> &buffer)
{
    std::ifstream binaryFile(filePath, std::ios::ate | std::ios::binary);
    if (!binaryFile.is_open()) {
        LogError("Failed to open binary file %s", filePath.c_str());
        return false;
    }
    std::streampos fileSize = binaryFile.tellg();
    if (fileSize < 0) {  // 处理无效文件大小
        LogError("Invalid file size for %s", filePath.c_str());
        return false;
    }
    if (buffer.size() < static_cast<size_t>(fileSize)) {
        buffer.resize(static_cast<size_t>(fileSize));  // 调整 vector 大小以容纳文件数据
    }

    binaryFile.seekg(0, std::ios::beg);
    binaryFile.read(buffer.data(), fileSize);
    binaryFile.close();
    return true;
}

bool FilesNumGreaterThanX(const std::string &path, uint32_t x)
{
    uint32_t num {0};
    DIR *pDir;
    struct dirent *ptr;
    if (!(pDir = opendir(path.c_str()))) {
        LogError("Folder doesn't exist");
        return false;
    }
    while ((ptr = readdir(pDir)) != nullptr) {
        if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0) {
            num++;
        }
        if (num > x) {
            closedir(pDir);
            return true;
        }
    }
    closedir(pDir);
    return false;
}

bool MkdirRecusively(std::string const &path, mode_t mode)
{
    // not thread safe. Mkdir same path concurrently may cause problem
    std::vector<std::string> dirs;
    SplitString(GetAbsolutePath(path), '/', dirs);
    if (dirs.empty()) {
        LogError("Mkdir [%s] failed because path is empty.", path.c_str());
        return false;
    }
    if (IsDir(path)) {
        return true;
    }

    std::string current;
    for (auto it = dirs.cbegin(); it != dirs.cend(); ++it) {
        if (it == dirs.cbegin()) {
            current = *it;
        } else {
            current.append(PATH_SEP + *it);
        }
        if (*it == "") {
            continue;
        }
        if (IsDir(current)) {
            continue;
        }
        if (mkdir(current.c_str(), mode) < 0) {
            std::string msg;
            if (IsDir(current) && CheckPermission(current) && CheckOwnerPermission(current, msg)) {
                LogWarn("Mkdir dir %s failed, dir already exist, msg : %s", current.c_str(), msg.c_str());
                continue;
            }
            std::array<char, 256> err_buf{};
            strerror_r(errno, err_buf.data(), err_buf.size());
            LogError("Mkdir [%s] failed, errno: %d, reason: %s", path.c_str(), errno, err_buf.data());
            return false;
        }
    }

    return true;
}

void CopyFolder(const std::string &fromPath, const std::string &toPath, const std::string &outputRootPath,
    const std::function<void(const std::string&, const std::string&)>& func, int depth)
{
    if (depth <= 0) {
        return;
    }
    depth--;
    using namespace std::experimental::filesystem;
    if (!IsDir(fromPath)) {
        LogWarn("Copy failed, src dir not exist [%s]", fromPath.c_str());
        return;
    }
    if (!IsDir(toPath) && !MkdirRecusively(toPath)) {
        LogWarn("Copy failed, target dir not exist and create failed [%s]", toPath.c_str());
        return;
    }
    const std::experimental::filesystem::path sourceDir{fromPath};
    const std::experimental::filesystem::path destDir{toPath};

    for (const auto& dirEntry : directory_iterator(sourceDir)) {
        const auto& sourcePath = dirEntry.path();
        const auto& destPath = destDir / sourcePath.filename();
        if (is_directory(sourcePath)) {
            CopyFolder(sourcePath, destPath, outputRootPath, func, depth);
            continue;
        }
        try {
            if (!copy_file(sourcePath, destPath, copy_options::overwrite_existing)) {
                LogWarn("Copy failed [%s]", sourcePath.c_str());
                continue;
            }
        } catch (const std::experimental::filesystem::filesystem_error& e) {
            LogWarn("Copy failed [%s]", sourcePath.c_str());
            continue;
        }

        if (func == nullptr) {
            continue;
        }
        func(destPath, outputRootPath);
    }
}

bool GetFileNames(const std::string& path, std::vector<std::string> &fileNames, int maxFileCount)
{
    DIR *pDir;
    struct dirent *ptr;
    if (!(pDir = opendir(path.c_str()))) {
        return false;
    }
    while ((ptr = readdir(pDir)) != nullptr) {
        if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0) {
            fileNames.emplace_back(ptr->d_name);
        }
        if (maxFileCount <= 0) {
            closedir(pDir);
            fileNames = {};
            return false;
        }
        maxFileCount--;
    }
    closedir(pDir);
    if (fileNames.empty()) {
        return false;
    }
    sort(fileNames.begin(), fileNames.end());
    return true;
}

bool GetFileLines(std::string path, std::vector<std::string> &fileLines)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        LogError("Failed to open file %s", path.c_str());
        return false;
    } else {
        std::string line;
        size_t lineCount = 0;
        while (std::getline(file, line)) {
            if (lineCount >= MAX_NUM_FOR_GET_LINE) {
                break;
            }
            fileLines.push_back(Strip(line.substr(0, line.size()), "\n\r\t "));
            lineCount++;
        }
        file.close();
    }
    return true;
}

/// return absolute path without ./ or ../, especially used for dir which isn't exist currently
std::string GetAbsolutePath(std::string const &path)
{
    std::string absPath = path;
    std::string currentPath;
    if (!path.empty() && path[0] == USER_HOME) {
        auto homeDir = std::getenv("HOME");
        absPath = homeDir + absPath.substr(1);
    }
    if (absPath.rfind(PATH_SEP, 0) == std::string::npos) {
        if (!GetCurrentWorkingDir(currentPath)) {
            return "";
        }
        absPath = JoinPath({currentPath, absPath});
    }
    std::vector<std::string> dirs;
    std::string::size_type slow = absPath.find_first_not_of(PATH_SEP);
    std::string::size_type fast = absPath.find_first_of(PATH_SEP, slow);
    while (fast != slow) {
        std::string cur = absPath.substr(slow, fast - slow);
        if (cur == "..") {
            if (!dirs.empty()) {
                dirs.resize(dirs.size() - 1);
            }
        } else if (cur != ".") {
            dirs.emplace_back(std::move(cur));
        }
        slow = absPath.find_first_not_of(PATH_SEP, fast);
        fast = absPath.find_first_of(PATH_SEP, slow);
    }

    return "/" + JoinPath(dirs);
}

/// fileMode: bitmap,
///                   S_IRUSR--set: need read permission, unset: don't confirm read permission
///                   S_IWUSR--set: need write permission, unset: don't confirm write permission
///                   S_IXUSR--set: need execute permission, unset: don't confirm execute permission
bool CheckPathPermission(const std::string &path, unsigned int fileMode, const std::string &paramName)
{
    if ((fileMode & S_IRUSR) != 0 && !IsReadable(path)) {
        LogError("%s path: %s is not readable", paramName.c_str(), path.c_str());
        return false;
    }
    if ((fileMode & S_IWUSR) != 0 && !IsWritable(path)) {
        LogError("%s path: %s is not writable", paramName.c_str(), path.c_str());
        return false;
    }
    if ((fileMode & S_IXUSR) != 0 && !IsExecutable(path)) {
        LogError("%s path: %s is not executable", paramName.c_str(), path.c_str());
        return false;
    }
    return true;
}

bool IsSoftLink(const std::string &path)
{
    std::string nonConstPath = GetAbsolutePath(path);
    // If paths begin with "/" and consist of "." ".."and "/" only, they are converted to "/" or "" by GetAbsolutePath.
    // If paths have several '..', they will be converted to '' '/'or other normal paths.
    // Root dir and empty string dir are not treated as a SoftLink in general.
    if (nonConstPath == "/" || nonConstPath == "") {
        return false;
    }
    while (!nonConstPath.empty() && nonConstPath.back() == '/') {
        nonConstPath.pop_back();
    }
    struct stat buf;
    if (memset_s(&buf, sizeof(buf), 0, sizeof(buf)) != EOK) {
        return false;
    }
    return (lstat(nonConstPath.c_str(), &buf) == 0) && ((S_IFMT & buf.st_mode) == S_IFLNK);
}

bool CheckInputFileValid(const std::string &path, const std::string &fileType, size_t threshold,
                         std::string paramName, LogLv logLv)
{
    // [key,value] = [fileType, {neededPermission, trueIfCheckOwnerPermission}]
    const std::map<std::string, std::pair<uint32_t, bool>> FileTypePermission = {
        {"json", {S_IRUSR, true}}, {"cpp", {S_IRUSR, false}},
        {"bin", {S_IRUSR, true}}, {"kernel", {S_IRUSR, true}},
        {"dump", {S_IRUSR, true}}, {"dir", {S_IRUSR, true}},
        {"so", {S_IRUSR, false}},
    };
    if (!paramName.empty()) { paramName = " " + paramName; }

    std::string absPath = GetAbsolutePath(path);
    std::string errorMsg;
    if (!IsStringCharValid(absPath, errorMsg)) {
        LogError("Input parameter%s path contains %s, which is invalid", paramName.c_str(), errorMsg.c_str());
        return false;
    }
    if (IsSoftLinkRecursively(absPath)) {
        LogWarn("Input parameter%s path contains softlink, may cause security problems", paramName.c_str());
    }
    if (!PathLenCheckValid(absPath)) {
        LogError("Input parameter%s path length is too long.", paramName.c_str());
        return false;
    }
    if (!IsExist(absPath)) {
        Log::GetLog().Printf("Input parameter%s path does not exist", logLv, paramName.c_str());
        return false;
    }

    bool isDir = (fileType == "dir");
    if (!isDir && IsDir(absPath)) {
        LogError("Input parameter%s path: %s is not a file", paramName.c_str(), absPath.c_str());
        return false;
    }
    if (FileTypePermission.count(fileType) == 0) {
        LogError("File type not in check map");
        return false;
    }
    uint32_t fileMode = FileTypePermission.at(fileType).first;
    if (!CheckPathPermission(absPath, fileMode, paramName)) {
        return false;
    }

    if (FileTypePermission.at(fileType).second && !CheckOwnerPermission(absPath, errorMsg)) {
        LogError("%s", errorMsg.c_str());
        return false;
    }

    if (!isDir && (fileMode == S_IRUSR) && !CheckFileSizeValid(absPath, threshold)) {
        LogError("Input parameter%s file size is too large, max file size: %zu", paramName.c_str(), threshold);
        return false;
    }
    return true;
}

std::string CheckAndReadFile(const std::string &path, const std::string &fileType, size_t threshold,
                             std::string paramName, LogLv logLv)
{
    char buf[PATH_MAX + 1];
    if (realpath(path.c_str(), buf) == nullptr) {
        return "";
    }
    const std::string realPath = buf;
    if (!CheckInputFileValid(realPath, fileType, threshold, paramName, logLv)) {return "";}
    size_t fileSize = GetFileSize(realPath);
    std::vector<char> rawFile(fileSize);
    if (!ReadBinaryFile(realPath, rawFile)) {
        return "";
    }
    return std::string(rawFile.begin(), rawFile.end());
}

bool CopyFile(const std::string &srcPath, const std::string &destPath)
{
    try {
        std::experimental::filesystem::copy(srcPath, destPath);
    }
    catch (const std::experimental::filesystem::filesystem_error& e) {
        LogError("Failed to copy file, error reason is %s", e.what());
        return false;
    }
    return true;
}
bool GetFileSuffix(const std::string &file, std::string &suffix)
{
    auto sepPos = file.find_last_of('.');
    if (sepPos == std::string::npos) {
        LogWarn("Get file suffix failed, file is %s", file.c_str());
        return false;
    }
    suffix = file.substr(sepPos + 1);
    return true;
}

bool Mkdir(std::string const &path, mode_t mode, bool ignoreExist)
{
    // not thread safe. Mkdir same path concurrently may cause problem
    if (IsDir(path)) {
        if (ignoreExist) {
            return true;
        }
        LogWarn("Mkdir [%s] failed because the folder already exists.", path.c_str());
        return false;
    }
    if (mkdir(path.c_str(), mode) < 0) {
        std::array<char, 256> err_buf{};
        strerror_r(errno, err_buf.data(), err_buf.size());
        LogError("Mkdir [%s] failed, errno: %d, reason: %s", path.c_str(), errno, err_buf.data());
        return false;
    }
    return true;
}

bool IsSoftLinkRecursively(const std::string &path)
{
    std::string nonConstPath = path;
    while (!nonConstPath.empty() && nonConstPath.back() == '/') {
        nonConstPath.pop_back();
    }
    std::vector<std::string> dirs;
    SplitString(nonConstPath, '/', dirs);
    if (dirs.empty()) {
        return false;
    }
    std::string current;
    for (auto it = dirs.cbegin(); it != dirs.cend(); ++it) {
        if (it == dirs.cbegin()) {
            current = *it;
        } else {
            current.append(PATH_SEP + *it);
        }
        if (*it == "." || *it == ".." || *it == "") {
            continue;
        }
        if (IsSoftLink(current)) {
            return true;
        }
    }
    return false;
}

/* fileChecker: 基于业务决定子文件、子目录的校验规则
 *   参数含义:
 *     1. const std::string&:        原始文件夹路径
 *     2. std::vector<std::string>&: 参数1目录下的所有文件、文件夹名称
 *     3. std::string&:              校验不通过时，返回的错误信息
 *   return: 校验通过为true，否则false
 */
bool CheckFolder(const std::string &path, std::string &errorMsg, bool ignoreEmpty,
                 const std::function<bool(const std::string&, std::vector<std::string>&, std::string&)> &fileChecker)
{
    if (path.empty()) {
        return ignoreEmpty;
    }
    if (!Utility::CheckInputFileValid(path, "dir")) {
        errorMsg = "dir is invalid";
        return false;
    }

    std::vector<std::string> fileNames;
    if (!GetFileNames(path, fileNames)) {
        errorMsg = "dir is invalid, maybe dir is not exist or empty or contains too many files";
        return false;
    }
    if (fileChecker) {
        return fileChecker(path, fileNames, errorMsg);
    }
    return true;
}

bool CheckPermission(const std::string& filePath)
{
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) != 0) {
        LogError("Error getting file %s status", filePath.c_str());
        return false;
    }

    if (IsRootUser()) {
        return true;
    }

    // 读取权限位
    mode_t permissions = fileStat.st_mode & (S_IWGRP | S_IWOTH);
    // 检查除了当前用户外其他人是否有写入权限
    if (permissions != 0) {
        LogWarn("The current file %s is not recommended to be writable by group or other users",
            filePath.c_str());
    }
    return true;
}

uint64_t GetSystemAvailableMemory()
{
    uint64_t freeMemory = INPUT_FILE_LIMIT_SIZE;
    const std::string filePath = "/proc/meminfo";
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LogDebug("Failed to get system free memory.Default file size 1G will use as limit.");
        return freeMemory;
    }
    // 可用内存
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("MemFree:") != std::string::npos) {
            std::string memoryString = line.substr(line.find(':') + 1);
            if (StoullConverter(memoryString, freeMemory, RADIX_10, false)) { // 转换为10进制
                freeMemory = freeMemory * 1024; // 读取数据单位KB，乘1024转换为B
            }
        }
    }
    file.close();
    freeMemory = static_cast<uint64_t>(freeMemory * MAX_MEMORY_USAGE_RATIO); // 当前0.5的可用内存作为文件大小阈值
    return freeMemory;
}

std::string FindExecutableCommand(const std::string &command)
{
    std::vector<std::string> paths(1);
    if (!GetCurrentWorkingDir(paths.back())) {
        LogDebug("Will not check command %s existence in working dir.", command.c_str());
    }

    const char *env = getenv("PATH");
    if (env && !std::string(env).empty()) {
        SplitString(std::string(env), ':', paths);
    }

    for (auto &dir : paths) {
        if (dir.empty()) {
            continue;
        }
        dir.append("/" + command);
        if (!IsDir(dir) && IsExecutable(dir)) {
            LogDebug("Find executable command %s", dir.c_str());
            return dir;
        }
    }
    return "";
}

bool PathLenCheckValid(const std::string &checkPath)
{
    if (checkPath.length() > DIR_NAME_LENGTH_LIMIT) {
        return false;
    }
    std::vector<std::string> dirs;
    SplitString(checkPath, '/', dirs);
    for (const auto &it : dirs) {
        if (it.length() > FILE_NAME_LENGTH_LIMIT) {
            return false;
        }
    }
    return true;
}

bool CheckOwnerPermission(std::string &path, std::string &msg)
{
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) != 0) {
        msg = "get file" + path + "permission error.";
        return false;
    }
    if (IsRootUser()) {
        return true;
    }
    if ((fileStat.st_mode & (S_IWOTH | S_IWGRP)) != 0) {
        LogWarn("%s is not recommended to be writable by group or other users", path.c_str());
    }
    if (fileStat.st_uid == 0 || fileStat.st_uid == static_cast<uint32_t>(getuid())) {
        return true;
    }
    LogWarn("%s is not owned by the current user, which may cause security problems", path.c_str());
    return true;
}

bool RollbackPath(std::string &path, uint32_t rollNum)
{
    std::string tmpPath = path;
    while (rollNum > 0) {
        rollNum--;
        std::size_t found = tmpPath.find_last_of('/');
        if (found == std::string::npos) {
            return false;
        }
        tmpPath = tmpPath.substr(0, found);
        size_t len = tmpPath.size();
        while (len <= tmpPath.size() && len > 0 && tmpPath[len - 1] == '/') {
            len--;
        }
        if (len <= tmpPath.size() && len > 0) {
            tmpPath = tmpPath.substr(0, len);
        }
    }
    path = tmpPath.empty() ? "/" : tmpPath;
    return true;
}

std::time_t GenerateTimeStamp(std::string& str, TimeAccuracy acc)
{
    char buf[32] = "0";
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    struct tm temp;
    std::tm *tm = localtime_r(&time, &temp);
    if (tm != nullptr) {
        std::strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", tm);
    } else {
        LogWarn("Failed to generate time stamp");
    }
    str = buf;
    if (acc == TimeAccuracy::MILLISECOND) {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // 保证毫秒是3位数
        std::stringstream ss;
        ss << std::setw(3) <<std::setfill('0') << (ms % 1000); // 1000ms equal 1s, 3 is the number of characters written
        str += ss.str();
    }
    return time;
}

void ReadFileByMMap(const std::string &fileName, std::vector<std::string> &fileLine)
{
    char buf[PATH_MAX];
    if (realpath(fileName.c_str(), buf) == nullptr) {
        LogDebug("Failed to get file %s", fileName.c_str());
        return;
    }
    int fd = open(buf, O_RDONLY);
    if (fd < 0) {
        LogDebug("Failed to open file %s", fileName.c_str());
        return;
    }
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        LogDebug("Failed to get file %s size", fileName.c_str());
        close(fd);
        return;
    }
    auto fileSize = sb.st_size;
    char *mmpedData = static_cast<char*>(mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd, 0));
    if (mmpedData == MAP_FAILED) {
        close(fd);
        LogDebug("Failed to mmap file %s", fileName.c_str());
        return;
    }
    uintmax_t count = 0;
    char* start = mmpedData;
    for (char* p = start; count < static_cast<uintmax_t>(fileSize); p++, count++) {
        if (*p == '\n') {
            fileLine.emplace_back(start, p - start);
            start = p + 1;
        }
    }
    if (start < mmpedData + fileSize) {
        fileLine.emplace_back(start, mmpedData + fileSize);
    }
    close(fd);
    if (munmap(mmpedData, fileSize) == -1) {
        LogDebug("Failed to unmapped file %s", fileName.c_str());
    }
}

bool ReadBinFileByMultiStruct(const std::string &filePath, const size_t &fileSize, const size_t &structSize,
                              std::vector<char> &binData)
{
    if ((fileSize < structSize) || (fileSize % structSize != 0)) {
        LogDebug("File %s is not complete, please check the file.", filePath.c_str());
        return false;
    }
    if (!IsReadable(Realpath(filePath))) {
        LogDebug("File %s is not exists or not readable.", filePath.c_str());
        return false;
    }
    binData.resize(fileSize);
    if (!ReadBinaryFile(filePath, binData)) {
        return false;
    }
    return true;
}

bool AppendLinesToFile(const std::string& fileName, const std::vector<std::string> &lines)
{
    std::string filePath = Realpath(fileName);
    if (!Utility::IsWritable(filePath)) {
        LogWarn("File [%s] is not exist or not writeable.", fileName.c_str());
        return false;
    }
    std::ofstream outFile(filePath.c_str(), std::ios::app | std::ios::binary);
    if (!outFile.is_open()) {
        LogWarn("Failed to open file %s", fileName.c_str());
        return false;
    }
    for (const auto &l: lines) {
        outFile << l << "\n";
    }
    outFile.close();
    return true;
}

void SearchDirRecursive(const std::string &searchPath, const std::string &targetDirName, std::vector<std::string> &targetPath, int deep)
{
    if (deep > 6) {
        return;
    }

    DIR* dir = opendir(searchPath.c_str());
    if (!dir) {
        return;
    }
    struct dirent* entry;
    while((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        std::string fullPath = searchPath + PATH_SEP + name;
        if (!IsDir(fullPath)) {
            continue;
        }
        if (name == targetDirName) {
            targetPath.push_back(fullPath);
        }
        SearchDirRecursive(fullPath, targetDirName, targetPath, deep + 1);
    }
    closedir(dir);
}
}
