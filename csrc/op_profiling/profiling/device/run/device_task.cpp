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


#include "device_task.h"

#include <set>
#include <cmath>
#include <sys/mman.h>
#include <fcntl.h>

#include "log.h"
#include "op_runner.h"
#include "common/defs.h"
#include "securec.h"
#include "filesystem.h"
#include "profiling/device/data_parse/dbi_parser.h"
#include "prof_injection/injection_event.h"

using namespace Utility;
using namespace Common;
using namespace std;

namespace Profiling {

bool DeviceTask::Run()
{
    LogInfo("Running device task: %s", taskName.c_str());
    RegisterRunningEvent();
    if (!PreProcess()) {
        return false;
    }
    bool taskRes;

    DBIParser dbiParser(outputPath);
    ProfStub::InjectionEvent::Instance().RegisterPacketHandler(
        {ProfStub::ProfPacketType::DBI_DATA, [&](const std::shared_ptr<ProfStub::Packet>& pkt, size_t) -> std::string {
            dbiParser.ParsePacket(pkt->GetClientId(), std::move(pkt->GetAskMsg()));
            return "";
        }}
    );
    // A2/A3 application需要采集PMU时需要额外采集1轮L2cache
    if (chipType_ == ChipType::ASCEND910B && profMessage_.l2CachePmu[0] != 0 && replayMode_ == ReplayMode::APPLICATION) {
        profMessage_.replayCount = 0;
        ProfStub::InjectionEvent::Instance().StartDisposeClientAsk(profMessage_, profConfig_);
        Task::execStatus = ExecStatus::RUNNING;
        taskRes = OpRunner::RunOpBinary(cmd, env);
        ProfStub::InjectionEvent::Instance().Stop();
        Task::execStatus = ExecStatus::STOPPED;
        profMessage_.l2CachePmu[0] = 0;
    }
    for (uint32_t i = 0U; i < replayCount_; ++i) {
        if (inExitMode) {
            break;
        }
        if (replayMode_ == ReplayMode::APPLICATION) {
            ProcessApplication(i);
            LogDebug("Application mode count is %lu, type is %d", profMessage_.replayCount, static_cast<int>(profMessage_.biType));
        }
        if (!ProfStub::InjectionEvent::Instance().StartDisposeClientAsk(profMessage_, profConfig_)) {
            LogError("Starting client failed, get profiling data failed.");
            continue;
        }
        Task::execStatus = ExecStatus::RUNNING;
        taskRes = OpRunner::RunOpBinary(cmd, env);
        ProfStub::InjectionEvent::Instance().Stop();
        Task::execStatus = ExecStatus::STOPPED;
        if (!taskRes) {
            LogError("Get profiling data failed.");
        }
    }
    dbiParser.WaitParseTask();
    ProfStub::InjectionEvent::Instance().UnregisterPacketHandler(ProfStub::ProfPacketType::DBI_DATA);
    if (realTimeDataParser_ != nullptr) {
        realTimeDataParser_->Stop();
    }
    return taskRes;
}

void DeviceTask::ProcessApplication(uint32_t count)
{
    profMessage_.replayCount = count;
    if (profMessage_.isSource && profMessage_.isMemoryDetail) {
        if (count == replayCount_ - 2) {
            profMessage_.biType = BIType::CUSTOMIZE;
        }
        if (count == replayCount_ - 1) {
            profMessage_.biType = BIType::BB_COUNT;
        }
        return;
    }
    if (profMessage_.isSource || profMessage_.isMemoryDetail) {
        if (count == replayCount_ - 1) {
            profMessage_.biType = profMessage_.isMemoryDetail ? BIType::CUSTOMIZE : BIType::BB_COUNT;
        }
        return;
    }
}

uint32_t DeviceTask::GetReplayTimes()
{
    uint32_t replayTimes = 1U;
    if (replayMode_ == ReplayMode::APPLICATION) {
        uint32_t aicMaxTimes = static_cast<uint32_t>(std::ceil(static_cast<double>
            (pmuValue_.aicPmu.size()) / PMU_EVENT_MAX_NUM));
        uint32_t aivMaxTimes = static_cast<uint32_t>(std::ceil(static_cast<double>
            (pmuValue_.aivPmu.size()) / PMU_EVENT_MAX_NUM));
        // 至少采集1次来获取时间，动态插桩自定义插桩分别增加1次重放
        replayTimes = std::max(aivMaxTimes, aicMaxTimes);
        replayTimes = std::max(replayTimes, 1U);
        replayTimes += static_cast<uint32_t>(profMessage_.isMemoryDetail);
        replayTimes += static_cast<uint32_t>(profMessage_.isSource);
    }
    LogDebug("Replay mode %d, replay times is %u", replayMode_, replayTimes);
    return replayTimes;
}

bool DeviceTask::PreProcess()
{
    if (!CreateTaskDir(outputPath)) {
        return false;
    }
    CreateTaskDir(tmpPath_);
    if (isDeviceToSimulator_) {
        CreateCamodelConfig(false);
    }
    profMessage_.isSimulator = false;
    profMessage_.profWarmUpTimes = profWarmUpTimes_;
    profMessage_.timelineEnable = metrics_.timelineEnable;
    profMessage_.pcSamplingEnable = metrics_.pcSamplingEnable;
    // generate pipe message for prof_injection before running task
    if (strcpy_s(profMessage_.mstxProfConfig.mstxEnabledMessage, sizeof(profMessage_.mstxProfConfig.mstxEnabledMessage),
                 mstxEnabledMessageString.c_str()) != 0) {
        LogError("Subprocess control message generate failed");
        return false;
    }
    profMessage_.mstxProfConfig.isMstxEnable = isMstxEnable;
    profMessage_.killAdvance = Profiling::Task::killAdvance;
    profMessage_.isDeviceToSimulator = isDeviceToSimulator_;
    if (metrics_.isKernelScale) {
        profMessage_.useProfileMode = true;
    }
    profMessage_.isSource = metrics_.isSource;
    profMessage_.isMemoryDetail = metrics_.isMemoryDetail;
    std::copy(pmuValue_.aicPmu.begin(), pmuValue_.aicPmu.end(), profMessage_.aicPmu);
    std::copy(pmuValue_.aivPmu.begin(), pmuValue_.aivPmu.end(), profMessage_.aivPmu);
    if (chipType_ == ChipType::ASCEND310P) {
        std::copy(L2_CACHE_EVENTS_FOR_310P.begin(), L2_CACHE_EVENTS_FOR_310P.end(), profMessage_.l2CachePmu);
    }
    if (chipType_ == ChipType::ASCEND910B && !(profMessage_.aicPmu[0] == 0 && profMessage_.aivPmu[0] == 0) && replayMode_ != ReplayMode::RANGE) {
        std::copy(L2_CACHE_EVENTS_FOR_A2_A3.begin(), L2_CACHE_EVENTS_FOR_A2_A3.end(), profMessage_.l2CachePmu);
    }
    profMessage_.replayMode = static_cast<uint8_t>(replayMode_);
    replayCount_ = GetReplayTimes();

    if (opRunMode == OpRunnerMode::RUN_KERNEL) {
        nlohmann::json jsonData;
        if (!GenOpConfig(jsonData)) {
            return false;
        }
    }
    LogDebug("Task %s profMessage generate success.", taskName.c_str());
    return true;
}
}
