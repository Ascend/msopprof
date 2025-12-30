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


#include "data_save.h"

#include <fstream>
#include <sys/stat.h>
#include <cstring>
#include <array>
#include <cerrno>

#include "log.h"

namespace Utility {

template <typename DataType>
bool DataSave::Save(const std::string& saveFilePath,
                    const std::vector<std::vector<DataType>> &saveData, bool appendMode) const
{
    std::ofstream file;
    if (appendMode) {
        file.open(saveFilePath, std::ios::app);
    } else {
        file.open(saveFilePath, std::ios::out);
    }
    if (!file.is_open()) {
        LogError("Cannot create file [%s]", saveFilePath.c_str());
        return false;
    }
    for (std::vector<DataType> row : saveData) {
        for (DataType element : row) {
            file << element << ",";
        }
        file << "\n";
    }
    file.close();
    return true;
}

bool DataSave::CsvSave(const std::string &saveFilePath,
                       const std::vector<std::string> &headers, const CsvData &saveCsvData) const
{
    if (!this->Save<std::string>(saveFilePath, {headers}, false)) {
        return false;
    }
    if (!this->Save<std::string>(saveFilePath, saveCsvData, true)) {
        return false;
    }
    int ret = chmod(saveFilePath.c_str(), SAVE_DATA_FILE_AUTHORITY);
    if (ret != 0) {
        std::array<char, 256> err_buf{};
        strerror_r(errno, err_buf.data(), err_buf.size());
        LogError("Chmod [%s] failed, errno: %d, reason: %s", saveFilePath.c_str(), errno, err_buf.data());
        return false;
    }
    LogDebug("Profiling file [%s] saved.", saveFilePath.c_str());
    return true;
};

bool DataSave::CsvSave(const CsvFileStruct &saveFileStruct) const
{
    return this->CsvSave(saveFileStruct.fileName, saveFileStruct.headers, saveFileStruct.data);
};

}
