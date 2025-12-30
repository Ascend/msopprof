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


#include "visualize.h"
#include <mutex>
#include "filesystem.h"
#include "data_format.h"
#include "communication/serializer.h"

using namespace std;

namespace Utility {
static std::mutex g_writeLock;
static constexpr uint32_t ALIGN = 4;

#pragma pack(4)
struct BinaryBlockHeader {
    uint64_t contentSize = 0;
    uint8_t type = 0;
    uint8_t padding = 0;
    uint8_t version = RESERVED_VALUE;
    uint8_t reverse = RESERVED_VALUE;
};

struct BinaryBlockHeaderCode {
    BinaryBlockHeader binaryBlockHeader{};
    char filePath[PATH_MAX] = {0};
};
#pragma pack()

template<typename Derived>
void Visualize::VisualizeWriter<Derived>::WriteVisualizeBin(const string &header, const string &&data) const
{
    std::lock_guard<std::mutex> lock(g_writeLock);
    std::string binFilePath = JoinPath({outputPath_, VISUALIZE_DATA_BIN});
    std::ofstream binFile(binFilePath, std::ios::binary | std::ios::app);
    if (!binFile.is_open()) {
        LogWarn("Can not open file [%s]", binFilePath.c_str());
        return;
    }
    long oriSize = static_cast<long>(binFile.tellp());
    binFile.write(header.data(), static_cast<std::streamsize>(header.size()));
    binFile.write(data.data(), static_cast<std::streamsize>(data.size()));
    binFile.close();
    if (binFile.fail()) {
        LogWarn("Failed to write data to %s.", VISUALIZE_DATA_BIN);
        try {
            truncate(binFilePath.c_str(), oriSize);
        } catch (std::exception &ex) {
            LogDebug("Restore %s failed, reason is %s.", VISUALIZE_DATA_BIN, ex.what());
        }
    }
    chmod(binFilePath.c_str(), Utility::SAVE_DATA_FILE_AUTHORITY);
}

void Visualize::DefaultWriter::Write(const string &content) const
{
    BinaryBlockHeader header{};
    size_t contentSize = content.size();
    if (contentSize % ALIGN != 0) {
        header.padding = ALIGN - (contentSize % ALIGN);
    }
    header.contentSize = content.size() + header.padding;
    header.type = static_cast<uint8_t>(visualizeType_);
    WriteVisualizeBin(Communication::Serialize(header),
                      content + string(header.padding, 0));
}

void Visualize::CodeWriter::Write(const string &content, const string &codeFile) const
{
    BinaryBlockHeaderCode header{};
    size_t contentSize = content.size();
    if (contentSize % ALIGN != 0) {
        header.binaryBlockHeader.padding = ALIGN - (contentSize % ALIGN);
    }
    header.binaryBlockHeader.contentSize = content.size() + header.binaryBlockHeader.padding;
    header.binaryBlockHeader.type = static_cast<uint8_t>(visualizeType_);
    if (strcpy_s(header.filePath, sizeof(header.filePath), codeFile.c_str()) != 0) {
        LogWarn("File path for %s copy failed", VISUALIZE_DATA_BIN);
    }
    WriteVisualizeBin(Communication::Serialize(header),
                      content + string(header.binaryBlockHeader.padding, 0));
}

void Visualize::InstrApiWriter::Write(const string &content) const
{
    BinaryBlockHeader header{};
    size_t contentSize = content.size();
    if (contentSize % ALIGN != 0) {
        header.padding = ALIGN - (contentSize % ALIGN);
    }
    header.contentSize = content.size() + header.padding;
    header.type = static_cast<uint8_t>(visualizeType_);
    // 0: use "AscendC Inner Code" to map pc to code_line, "L2Cache Hit Rate" fill with float(string before)
    constexpr uint8_t version = 0;
    header.version = version;

    WriteVisualizeBin(Communication::Serialize(header),
                      content + string(header.padding, 0));
}
} // namespace Utility