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

#include "storage_access.h"

#include "basic_op_and_pmu.h"

using namespace Common;
namespace Visualize {

// OpBasicInfo类成员函数定义
void OpBasicInfo::ClearOpBasicInfo()
{
    std::string nullStr = "";
    std::map<uint16_t, std::vector<float>> blockDetailMap;
    handler_->SetOpName(nullStr);
    handler_->SetSoc(nullStr);
    handler_->ClearOpType();
    handler_->SetBlockDim(nullStr);
    handler_->SetMixBlockDim(nullStr);
    handler_->SetDeviceId(nullStr);
    handler_->SetPid(nullStr);
    blockDetailMap_.clear();
}

std::vector<std::string> OpBasicInfo::GetBlockDetailHeader(bool is310P, bool isMix) const
{
    std::vector<std::string> headerRes;
    std::vector<std::string> blockDetailHeader = {"Core Type", "Duration (μs)"};
    std::vector<std::string> blockDetailHeaderMix = {"Cube0 Duration (μs)", "Vector0 Duration (μs)",
        "Vector1 Duration (μs)"};
    if (!is310P) {
        headerRes.emplace_back("Block ID");
    }
    auto header = (!is310P && isMix) ? blockDetailHeaderMix : blockDetailHeader;
    headerRes.insert(headerRes.end(), header.begin(), header.end());
    return headerRes;
}

void OpBasicInfo::OpBasicInfoToJson()
{
    std::string opType = GetOpType();
    opBasicFileJson_["name"] = GetOpName();
    opBasicFileJson_["soc"] = GetSoc();
    opBasicFileJson_["op_type"] = opType;
    opBasicFileJson_["block_dim"] = GetBlockDim();
    opBasicFileJson_["mix_block_dim"] = GetMixBlockDim();
    opBasicFileJson_["device_id"] = GetDeviceId();
    opBasicFileJson_["pid"] = GetPid();
    opBasicFileJson_["duration"] = GetDuration();
    opBasicFileJson_["cur_freq"] = GetCurFreq();
    opBasicFileJson_["rated_freq"] = GetRatedFreq();
    bool is310P = (opType == Common::OpType::AI_CORE);  // true为310P
    bool isMix = (opType == "mix");
    const std::map<uint16_t, std::vector<float>> &blockDetailMap = GetBlockDetail();
    nlohmann::json blockDetailJson;
    std::vector<std::string> header = GetBlockDetailHeader(is310P, isMix);
    for (size_t i = 0; i < header.size(); ++i) {
        blockDetailJson["head_name"].emplace_back(header[i]);
    }
    blockDetailJson["size"] = {blockDetailMap.size() + 1, header.size()};    // 非310P展示block id
    std::vector<nlohmann::json> blockDetailRows;
    for (const auto &pair : blockDetailMap) {
        nlohmann::json rows;
        if (!is310P) {
            rows["value"].emplace_back(std::to_string(pair.first));
        }
        if (is310P || !isMix) {
            rows["value"].emplace_back(opType);
        }
        for (size_t i = 0; i < pair.second.size(); ++i) {
            rows["value"].emplace_back(std::to_string(pair.second[i]));
        }
        blockDetailRows.emplace_back(rows);
    }
    blockDetailJson["row"] = blockDetailRows;
    if (opType == "mix") {
        opBasicFileJson_["mix_block_detail"] = blockDetailJson;
    } else {
        opBasicFileJson_["block_detail"] = blockDetailJson;
    }
    opBasicFileJson_["advice"] = GenFreAdvice();
}

std::vector<nlohmann::json> OpBasicInfo::GenFreAdvice()
{
    std::vector<nlohmann::json> advice;
    static constexpr float freqFactor = 0.95f;
    int64_t curFreq;
    int64_t ratedFreq;
    if (Utility::StringToNum(GetCurFreq(), curFreq) && Utility::StringToNum(GetRatedFreq(), ratedFreq)) {
        if (curFreq <= freqFactor * ratedFreq) {
            Utility::LogWarn("The current frequency is lower than the rated frequency");
            advice.emplace_back("The current frequency is lower than the rated frequency");
        }
    }
    return advice;
}

void OpBasicInfo::ShowOpBasicInfo()
{
    std::string summary = std::string("\t") + "Op Name: " + opName_ + "\n" +
                          std::string("\t") + "Op Type: " + opType_ + "\n" +
                          std::string("\t") + "Task Duration(us): " + std::to_string(duration_) + "\n" +
                          std::string("\t") + "Block Dim: " + blockDim_ + "\n" +
                          std::string("\t") + "Mix Block Dim: " + mixBlockDim_ + "\n" +
                          std::string("\t") + "Device Id: " + deviceId_ + "\n" +
                          std::string("\t") + "Pid: " + pid_ + "\n" +
                          std::string("\t") + "Current Freq: " + curFreq_ + "\n" +
                          std::string("\t") + "Rated Freq: " + ratedFreq_ + "\n";

    Utility::LogSummary("Operator Basic Information:\n\n" + summary);
}

void OpBasicInfo::ClearOpBasicFileJson()
{
    ClearOpBasicInfo();
    opBasicFileJson_.clear();
}
}