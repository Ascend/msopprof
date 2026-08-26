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
#include <iostream>
#include <dlfcn.h>
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
#include "cli_logo.h"

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
    argParser.Add(Switch('V', "version", args.printVersion));
    argParser.Add(Switch('v', "", args.deprecatedVersion));
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
    argParser.Add(Option<std::string>('\0', "custom-input", "ConfigPath", args.argCustomInput));
    argParser.Add(Option<std::string>('\0', "instr-timeline-pipe", "STRING", args.argInstrTimelinePipe));
    return argParser;
}

ArgParser BuildSimulatorArgParser(ProfArgs &args)
{
    ArgParser argParser("msopprof", "operator profiling tool");
    argParser.Add(Switch('h', "help", args.printHelp));
    argParser.Add(Switch('V', "version", args.printVersion));
    argParser.Add(Switch('v', "", args.deprecatedVersion));
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

std::string GetFuncInjectionRevision()
{
    std::string revision;

    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len <= 0) {
        return revision;
    }
    buf[len] = 0;
    std::string soPath = std::string(buf);
    if (!Utility::RollbackPath(soPath, 2)) {
        return revision;
    }
    soPath = Utility::JoinPath({soPath, "lib64/libmsopprof_injection.so"});
    if (!CheckInputFileValid(soPath, "so")) {
        return revision;
    }
    void *handle = dlopen(soPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (handle == nullptr) {
        return revision;
    }
    using FuncType = char const *(*)();
    FuncType func = reinterpret_cast<FuncType>(dlsym(handle, "GetFuncInjectionRevision"));
    if (func != nullptr) {
        auto ret = func();
        if (ret != nullptr) {
            revision = ret;
        }
    }
    dlclose(handle);
    return revision;
}

void PrintVersion()
{
    PrintLogo();
    const std::string revision = GetFuncInjectionRevision();
    std::cout
        << "msopprof " << __PACKAGE_VERSION__ << " (" << __MSOPPROF_COMMIT_REVISION__ << ")\n"
        << "Copyright (c) 2026 Huawei Technologies Co., Ltd.\n"
        << "License: Mulan PSL v2.\n\n"
        << "Build Info:\n"
        << "  Date: " << __MSOPPROF_BUILD_DATE__ << "\n"
        << "  Repo: https://gitcode.com/Ascend/msopprof";
    if (revision.find_first_not_of(" \t\r\n") != std::string::npos &&
        revision.find("unknown") == std::string::npos && revision.find("UNKNOWN") == std::string::npos) {
        std::cout << "\nDependencies:\n  msopscommon: " << revision;
    }
    std::cout << std::endl;
}

void PrintDeviceHelp(ChipType chipType)
{
    std::cout << "Description:" << std::endl
              << "  Profile Ascend C operators by running the application on an Ascend device." << std::endl
              << std::endl
              << "Usage:" << std::endl
              << "  msopprof [options] (<application> [application-args] | --config <FILE>)" << std::endl
              << std::endl
              << "Optional arguments:" << std::endl
              << "  Exactly one of <application> and --config must be specified." << std::endl
              << "  -h, --help                                Show help message." << std::endl
              << "  -V, --version                             Show version information." << std::endl
              << "      --config <FILE>                       JSON operator configuration file." << std::endl
              << "      --application <FILE>                  Application executable path." << std::endl
              << "      --output <DIR>                        Output directory [default: ./]." << std::endl
              << "      --aic-metrics <NAME>[,<NAME>...]      Enable collection ability type:" << std::endl
              << "                                              Supported values: ArithmeticUtilization | MemoryUB | Memory | "
                 "MemoryL0 | L2Cache |"
              << std::endl
              << "                                                | PipeUtilization | ResourceConflictRatio | Default "
                 "| BasicInfo | Roofline |"
              << std::endl;
    if (chipType == Common::ChipType::ASCEND910B) {
        std::cout << "                                                | Occupancy | TimelineDetail | KernelScale | Source | MemoryDetail |" << std::endl;
    }
    if (chipType == Common::ChipType::ASCEND950) {
        std::cout << "                                                | Occupancy | TimelineDetail | KernelScale | Source | MemoryDetail |" << std::endl
                  << "                                                | PCSampling | PipeTimeline | InstrTimeline |" << std::endl
                  << "      --instr-timeline-pipe <PIPE>          Specify the pipe for InstrTimeline." << std::endl;
    }
    std::cout << "      --kernel-name <NAME>                  Specify the kernel name to profile." << std::endl
              << "                                              Not effective in config mode." << std::endl
              << "      --launch-count <N>                    Kernel launches to collect (1-5000) [default: 1]." << std::endl
              << "      --launch-skip-before-match <N>        Kernel launches to skip (0-1000) [default: 0]." << std::endl
              << "      --replay-mode {application,kernel";
    if (chipType == Common::ChipType::ASCEND910B || chipType == Common::ChipType::ASCEND950) {
        std::cout << ",range";
    }
    std::cout
        << "}              Data collection replay mode [default: kernel]." << std::endl
        << "      --kill {true,false}                       Kill the process after launch-count [default: false]." << std::endl
        << "      --mstx {true,false}                       Enable mstx API [default: false]." << std::endl
        << "      --mstx-include <RANGE>                Specify the mstx range to collect." << std::endl
        << "      --warm-up <N>                         Warm-up runs (0-500) [default: 5]." << std::endl;
    if (chipType == Common::ChipType::ASCEND910B) {
        std::cout << "      --dump {true,false}                       Enable TimelineDetail dump [default: false]." << std::endl;
    }
    if (chipType == Common::ChipType::ASCEND910B || chipType == Common::ChipType::ASCEND950) {
        std::cout << "      --core-id <ID>                        Specify the id of cores to be parsed." << std::endl
                  << "                                              Only effective when --aic-metrics=<TIMELINEDETAIL> and only" << std::endl
                  << "                                              effective in simulation products." << std::endl;
    }
    std::cout << "      --custom-input <FILE>                 JSON file for custom operator input." << std::endl
              << std::endl
              << "Examples:" << std::endl
              << "  msopprof ./my_operator" << std::endl
              << "  msopprof --config ./op_config.json" << std::endl
              << std::endl
              << "Output:" << std::endl
              << "  Results are written to <output-dir>/OPPROF_{timestamp}_{random}." << std::endl
              << std::endl
              << "Troubleshooting:" << std::endl
              << "  Check that exactly one input is provided and option values are valid." << std::endl;
}

void PrintSimulatorHelp(void)
{
    std::cout
        << "Description:" << std::endl
        << "  Profile Ascend C operators by running the application on the simulator." << std::endl
        << std::endl
        << "Usage:" << std::endl
        << "  msopprof simulator [options] (<application> [application-args] | --config <FILE> | --export <DIR>)" << std::endl
        << std::endl
        << "Optional arguments:" << std::endl
        << "  Exactly one of <application>, --config, and --export must be specified." << std::endl
        << "  -h, --help                                Show help message." << std::endl
        << "  -V, --version                             Show version information." << std::endl
        << "      --config <FILE>                       JSON operator configuration file." << std::endl
        << "      --application <FILE>                  Application executable path." << std::endl
        << "      --export <DIR>                        Simulator data directory to parse." << std::endl
        << "      --output <DIR>                        Output directory [default: ./]." << std::endl
        << "      --kernel-name <NAME>                  Specify the kernel name to profile." << std::endl
        << "                                              Not effective in config mode." << std::endl
        << "      --launch-count <N>                    Kernel launches to collect (1-5000) [default: 1]." << std::endl
        << "      --aic-metrics <NAME>[,<NAME>...]      Enable collection ability type:" << std::endl
        << "                                              Supported values: PipeUtilization | ResourceConflictRatio |" << std::endl
        << "                                                | PMSampling | Overhead |" << std::endl
        << "                                              PipeUtilization is required." << std::endl
        << "      --mstx {true,false}                       Enable mstx API [default: false]." << std::endl
        << "      --mstx-include <RANGE>                Specify the mstx range to collect." << std::endl
        << "      --soc-version <VERSION>               Simulator version; not effective in config mode." << std::endl
        << "      --core-id <ID>                        Specify the id of cores to parse." << std::endl
        << "      --timeout <MINUTES>                   Application timeout (1-2880 minutes)." << std::endl
        << "      --dump {true,false}                       Enable dump mode for A2/A3 [default: false]." << std::endl
        << std::endl
        << "Examples:" << std::endl
        << "  msopprof simulator ./my_operator" << std::endl
        << "  msopprof simulator --config ./op_config.json" << std::endl
        << "  msopprof simulator --export ./simulator_output" << std::endl
        << std::endl
        << "Output:" << std::endl
        << "  Results are written to <output-dir>/OPPROF_{timestamp}_{random}." << std::endl
        << std::endl
        << "Troubleshooting:" << std::endl
        << "  Check that exactly one input is provided and option values are valid." << std::endl;
}

void PrintErrorMsg(std::string const &msg)
{
    if (!msg.empty()) {
        LogError("%s", msg.c_str());
    }
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

    if (parseRet && args.deprecatedVersion) {
        std::cerr << "WARNING: '-v' is deprecated; use '-V' instead." << std::endl;
        args.printVersion = true;
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

    if (args.printHelp || args.printVersion) {
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
