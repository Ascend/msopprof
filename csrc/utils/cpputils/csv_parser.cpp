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

#include <vector>

#include "csv_parser.h"

namespace Utility {
CsvRow CsvParser::AddRow(const std::vector<std::string> &columnNames,
                         const std::map<std::string, std::string> &rowData)
{
    CsvRow csvRow;
    for (const std::string &singleColumnName : columnNames) {
        auto it = rowData.find(singleColumnName);
        if (it == rowData.end()) {
            csvRow.emplace_back("NA");
            continue;
        }
        csvRow.push_back(it->second);
    }
    return csvRow;
}

CsvData CsvParser::GenerateCsvData(const std::vector<std::string> &columnNames,
                                   const std::vector<std::map<std::string, std::string>> &multipleRowData)
{
    CsvData csvData;
    for (const auto &row : multipleRowData) {
        CsvRow csvRow = AddRow(columnNames, row);
        csvData.push_back(csvRow);
    }
    return csvData;
}
}

