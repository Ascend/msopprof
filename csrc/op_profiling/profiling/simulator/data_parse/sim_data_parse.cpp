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


#include "sim_data_parse.h"

#include <vector>
#include <experimental/filesystem>
#include <thread>
#include <map>
#include "common/prof_args.h"
#include "common/defs.h"
#include "filesystem.h"
#include "log.h"
#include "ascend_helper.h"
#include "common/runtime_helper.h"
#include "sim_defs.h"
#include "thread_pool.h"
#include "common/runtime_helper.h"

using namespace Common;
using namespace Utility;
using namespace std::experimental::filesystem;

namespace Profiling {
using namespace Parse;
const std::string DUMP_DIR = "dump";
const std::string TMP_DIR = "tmp_dump";

float GetMicrosecond(ChipProductType chipType, int64_t cycles, int roundParam)
{
    if (cycles < 0) {
        LogDebug("There are some value wrong when calculate time");
        return 0;
    }
    double clockSpeed;
    auto find_soc = Profiling::CLOCK_SPEED_SERIES_MAP.find(chipType);
    clockSpeed = (find_soc == Profiling::CLOCK_SPEED_SERIES_MAP.end()) ? Profiling::DEFAULT_MHZ : find_soc->second;
    if (roundParam == -1) {
        return static_cast<float>(cycles / clockSpeed);
    }
    return static_cast<float>(round(cycles / clockSpeed *
                                    pow(10, roundParam)) / pow(10, roundParam)); // 10:used to calculate round
}

void SimDataParse::ParseKernelFile(const std::string &kernelDir, const std::string &kernelName,
                                   const std::string &deviceId)
{
    std::vector<std::string> fileNames;
    if (!GetFileNames(kernelDir, fileNames)) {
        LogWarn("Parsing %s failed, which maybe empty or too many kernels.", kernelDir.c_str());
        return ;
    }
    sort(fileNames.begin(), fileNames.end());

    std::regex orderPattern("[0-9]{1,3}");
    for (const auto &fileName : fileNames) {
        std::string orderDir = JoinPath({kernelDir, fileName});
        if (!IsDir(orderDir)) {
            continue;
        }
        if (dump_) {
            if (!regex_match(fileName, orderPattern)) {
                continue;
            }
            // orderDir is OPPO/device_0/argKernelName/0
            if (!ParseKernelData(orderDir)) {
                failedKernelNum_++;
                LogError("Parse kernel data failed, kernel name is %s, the order is %s",
                         kernelName.c_str(), fileName.c_str());
            }
        }
        if (!theOnlyKernelPath_.empty()) {
            isTheOnlyKernel_ = false;
        }
        if (isTheOnlyKernel_) {
            theOnlyKernelPath_ = orderDir;
        }
        totalKernelNum_++;
    }
}

// After the parsing is complete, the muliti kernels directories are generated as followed
// |---OPPROF_XXXX
// |---OpName1 //OpName is kernel name
// | |---0
// | | |---dump
// | | |---simulator
// | |---1
// | | |---dump
// | | |---simulator
// | |---dump
// | | |---aicore_binary.o
// |---OpName2
// | |---0
// | | |---dump
// | | |---simulator
// | |---dump
// | | |---aicore_binary.o
// After the parsing is complete, the single kernel directories are generated as followed
// |---OPPROF_XXXX
// | |---dump
// | ||---aicore_binary.o
// | |---simulator
bool SimDataParse::Execute(std::string dataPath)
{
    LogInfo("Start parse dump file");
    std::string ascendHomePath;
    if (!GetAscendHomePath(ascendHomePath)) {
        LogWarn("Ascend path not found");
        return false;
    }
    if (socVersion_.empty()) {
        socVersion_ = Common::RuntimeHelper::Instance().GetSocVersion();
    }
    if (!exportPath_.empty()) {
        return ParseExportDumpFile(dataPath);
    }
    if (!ParserDeviceIdDir(dataPath)) {
        return false;
    }
    return ExecuteSummary(dataPath);
}

bool SimDataParse::ParseKernelData(const std::string &path)
{
    std::string dumpPath = JoinPath({path, DUMP_DIR});
    std::string simulatorPath = JoinPath({path, "simulator"});
    std::string relocObjectPath = JoinPath({dumpPath, AICORE_KERNEL_NAME});
    if (!IsExist(relocObjectPath)) {
        LogError("Can not get aicore kernel, path is %s", relocObjectPath.c_str());
        return false;
    }
    if (!ParseMergeDumpData(dumpPath, relocObjectPath, simulatorPath)) {
        return false;
    }

    return true;
}

bool SimDataParse::ParseExportDumpFile(const std::string &dataPath)
{
    std::string relocObjectPath = JoinPath({exportPath_, AICORE_KERNEL_NAME});
    std::string simulatorPath = JoinPath({dataPath, "simulator"});
    if (!ParseMergeDumpData(exportPath_, relocObjectPath, simulatorPath)) {
        return false;
    }
    std::string dumpOutputPath = JoinPath({dataPath, "dump"});
    remove_all(dumpOutputPath);
    LogInfo("Profiling results saved in %s", dataPath.c_str());
    return true;
}


void SimDataParse::DisposeTmp(const std::string &dumpPath)
{
    std::string tmpDumpPath = JoinPath({dumpPath, TMP_DIR});
    if (inExitMode && dump_) {
        HandleDumpLog(tmpDumpPath);
    }
    remove_all(tmpDumpPath);
}

bool SimDataParse::GetObjectOutPathAndCopyAicoreFile(const std::string &path, std::string &outPath) const
{
    std::vector<std::string> fileLines;
    std::string kernelNamePath = JoinPath({path, "object_dump.txt"});
    if (!IsReadable(kernelNamePath)) {
        LogWarn("Can not get object kernel dump path");
        return false;
    }
    // 2 lines information, including the output path and kernel name
    if (GetFileLines(kernelNamePath, fileLines) && fileLines.size() == 2) {
        outPath = fileLines[0];
        if (outPath.empty()) {
            LogWarn("Object kernel dump path is not valid");
            return false;
        }
        if (!IsExist(outPath) && !MkdirRecusively(outPath)) {
            return false;
        }
        LogDebug("In exit mode, get output path success, path is %s", outPath.c_str());
        // copy aicore.bin.0
        // fileLines[1] is kernel name
        std::string aicoreBinFileSrc = JoinPath({path, fileLines[1], DUMP_DIR});
        std::string aicoreBinFileDest = JoinPath({outPath});
        if (IsExist(aicoreBinFileSrc) && IsExist(aicoreBinFileDest)) {
            if (!CopyFile(aicoreBinFileSrc, aicoreBinFileDest)) {
                LogWarn("CopyFile aicoreBinFileSrc to aicoreBinFileDest failure");
                return false;
            }
            LogDebug("Data parse in ExitMode successfully copy aicore.bin");
        }
        return true;
    }
    LogWarn("No target file is stored in temp dump.");
    return false;
}

void SimDataParse::HandleDumpLog(const std::string &tmpOutputPath) const
{
    std::string outputPath;
    if (!GetObjectOutPathAndCopyAicoreFile(tmpOutputPath, outputPath)) {
        return;
    }
    for (auto const& dir_entry : directory_iterator(tmpOutputPath)) {
        if (IsDir(dir_entry.path())) {
            continue;
        }
        CopyFile(dir_entry.path().string(), outputPath);
    }
}

// Refactor Function, Will Replace ParseMergeDumpData
bool SimDataParse::ParseMergeDumpData(const std::string &dumpPath, std::string &relocObjectPath,
    const std::string &simulatorPath)
{
    std::string errorMsg;
    auto subFilesChecker = std::bind(&SimDataParse::CheckKernelFiles, this, std::placeholders::_1,
                                     std::placeholders::_2, std::placeholders::_3);
    if (!CheckFolder(dumpPath, errorMsg, false, subFilesChecker)) {
        LogError("In profiling data processing, %s", errorMsg.c_str());
        return false;
    }

    ParseInfoStruct parseInfoStruct;
    parseInfoStruct.path = dumpPath;

    DumpParserArgs dumpParserArgs;
    auto it = Common::SOC_STRING_TO_CHIP_PRODUCT.find(socVersion_);
    dumpParserArgs.chipType = it == Common::SOC_STRING_TO_CHIP_PRODUCT.end() ?
                                Common::ChipProductType::UNKNOWN_PRODUCT_TYPE : it->second;
    dumpParserArgs.parseCoreIds = {};
    parseInfoStruct.chipType = dumpParserArgs.chipType;
    if (!MkdirRecusively(simulatorPath)) {
        LogError("Create profiling path: simulator failed.");
        return false;
    }
    SimParseContext simContext;
    // set thread pool size for one kernel data parse
    uint32_t systemCores = static_cast<uint32_t>(std::thread::hardware_concurrency() * MAX_THREAD_USAGE_RATIO);
    bool res = GetCoresTuple(dumpPath, simContext.coresNamePair);
    if (!res) { return false; }

    simContext.dumpPath = dumpPath;
    simContext.outputPath = simulatorPath;
    simContext.chipType = dumpParserArgs.chipType;
    simContext.parseCorIds = Utility::SplitString<int32_t>(coreId_, '|');
    simContext.metricsConfig = metrics_;

    auto dataCenterPtr = Utility::MakeShared<Profiling::Parse::DataCenter>();
    if (dataCenterPtr == nullptr) {
        LogError("Failed to makeShared of dataCenterPtr");
        return false;
    }

    // ParseAndCalCulate used to get data
    std::shared_ptr<Pc2CodeMap> pc2codePtr = Utility::MakeShared<Pc2CodeMap>();
    if (!ParseDumpFile(simContext, dataCenterPtr, pc2codePtr, systemCores)) {
        LogError("Failed to parse dump file data");
        return false;
    }

    if (pc2codePtr == nullptr) {
        Pc2CodeMap empty;
        LogWarn("Can not get kernel file of %s. code information will be missing", relocObjectPath.c_str());
        Visualizedata(simContext.outputPath, simContext.chipType, empty, dataCenterPtr, systemCores);
    } else  {
        Visualizedata(simContext.outputPath, simContext.chipType, *pc2codePtr, dataCenterPtr, systemCores);
    }
    return true;
}

bool SimDataParse::CheckKernelFiles(const std::string &path, std::vector<std::string> &fileNames,
                                    std::string &errorMsg) const
{
    std::regex dumpPattern = std::regex(Common::CHECK_DUMP_FILE);
    std::regex kernelPattern = std::regex(Common::AICORE_KERNEL_NAME);
    for (const std::string &fileName : fileNames) {
        std ::string dumpFilePath = Utility::JoinPath({path, fileName});
        if (std::regex_match(fileName, dumpPattern) &&
            (!Utility::CheckInputFileValid(dumpFilePath, "dump", INPUT_DUMP_FILE_MAX_SIZE) ||
             !Utility::CheckPermission(dumpFilePath))) {
            errorMsg = "dump file permission check failed, " + dumpFilePath;
            return false;
        } else if (std::regex_match(fileName, kernelPattern) &&
                   (!Utility::CheckInputFileValid(dumpFilePath, "bin", INPUT_BINARY_FILE_MAX_SIZE) ||
                    !Utility::CheckPermission(dumpFilePath))) {
            errorMsg = "kernel file permission check failed, " + dumpFilePath;
            return false;
        }
    }
    return true;
}

SimDataParse::SimDataParse(std::string socVersion, std::string exportPath, std::string coreId,
    Common::ProfMetricsAbilityConfig metrics, bool dump) : DataParse(std::move(metrics)),
    socVersion_(std::move(socVersion)), coreId_(std::move(coreId)), exportPath_(std::move(exportPath)), dump_(dump)
{
    if (socVersion_.empty() && !GetSocVersionFromEnvVar(socVersion_)) {
        socVersion_ = Common::RuntimeHelper::Instance().GetSocVersion();
        if (socVersion_.empty()) {
            LogWarn("Cant get socVersion from LD_LIBRARY_PATH and runtime, set to the default socVersion Ascend910B1");
            socVersion_ = "Ascend910B1";
        }
    }
}
}