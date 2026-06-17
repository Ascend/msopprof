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


#ifndef MSOPT_VISUALIZE_H
#define MSOPT_VISUALIZE_H

#include "json.hpp"
#include "log.h"

namespace Utility {
constexpr char const *VISUALIZE_DATA_BIN = "visualize_data.bin";
constexpr uint8_t RESERVED_VALUE = 0x5a;

enum class VisualizeBinDType : int {
    DO_NOT_DISPLAY = 0,
    INT = 1,
    FLOAT = 2,
    STRING = 3,
    PERCENTAGE = 4,
    JSON_STR = 5,     // 展示寄存器状态信息数据
    CUSTOM_PERCENTAGE = 100,     // 定制类型，悬浮展示百分比
};

enum class VisualizeType : uint8_t {
    INVALID = 0x0,
    CODE = 0x1,
    TRACE = 0x2,
    FILE_API = 0x3,
    INSTR_API = 0x4,
    OP_BASIC_INFO = 0x5,
    COMPUTE_LOAD_FIGURE = 0x6,
    COMPUTE_LOAD_TABLE = 0x7,
    STORAGE_ACCESS_HEAT_MAP = 0x8,
    STORAGE_ACCESS_TABLE = 0x9,
    OCCUPANCY_MAP = 0xc,
    ROOF_LINE = 0xd,
    CACHELINE_HEAT_MAP = 0xe,
    TOP_STALL_REASON = 0xf,
    END,
};

const std::map<VisualizeType, std::string> VISUALIZE_TYPE_STR = {
    {VisualizeType::INVALID,                 "Invalid type"},
    {VisualizeType::CODE,                    "Code"},
    {VisualizeType::TRACE,                   "Trace"},
    {VisualizeType::FILE_API,                "FileApi"},
    {VisualizeType::INSTR_API,               "InstrApi"},
    {VisualizeType::OP_BASIC_INFO,           "OpBasicInfo"},
    {VisualizeType::COMPUTE_LOAD_FIGURE,     "ComputeLoadFigure"},
    {VisualizeType::COMPUTE_LOAD_TABLE,      "ComputeLoadTable"},
    {VisualizeType::STORAGE_ACCESS_HEAT_MAP, "StorageAccessHeatMap"},
    {VisualizeType::STORAGE_ACCESS_TABLE,    "StorageAccessTable"},
    {VisualizeType::OCCUPANCY_MAP,           "OccupancyMap"},
    {VisualizeType::ROOF_LINE,               "RoofLine"},
    {VisualizeType::CACHELINE_HEAT_MAP,      "CacheLineHeatMap"},
    {VisualizeType::TOP_STALL_REASON,        "TopStallReason"},
};

// total 12 color names for mindstudio-insight
struct VISUALIZE_COLOR_NAME {
    static constexpr char const *GRASS_GREEN = "thread_state_runnable";
    static constexpr char const *GREEN = "good";
    static constexpr char const *PINK = "thread_state_iowait";
    static constexpr char const *YELLOW = "thread_state_unknown";
    static constexpr char const *PURPLE = "heap_dump_child_node_arrow";
    static constexpr char const *BLUE = "thread_state_running";
    static constexpr char const *ORANGE = "startup";
    static constexpr char const *CYAN = "background_memory_dump";
    static constexpr char const *RED = "rail_response";
    // add three color names
    static constexpr char const *VIVID_BLUE = "thread_state_uninterruptible";
    static constexpr char const *AMETHYST_PURPLE = "head_dump_object_type";
    static constexpr char const *SKY_BLUE = "cq_build_attempt_passed";
};
const std::vector<std::string> TOTAL_CNAME_MAP = {
    std::string(VISUALIZE_COLOR_NAME::GRASS_GREEN), std::string(VISUALIZE_COLOR_NAME::GREEN), std::string(VISUALIZE_COLOR_NAME::PINK),
    std::string(VISUALIZE_COLOR_NAME::YELLOW), std::string(VISUALIZE_COLOR_NAME::PURPLE), std::string(VISUALIZE_COLOR_NAME::BLUE),
    std::string(VISUALIZE_COLOR_NAME::ORANGE), std::string(VISUALIZE_COLOR_NAME::CYAN), std::string(VISUALIZE_COLOR_NAME::RED),
    std::string(VISUALIZE_COLOR_NAME::VIVID_BLUE), std::string(VISUALIZE_COLOR_NAME::AMETHYST_PURPLE), std::string(VISUALIZE_COLOR_NAME::SKY_BLUE),
};
/*
 * Visualize class writes string to `${outputPath}/visualize_data.bin` in append mode.
 * A user-defined data header is added to form a visualize_data.bin.
 * use like:
 * 1. Utility::VisualizeFactory::Visualize<Utility::VisualizeType::FILE_API>(outputPath, apiFileJson);
 * 2. Utility::VisualizeFactory::Visualize<Utility::VisualizeType::CODE>(outputPath, fileContent, fileName);
 * Data header's version control is in XXXWriter::Write(...), please explicit comment what newer version do.
 * For some historical reasons, default version(reserved bytes also) were filled with 0x5A.
 * */
class Visualize {
public:
    template<VisualizeType type, typename... Args>
    static void WriteBin(std::string outputPath, Args&&... args)
    {
        typename VisualizeType2Writer<type>::Writer writer(type, std::move(outputPath));
        writer.WriteBin(std::forward<Args>(args)...);
    }

private:
    template<typename Derived>
    class VisualizeWriter {
    public:
        VisualizeWriter(VisualizeType visualizeType, std::string outputPath)
            : visualizeType_(visualizeType), outputPath_(std::move(outputPath)) {}
        template<typename... Args>
        inline void WriteBin(const std::string &content, Args&&... args) const
        {
            if (!Check(content)) {
                return;
            }
            static_cast<const Derived*>(this)->Write(content, std::forward<Args>(args)...);
        }

