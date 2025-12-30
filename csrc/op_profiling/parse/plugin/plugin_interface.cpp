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


#include <algorithm>

#include "plugin_interface.h"

namespace Profiling {
namespace Parse {

PluginErrorCode PluginInterface::Run()
{
    DependencyRegister();
    if (!DependencyCheck(chipType_)) {
        if (pluginInfo_.isKeyPlugin) {
            Utility::LogError("Dependency of plugin %s are not satisfied, error code is %u",
                              pluginInfo_.pluginName.c_str(), static_cast<uint32_t>(PluginErrorCode::FATAL_ERROR));
            return PluginErrorCode::FATAL_ERROR;
        }
        Utility::LogWarn("Dependency of plugin %s are not satisfied, error code is %u",
                         pluginInfo_.pluginName.c_str(), static_cast<uint32_t>(PluginErrorCode::NONBLOCKING_ERROR));
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    return Entry();
}

bool PluginInterface::IsBaseProductTypeRegistered(const Common::ChipProductType &profChipType) const
{
    auto profChipBaseProductType = Common::GetProductSeriesType(profChipType);
    return pluginInfo_.chipSupport.find(profChipBaseProductType) != pluginInfo_.chipSupport.end();
}

bool PluginInterface::DependencyCheck(Common::ChipProductType profChipType) const
{
    // mandatory dbs are checked, optional db is checked in Entry by plugin functionality
    std::vector<bool> dbRes = dataCenter_.IsRequiredDbExist(pluginInfo_.mandatoryDb);
    if (std::find(dbRes.begin(), dbRes.end(), false) != dbRes.end()) {
        return false;
    }
    // 准备执行的profChipType是否插件支持。通过情况:全芯片支持 或 所属的芯片系列 或 具体芯片类型 被注册在pluginInfo_中
    if (pluginInfo_.chipSupport.find(Common::ChipProductType::ALL_PRODUCT_TYPE) == pluginInfo_.chipSupport.end() &&
        !IsBaseProductTypeRegistered(profChipType) &&
        pluginInfo_.chipSupport.find(profChipType) == pluginInfo_.chipSupport.end()) {
        return false;
    }
    return true;
}

}
}