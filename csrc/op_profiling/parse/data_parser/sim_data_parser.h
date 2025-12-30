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


#ifndef MSOPT_SIM_DATA_PARSER_H
#define MSOPT_SIM_DATA_PARSER_H

#include "parse/plugin/plugin_interface.h"
#include "parse/plugin/plugin_manager.h"
#include "sim_data_parser_config.h"

namespace Profiling {
namespace Parse {

class SimDataParser : public PluginInterface {
public:
    explicit SimDataParser(DataCenter &dataCenter, SimDataParserConfig& config)
        : PluginInterface(dataCenter, config.GetChipType()), dataParserConfig_(config) {};

protected:
    SimDataParserConfig dataParserConfig_;
};
class RealTimeLogParer {
public:
    RealTimeLogParer(RealTimeSimParseContext context, uint32_t threadsNum) : pluginManager_(threadsNum),
        context_(std::move(context)) {}
    virtual void Start() = 0;
    virtual void Stop() = 0;
    DataCenter dataCenter_;
    PluginManager pluginManager_;
    RealTimeSimParseContext context_;
};
}
}
#endif // MSOPT_SIM_DATA_PARSER_H