        template<typename... Args>
        inline void WriteBin(const nlohmann::json &content, Args&&... args) const
        {
            WriteBin(content.dump(), std::forward<Args>(args)...);
        }

    protected:
        bool Check(const std::string &content) const
        {
            if (VISUALIZE_TYPE_STR.find(visualizeType_) == VISUALIZE_TYPE_STR.end()) {
                LogWarn("Visualize data type is invalid %u", static_cast<uint32_t>(visualizeType_));
                return false;
            }
            const std::string &typeStr = VISUALIZE_TYPE_STR.at(visualizeType_);
            if (content.empty()) {
                LogWarn("%s content for %s is empty, skip.", typeStr.c_str(), VISUALIZE_DATA_BIN);
                return false;
            }
            if (outputPath_.empty()) {
                LogWarn("%s output path for %s is empty, skip.", typeStr.c_str(), VISUALIZE_DATA_BIN);
                return false;
            }
            return true;
        }
        void WriteVisualizeBin(const std::string &header, const std::string &&data) const;

        VisualizeType visualizeType_;
        std::string outputPath_;
    };

    class DefaultWriter : public VisualizeWriter<DefaultWriter> {
    friend class VisualizeWriter<DefaultWriter>;
    public:
        DefaultWriter(VisualizeType visualizeType, std::string outputPath)
            : VisualizeWriter<DefaultWriter>(visualizeType, std::move(outputPath)) {}
    private:
        void Write(const std::string &content) const;
    };

    // VisualizeType::CODE = 0x1
    class CodeWriter : public VisualizeWriter<CodeWriter> {
        friend class VisualizeWriter<CodeWriter>;
    public:
        CodeWriter(VisualizeType visualizeType, std::string outputPath)
            : VisualizeWriter<CodeWriter>(visualizeType, std::move(outputPath)) {}
    private:
        void Write(const std::string &content, const std::string &codeFile) const;
    };

    // VisualizeType::INSTR_API = 0x4
    class InstrApiWriter : public VisualizeWriter<InstrApiWriter> {
        friend class VisualizeWriter<InstrApiWriter>;
    public:
        InstrApiWriter(VisualizeType visualizeType, std::string outputPath)
            : VisualizeWriter<InstrApiWriter>(visualizeType, std::move(outputPath)) {}
    private:
        void Write(const std::string &content) const;
    };

    template<VisualizeType type>
    struct VisualizeType2Writer { using Writer = DefaultWriter; };
};

template<>
struct Visualize::VisualizeType2Writer<VisualizeType::CODE> { using Writer = CodeWriter; };
template<>
struct Visualize::VisualizeType2Writer<VisualizeType::INSTR_API> { using Writer = InstrApiWriter; };
} // namespace Utility
#endif // MSOPT_VISUALIZE_H
