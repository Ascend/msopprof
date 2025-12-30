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


#include "icache_parser.h"
using namespace Utility;

namespace Profiling {
namespace Parse {

const std::string I_CACHE_FILE_A2_A3 = "ifu.icache_log";
const std::string I_CACHE_FILE_A300_910_A = "icache_log";
const std::string I_CACHE_FILE_310_B = "ifu.icache_log";

const std::map<Common::ChipProductType, std::regex> ICachePattern = {
    {Common::ChipProductType::ASCEND910B_SERIES,
        std::regex("^(?:\\[info\\] )?([0-9]+): icache read address is (0x[0-9a-f]{1,16})"
            ", (size is 0x[0-9a-f]{8}, type:[0-9]{0,3}, last:[0-9]{0,3}, status is MISS$)")},
    {Common::ChipProductType::ASCEND910_93_SERIES,
        std::regex("^(?:\\[info\\] )?([0-9]+): icache read address is (0x[0-9a-f]{1,16})"
            ", (size is 0x[0-9a-f]{8}, type:[0-9]{0,3}, last:[0-9]{0,3}, status is MISS$)")},
    {Common::ChipProductType::ASCEND310B_SERIES,
        std::regex("(?:\\[info\\] )?@([0-9]+): icache read address is (0x[0-9a-f]{1,16})"
            ", (size is 0x[0-9a-f]{8}, status is MISS)")},
    {Common::ChipProductType::ASCEND310P_SERIES,
        std::regex("(?:\\[info\\] )?\\[([0-9]+)\\]: icache read address is (0x[0-9a-f]{1,16})"
            ", (size is 0x[0-9a-f]{8}, status is MISS)")},
    {Common::ChipProductType::ASCEND910A_SERIES,
        std::regex("(?:\\[info\\] )?\\[([0-9]+)\\]: icache read address is (0x[0-9a-f]{1,16})"
            ", (size is 0x[0-9a-f]{8}, status is MISS)")},
};

const std::string DUMP_SUFFIX = ".dump";
const std::string LOG_SUFFIX = ".log";

// ICachePattern, ICacheFile, ICacheSuffix 中的数据key必须都一致
const std::map<Common::ChipProductType, std::string> ICacheFile = {
    {Common::ChipProductType::ASCEND910B_SERIES, I_CACHE_FILE_A2_A3},
    {Common::ChipProductType::ASCEND910_93_SERIES, I_CACHE_FILE_A2_A3},
    {Common::ChipProductType::ASCEND310B_SERIES, I_CACHE_FILE_310_B},
    {Common::ChipProductType::ASCEND310P_SERIES, I_CACHE_FILE_A300_910_A},
    {Common::ChipProductType::ASCEND910A_SERIES, I_CACHE_FILE_A300_910_A},
};

const std::map<Common::ChipProductType, std::string> ICacheSuffix = {
    {Common::ChipProductType::ASCEND910B_SERIES, DUMP_SUFFIX},
    {Common::ChipProductType::ASCEND910_93_SERIES, DUMP_SUFFIX},
    {Common::ChipProductType::ASCEND310B_SERIES, LOG_SUFFIX},
    {Common::ChipProductType::ASCEND310P_SERIES, DUMP_SUFFIX},
    {Common::ChipProductType::ASCEND910A_SERIES, DUMP_SUFFIX},
};

ICacheParser::ICacheParser(DataCenter &dataCenter, SimDataParserConfig& config) : SimDataParser(dataCenter, config)
{
    Common::ChipProductType productSeries = dataParserConfig_.GetProductSeriesType();
    const std::string &coreInfo = dataParserConfig_.GetCoreInfo().second;
    // 默认初始化，使用310P赋值
    instrMatchPattern_ = ICachePattern.at(Common::ChipProductType::ASCEND310P_SERIES);
    suffix_ = DUMP_SUFFIX;
    fileName_ = coreInfo + ICacheFile.at(Common::ChipProductType::ASCEND310P_SERIES);
    // 在map中的其他类自行更新使用文件
    if (ICachePattern.find(productSeries) != ICachePattern.end() && ICacheFile.find(productSeries) != ICacheFile.end()
        && ICacheSuffix.find(productSeries) != ICacheSuffix.end()) {
        instrMatchPattern_ = ICachePattern.at(productSeries);
        suffix_ = ICacheSuffix.at(productSeries);
        fileName_ = coreInfo + ICacheFile.at(productSeries);
    }
}

void ICacheParser::ParseLine(const std::string &line)
{
    std::smatch lineMatch;
    bool res = std::regex_match(line, lineMatch, instrMatchPattern_);
    if (!res) {
        return;
    }
    // 行解析得到原始数据
    std::string detail = lineMatch[iCacheRuleNamePos_["detail"]].str();
    std::string pc = lineMatch[iCacheRuleNamePos_["pc"]].str();
    TrimBlank(detail);
    MergeInfo cache;
    if (!StoullConverter(lineMatch[iCacheRuleNamePos_["tick"]].str(), cache.tick, RADIX_10) ||
        !StoullConverter(lineMatch[iCacheRuleNamePos_["pc"]].str(), cache.pc, RADIX_16)) {
        LogWarn("Failed to convert cache str pc %s to int", pc.c_str());
        return;
    }
    cache.startTick = cache.tick;
    cache.endTick = cache.tick + 1;
    cache.pipe = "CACHEMISS";
    cache.name = pc;
    cache.detail = detail;
    cache.gprCount = 0;
    cache.ubWriteConflict = DEFAULT_INT_VALUE;
    cache.ubReadConflict = DEFAULT_INT_VALUE;
    cache.vecUtilization = static_cast<float>(DEFAULT_INT_VALUE);
    cache.processBytes = DEFAULT_INT_VALUE;
    cache.warpId = DEFAULT_INT_VALUE;
    cache.schId = DEFAULT_INT_VALUE;
    cacheInstr_.emplace_back(cache);
}

PluginErrorCode ICacheParser::Entry()
{
    std::vector<std::string> splitDumpFileVec;
    splitDumpFileVec = dataParserConfig_.GetSplitFilesVec(fileName_, suffix_);
    bool filterOk = true;
    for (const std::string &splitFile : splitDumpFileVec) {
        if (!CheckInputFileValid(splitFile, "dump", INPUT_DUMP_FILE_MAX_SIZE)) {
            continue;
        }
        std::vector<std::string> fileLines;
        ReadFileByMMap(splitFile, fileLines);
        if (fileLines.empty()) {
            Utility::LogWarn("file is empty or not exist. splitFile: %s", splitFile.c_str());
            filterOk = false;
            continue;
        }
        for (const auto &line : fileLines) {
            ParseLine(line);
        }
    }
    if (cacheInstr_.empty() && filterOk) {
        LogDebug("Parser %s cache info is empty because all hit", dataParserConfig_.GetCoreInfo().first.c_str());
    }
    auto cacheDetailTable = Utility::MakeShared<Parse::CacheDetailTable>(cacheInstr_);
    if (cacheDetailTable != nullptr && !dataCenter_.DataTableRegister(cacheDetailTable)) {
        LogWarn("Failed to register cache table");
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    return PluginErrorCode::SUCCESS;
}

}
}