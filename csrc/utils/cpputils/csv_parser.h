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


#ifndef __MSOPPROF_UTILITY_CSV_PARSER_H__
#define __MSOPPROF_UTILITY_CSV_PARSER_H__

#include <vector>
#include <string>
#include <map>

namespace Utility {
using CsvRow = std::vector<std::string>;
using CsvData = std::vector<CsvRow>;

struct CsvFileStruct {
    std::string fileName;
    std::vector<std::string> headers;
    CsvData data;
    std::map<std::string, int> headerIndex;
    bool valid = false;
};

// CsvParser is designed for decoding Csv file
class CsvParser {
public:
    static CsvRow AddRow(const std::vector<std::string> &columnNames,
                         const std::map<std::string, std::string> &rowData);

    static CsvData GenerateCsvData(const std::vector<std::string> &columnNames,
                                   const std::vector<std::map<std::string, std::string>> &multipleRowData);
};

}

#endif // __MSOPPROF_UTILITY_CSV_PARSER_H__
