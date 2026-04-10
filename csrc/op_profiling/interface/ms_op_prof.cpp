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


#include "ms_op_prof.h"

#include <thread>
#include <csignal>
#include <atomic>
#include <memory>
#include "common/defs.h"
#include "common/prof_args.h"
#include "common/hal_helper.h"
#include "argparser/arg_checker.h"
#include "argparser/arg_normalize.h"
#include "argparser/utils.h"
#include "argparser/parser.h"
#include "filesystem.h"
#include "log.h"
#include "json_parser.h"
#include "smart_pointer.h"
#include "profiling/op_prof.h"
#include "profiling/device/op_device_prof.h"
#include "profiling/simulator/op_sim_prof.h"
#include "profiling/op_prof_task.h"
#include "profiling/op_prof_data_parse.h"
#include "ascend_helper.h"

using namespace Common;
using namespace Parser;
using namespace Utility;

namespace Interface {

std::atomic<bool> g_isFirstReceiveSignal {true};
void SignalHandler(int signo)
{
    (void)signo;
    if (g_isFirstReceiveSignal || Interface::IsProcessRunning()) {
        g_isFirstReceiveSignal = false;
        Interface::SetExitMode();
        return;
    }
    signal(SIGINT, SIG_DFL);
}

ArgParser BuildDeviceArgParser(ProfArgs &args)
{
    ArgParser argParser("msopprof", "operator profiling tool");
    argParser.Add(Switch('h', "help", args.printHelp));
    argParser.Add(Option<std::string>('\0', "config", "ConfigPath", args.argConfig));
    argParser.Add(Option<std::string>('\0', "application", "CMD", args.argApplication));
    argParser.Add(Option<std::string>('\0', "output", "STRING", args.argOutput));
    argParser.Add(Option<ProfMetricsAbilityConfig>('\0', "aic-metrics", "MetricsLists", args.argAicMetrics));
    argParser.Add(Option<std::string>('\0', "kernel-name", "STRING", args.argKernelName));
    argParser.Add(Option<std::string>('\0', "launch-count", "STRING", args.argLaunchCount));
    argParser.Add(Option<std::string>('\0', "launch-skip-before-match", "STRING", args.argLaunchSkipBeforeMatch));
    argParser.Add(Option<std::string>('\0', "replay-mode", "STRING", args.argReplayMode));
    argParser.Add(Option<std::string>('\0', "kill", "STRING", args.argKill));
    argParser.Add(Option<std::string>('\0', "mstx", "STRING", args.argMstx));
    argParser.Add(Option<std::string>('\0', "mstx-include", "STRING", args.argMstxInclude));
    argParser.Add(Option<std::string>('\0', "warm-up", "STRING", args.argWarmUp));
    argParser.Add(Option<std::string>('\0', "dump", "STRING", args.argDump));
    argParser.Add(Option<std::string>('\0', "core-id", "STRING", args.argCoreId));
    return argParser;
}

ArgParser BuildSimulatorArgParser(ProfArgs &args)
{
    ArgParser argParser("msopprof", "operator profiling tool");
    argParser.Add(Switch('h', "help", args.printHelp));
    argParser.Add(Option<std::string>('\0', "config", "ConfigPath", args.argConfig));
    argParser.Add(Option<std::string>('\0', "application", "CMD", args.argApplication));
    argParser.Add(Option<std::string>('\0', "export", "STRING", args.argExport));
    argParser.Add(Option<std::string>('\0', "output", "STRING", args.argOutput));
    argParser.Add(Option<std::string>('\0', "kernel-name", "STRING", args.argKernelName));
    argParser.Add(Option<std::string>('\0', "launch-count", "STRING", args.argLaunchCount));
    argParser.Add(Option<ProfMetricsAbilityConfig>('\0', "aic-metrics", "MetricsLists", args.argAicMetrics));
    argParser.Add(Option<std::string>('\0', "mstx", "STRING", args.argMstx));
    argParser.Add(Option<std::string>('\0', "mstx-include", "STRING", args.argMstxInclude));
    argParser.Add(Option<std::string>('\0', "soc-version", "STRING", args.argSocVersion));
    argParser.Add(Option<std::string>('\0', "core-id", "STRING", args.argCoreId));
    argParser.Add(Option<std::string>('\0', "timeout", "STRING", args.argTimeout));
    argParser.Add(Option<std::string>('\0', "dump", "STRING", args.argDump));
    return argParser;
}

void PrintDeviceHelp(ChipType chipType)
{
    std::cout
        << "msopprof (MindStudio Profiler For Operator) is part of MindStudio Operator-dev Tools." << std::endl
        << "Used for Ascend C operator profiling by running on the board." << "\n" << std::endl
        << "Options:" << std::endl
        << "   --help / -h                          <Optional> Help message." << std::endl
        << "   --config                             <Optional> Json file for op config path" << std::endl
        << "   --application                        <Optional> Executable file path" << std::endl
        << "   --output                             <Optional> Output path" << std::endl
        << "   --aic-metrics=ability1,ability2,...  <Optional> Enable collection capability of ArithmeticUtilization "
           "/ MemoryUB / Memory / MemoryL0 / L2Cache / PipeUtilization" << std::endl
        << "                                                   / ResourceConflictRatio / BasicInfo / Roofline ";
    if (chipType == Common::ChipType::ASCEND910B) {
        std::cout << "/ Occupancy / TimelineDetail / KernelScale / Source / MemoryDetail ";
    }
    if (chipType == Common::ChipType::ASCEND950) {
        std::cout << "/ Occupancy / KernelScale / PipeTimeline / Source / PCSampling ";
    }
    std::cout
        << "/ Default. " << std::endl
        << "   --kernel-name                        <Optional> Specify the kernel name to profile."
        << " It's not effective in config mode." << std::endl
        << "   --launch-count                       <Optional> Number of kernel that can be collected,"
        << " number in [1, 5000]." << std::endl
        << "   --launch-skip-before-match           <Optional> Set the number of kernel launch that you want to"
        << " skip before " << std::endl
        << "                                                   starting to analyze the kernel,"
        << " number in [0, 1000]. " << std::endl
        << "   --replay-mode                        <Optional> Data collection replay Mode, select application / kernel";
    if (chipType == Common::ChipType::ASCEND910B || chipType == Common::ChipType::ASCEND950) {
        std::cout << " / range";
    }
    std::cout << ", the default value is kernel." << std::endl
        << "   --kill                               <Optional> Kill op process when the number of kernel reaches"
           " launch-count," << std::endl
        << "                                                   select on / off, the default value is off." << std::endl
        << "   --mstx                               <Optional> Enable mstx api or not, select on / off,"
        << " the default value is off." << std::endl
        << "   --mstx-include                       <Optional> Specify the mstx range for msprof op to be collected."
        << std::endl
        << "   --warm-up                            <Optional> Set the number of warm up times."
        << " The default value is 5, range in [0, 500]." << std::endl;
    if (chipType == Common::ChipType::ASCEND910B) {
        std::cout << "   --dump                               <Optional> Enable or disable dump flushed to disk mode, "
           "only effective when --aic-metrics=TimelineDetail" << std::endl;
        std::cout << "   --core-id                            <Optional> Specify the id of cores to be parsed, "
                     "only effective when --aic-metrics=TimelineDetail and only effective in simulation products."
                     << std::endl;
    }
}

void PrintSimulatorHelp(void)
{
    std::cout
        << "msopprof (MindStudio Profiler For Operator) is part of MindStudio Operator-dev Tools." << std::endl
        << "Used for Ascend C operator profiling by running on the simulator." << "\n" << std::endl
        << "Options:" << std::endl
        << "   --help / -h                        <Optional> Help message." << std::endl
        << "   --config                           <Optional> Json file for op config path" << std::endl
        << "   --application                      <Optional> Executable file path" << std::endl
        << "   --export                           <Optional> Specify the dump data path for parsing" << std::endl
        << "   --output                           <Optional> Output path" << std::endl
        << "   --kernel-name                      <Optional> Specify the kernel name to profile."
        << " It's not effective in config mode." << std::endl
        << "   --aic-metrics=metric1,metric2,...  <Optional> Enable PipeUtilization / ResourceConflictRatio / "
            "PMSampling metrics. PipeUtilization is required" << std::endl
        << "   --launch-count                     <Optional> Number of kernel that can be collected,"
        << " number in [1, 5000]." << std::endl
        << "   --mstx                             <Optional> Enable mstx api or not, select on / off,"
        << " the default value is off." << std::endl
        << "   --mstx-include                     <Optional> Specify the mstx range for msprof op to be collected."
        << std::endl
        << "   --soc-version                      <Optional> Specify a simulator in ascend toolkit."
        << " It's not effective in config mode." << std::endl
        << "   --core-id                          <Optional> Specify the id of cores to be parsed." << std::endl
        << "   --timeout                          <Optional> Set the timeout minutes for application runs,"
           " range in [1, 2880]."
        << std::endl
        << "   --dump                             <Optional> Enable or disable dump flushed to disk mode, "
           "This parameter is valid only for A2/A3." << std::endl;
}

void PrintErrorMsg(std::string const &msg)
{
    LogError("%s", msg.c_str());
    LogInfo("Use msprof op --help or msprof op simulator --help for more details");
}

bool ProfArgsNormalize(Common::ProfArgs &args, std::string &msg)
{
    ArgNormalize normalizer;
    if (!normalizer.Normalize(args, msg)) {
        return false;
    }
    return true;
}

bool ProfArgsChecker(const Common::ProfArgs &args, std::string &msg)
{
    ArgChecker checker(args.runMode);
    if (!checker.Check(args, msg)) {
        return false;
    }
    return true;
}

bool Parse(ArgParser &parser, int argc, char** argv, std::string &msg, Common::ProfArgs &args)
{
    Either ret = parser.Parse(TokenS{argc, argv}, args);
    if (ret.Valid()) {
        return true;
    }
    Error error = ret.Left();
    msg = error.msg;
    return false;
}

bool ProfArgsFileParser(Common::ProfArgs &args)
{
    std::string mode = args.runMode == "device" ? "onboard" : "ca";
    std::string configType;
    if (!GetFileSuffix(args.argConfig, configType)) {
        return false;
    }

    std::vector<CaseConfig> caseConfigs = ParseRunConfigJson(args.argConfig, "msopprof", mode);
    if (caseConfigs.empty()) {
        return false;
    }
    if (caseConfigs.size() != 1) {
        LogError("Only support one case for kernel.");
        return false;
    }
    args.kernelConfig = caseConfigs[0].kernelConfig;
    return true;
}

bool ProfArgsParse(int argc, char *argv[], ProfArgs &args, std::string &msg)
{
    constexpr int skipArgNum = 2;
    bool parseRet;

    if (argc >= skipArgNum && std::string(argv[1]) == "simulator") {
        ArgParser argParser = BuildSimulatorArgParser(args);
        parseRet = Parse(argParser, argc - skipArgNum, argv + skipArgNum, msg, args);
        args.runMode = "simulator";
    } else {
        ArgParser argParser = BuildDeviceArgParser(args);
        parseRet = Parse(argParser, argc - 1, argv + 1, msg, args);
    }

    return parseRet;
}

bool ProfArgsInit(Common::ProfArgs &args, int argc, char *argv[], char *env[])
{
    (void)env;
    std::string msg;
    if (!ProfArgsParse(argc, argv, args, msg)) {
        PrintErrorMsg(msg);
        return false;
    }

    if (args.printHelp) {
        return true;
    }

    if (!ProfArgsNormalize(args, msg) || !ProfArgsChecker(args, msg)) {
        PrintErrorMsg(msg);
        return false;
    }
    auto it = ReplayModeMap.find(args.argReplayMode);
    if (it != ReplayModeMap.end()) {
        args.argAicMetrics.replayMode = it->second;
    }
    args.argOutput = Utility::RandomizeDir(args.argOutput + Path::MSOPPROF_DIR_PREFIX);
    if (!args.argConfig.empty() && !ProfArgsFileParser(args)) {
        return false;
    }
    return true;
}

bool ProfilingRun(const Common::ProfArgs &args)
{
    signal(SIGINT, SignalHandler);
    if (args.argAicMetrics.isDeviceToSimulator) {
        std::unique_ptr<Profiling::OpProf> deviceProfiling = MakeUnique<Profiling::OpDeviceProf>(args);
        if (deviceProfiling && deviceProfiling->RunTask()) {
            std::unique_ptr<Profiling::OpProf> simProfiling = MakeUnique<Profiling::OpSimProf>(args);
            if (!simProfiling) {
                LogError("Simulator profiling failed because of nullptr.");
                return false;
            }
            simProfiling->dump_ = deviceProfiling->dump_;
            bool simParseRes = simProfiling->RunDataParse(false);
            bool deviceParseRes = deviceProfiling->RunDataParse();
            if (!simParseRes) {
                LogWarn("TimelineDetail data collection failed");
            }
            return (deviceParseRes && simParseRes);
        }
        return false;
    }
    std::unique_ptr<Profiling::OpProf> runProfiling;
    if (args.runMode == "device") {
        runProfiling = MakeUnique<Profiling::OpDeviceProf>(args);
    } else {
        runProfiling = MakeUnique<Profiling::OpSimProf>(args);
    }
    if (!runProfiling) {
        LogError("Profiling failed because of nullptr");
        return false;
    }
    return runProfiling->Run();
}

bool IsProcessRunning()
{
    return (Profiling::Task::GetExecutionStatus() == Profiling::ExecStatus::RUNNING);
}

void SetExitMode()
{
    Profiling::DataParse::inExitMode = true;
    Profiling::Task::inExitMode = true;
}
} // namespace Interface
