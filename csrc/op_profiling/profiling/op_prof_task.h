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


#ifndef __MSOPPROF_PROFILING_PROFILING_TASK_H__
#define __MSOPPROF_PROFILING_PROFILING_TASK_H__

#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include "parse/data_parser/real_time_data_parser.h"
#include "common/prof_args.h"

namespace Profiling {
enum class ExecStatus : uint8_t {
    NOT_RUNNING,
    RUNNING,
    STOPPED
};

class Task {
public:
    explicit Task(std::string taskName) : taskName(std::move(taskName)) {}
    virtual ~Task() = default;
    virtual bool Run() = 0;

    static ExecStatus GetExecutionStatus()
    {
        return execStatus;
    }
    std::string taskName;
    std::vector<std::string> cmd;
    std::map<std::string, std::string> env;
    std::string outputPath;
    std::string opRunMode {Common::OpRunnerMode::EXECUTE_BINARY};
    OpRunner::KernelConfig kernelConfig;
    bool isMstxEnable {false};
    std::string mstxEnabledMessageString;
    static ExecStatus execStatus;
    static std::atomic<bool> inExitMode;
    static std::atomic<bool> killAdvance;
    bool isSetSocVersion{false};
    bool isCaLogTransStartSuc_{false};
    std::string simSocVersion;
    MessageOfProfConfig profMessage_{};
    Common::ProfConfig profConfig_;
    int32_t timeout_ {-1};
    std::string tmpPath_;
    static std::atomic<bool> needRegisterEvent_;
    RealTimeSimParseContext realTimeSimParseContext_;
    std::shared_ptr<Parse::RealTimeDataParser> realTimeDataParser_ {nullptr};

protected:
    const std::string KERNEL_CONFIG_NAME = "kernel_config.bin";
    void CreateCamodelConfig(bool pmSamplingEnable);
    bool CreateTaskDir(const std::string &path) const;
    bool CheckSimulatorSoExist() const;
    bool IsSetSocVersion(std::string &paramSocVersion) const;
    void RegisterRunningEvent();
    bool GenOpConfig(nlohmann::json &jsonData);
private:
    inline std::string PrintableFileName(const std::string &fileName) const
    {
        // 日志打印要求，不能按910_xxxx的格式
        return Utility::ReplaceSubStr(fileName, "910_", "910 ");
    }
    bool ReadConfigFile(const std::string &fileName, const std::map<std::string, std::string> &replaceStrMap,
                        bool &createNewFile, std::vector<std::string> &newLines);
    bool CreateConfigFile(const std::string &fileName, const std::map<std::string, std::string> &replaceStrMap);
    bool CreateConfigFileWithAppend(const std::string &fileName, const std::string &part, const std::string &listName,
                                    const std::string &item);
    bool CheckIfNeedAppend(const std::string &fileName, const std::string &part, const std::string &listName,
                           const std::string &item, std::vector<std::string> &newLines);
    bool WriteNewConfig(const std::string &fileName, const std::vector<std::string> &newLines);
    bool UpdateNewConfig(const std::string &filePath, const std::vector<std::string> &newLines) const;

    std::string camodelConfigDir_;
    std::string camodelLibDir_;
};
}

#endif // __MSOPPROF_PROFILING_PROFILING_TASK_H__
