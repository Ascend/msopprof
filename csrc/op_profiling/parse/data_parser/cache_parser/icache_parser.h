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


#ifndef MSOPT_ICACHE_PARSER_H
#define MSOPT_ICACHE_PARSER_H

#include <vector>
#include <string>
#include <map>
#include "parse/data_parser/sim_data_parser_config.h"
#include "parse/data_parser/sim_data_parser.h"
#include "parse/data_center/data_center.h"
#include "parse/plugin/plugin_interface.h"
#include "parse/data_table/cache_detail_table.h"

namespace Profiling {
namespace Parse {

class ICacheParser : public SimDataParser {
public:
    explicit ICacheParser(DataCenter &dataCenter, SimDataParserConfig& config);
    PluginErrorCode Entry() override;
    void DependencyRegister() override
    {
        RegisterPluginName("CacheParser");
        RegisterMandatoryDb({});
        RegisterChip({ChipProductType::ALL_PRODUCT_TYPE});
    }
private:
    void ParseLine(const std::string &line);
    std::vector<MergeInfo> cacheInstr_;
    std::map<std::string, uint64_t> iCacheRuleNamePos_ = {{"tick", 1}, {"pc", 2}, {"detail", 3}};
    // 默认按照310P规则进行解析
    std::regex instrMatchPattern_;
    std::string suffix_;
    std::string fileName_;
};
}
}

#endif // MSOPT_ICACHE_PARSER_H
