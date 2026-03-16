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


#ifndef __MSOPPROF_PARSE_PLUGIN_INTERFACE_H__
#define __MSOPPROF_PARSE_PLUGIN_INTERFACE_H__

#include <vector>
#include "common/defs.h"
#include "parse/data_center/data_center.h"
#include "base_context.h"

namespace Profiling {
namespace Parse {

enum class PluginErrorCode : uint32_t {
    SUCCESS = 0,
    NONBLOCKING_ERROR,
    FATAL_ERROR,
    ERROR_CODE_END,
};

struct PluginInfo {
    std::string pluginName;                         // 本插件的类名
    std::vector<std::type_index> mandatoryDb;       // 本插件必须得数据块
    std::vector<std::type_index> optionalDb;        // 本插件可选数据块
    std::set<ChipProductType> chipSupport;         // 本插件支持的芯片类型
    bool isKeyPlugin = false;  // 是否为关键插件，关键插件失败后，会停止后续流程导致整体失败 true表示关键插件
};

class PluginInterface {
public:
    explicit PluginInterface(DataCenter& dataCenter, ChipProductType chipType)
        : dataCenter_(dataCenter), chipType_(chipType) {};
    virtual ~PluginInterface() = default;

    PluginErrorCode Run();

    void SetEntry(bool comeIn)
    {
        isEntry_.store(comeIn);
    }

    bool GetEntry()
    {
        return isEntry_.load();
    }

protected:
    // 子类必须实现的函数接口
    virtual PluginErrorCode Entry() = 0;
    virtual void DependencyRegister() = 0; // 要求调用DependencyRegisterxxxx系列接口实现注册

    // 父类提供的公共方法
    bool DependencyCheck(ChipProductType profChipType) const;
    void RegisterPluginName(const std::string& pluginName)
    {
        pluginInfo_.pluginName = pluginName;
    }

    void RegisterMandatoryDb(std::initializer_list<std::type_index> requiredDb)
    {
        pluginInfo_.mandatoryDb = requiredDb;
    }

    void RegisterOptionalDb(std::initializer_list<std::type_index> requiredDb)
    {
        pluginInfo_.optionalDb = requiredDb;
    }

    void RegisterChip(std::initializer_list<ChipProductType> supportChipType)
    {
        pluginInfo_.chipSupport = std::set<ChipProductType>(supportChipType);
    }

    void RegisterKeyPlugin(bool isKeyPlugin)
    {
        pluginInfo_.isKeyPlugin = isKeyPlugin;
    }

    PluginInfo pluginInfo_;
    DataCenter& dataCenter_;
    ChipProductType chipType_;

private:
    bool IsBaseProductTypeRegistered(const ChipProductType &profChipType) const;
    std::atomic<bool> isEntry_ {false};
};

}
}

#endif // __MSOPPROF_PARSE_PLUGIN_INTERFACE_H__
