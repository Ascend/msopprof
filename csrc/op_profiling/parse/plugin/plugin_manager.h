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


#ifndef __MSOPPROF_PARSE_PLUGIN_MANAGER_H__
#define __MSOPPROF_PARSE_PLUGIN_MANAGER_H__

#include "thread_pool.h"
#include "smart_pointer.h"

#include "plugin_interface.h"

namespace Profiling {
namespace Parse {

class PluginManager {
public:
    explicit PluginManager(uint32_t threadsNum) : threadPoolPtr_(new Utility::ThreadPool(threadsNum)) {}
    ~PluginManager()
    {
        delete threadPoolPtr_;
        threadPoolPtr_ = nullptr;
    }

    template<typename T, typename... Args>
    void AddPlugin(Args&&... args)
    {
        auto plugin = Utility::MakeShared<T>(std::forward<Args>(args)...);
        if (!std::is_base_of<PluginInterface, T>::value || plugin == nullptr) {
            Utility::LogDebug("Add plugin failed");
            return;
        }
        plugins_.emplace_back(plugin);
    }

    template<typename T>
    void AddPlugin(const std::shared_ptr<T> &plugin)
    {
        if (!std::is_base_of<PluginInterface, T>::value || plugin == nullptr) {
            Utility::LogDebug("Add plugin failed");
            return;
        }
        plugins_.emplace_back(plugin);
    }

    void RunAllPlugins(std::vector<PluginErrorCode> &res)
    {
        threadPoolPtr_->Start();
        res.resize(plugins_.size());
        int index = 0;
        for (const auto &plugin : plugins_) {
            threadPoolPtr_->AddTask([plugin, index, &res]() {
                res[index] = plugin->Run();
            });
            index++;
        }
        WaitForStop();
    }

    void RunAllPluginsNoBlock()
    {
        threadPoolPtr_->Start();
        for (const auto &plugin : plugins_) {
            threadPoolPtr_->AddTask([plugin]() {
                plugin->Run();
            });
        }
        uint32_t count = 0;
        while (count <= 50) { // 50次一共5s
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            count++;
            uint32_t runPluginSize = 0;
            for (const auto &plugin : plugins_) {
                if (plugin != nullptr && plugin->GetEntry()) {
                    runPluginSize++;
                }
            }
            if (runPluginSize == plugins_.size()) {
                break;
            }
        }
    }

    void WaitForStop() const
    {
        if (threadPoolPtr_ == nullptr) {
            return;
        }
        threadPoolPtr_->WaitAllTasks();
        threadPoolPtr_->Stop();
    }

private:
    std::vector<std::shared_ptr<PluginInterface>> plugins_;
    Utility::ThreadPool* threadPoolPtr_;
};

}
}


#endif // __MSOPPROF_PARSE_PLUGIN_MANAGER_H__
