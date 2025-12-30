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


#ifndef __MSOPPROF_UTILITY_DATA_SAVE_H__
#define __MSOPPROF_UTILITY_DATA_SAVE_H__

#include "data_format.h"
#include "csv_parser.h"

namespace Utility {

class DataSave {
public:
    template <typename DataType>
    bool Save(const std::string &saveFilePath, const std::vector<std::vector<DataType>> &saveData,
              bool appendMode) const;
    bool CsvSave(const std::string &saveFilePath, const std::vector<std::string> &headers,
                 const Utility::CsvData &saveCsvData) const;
    bool CsvSave(const Utility::CsvFileStruct &saveFileStruct) const;
};

}

#endif // __MSOPPROF_UTILITY_DATA_SAVE_H__
