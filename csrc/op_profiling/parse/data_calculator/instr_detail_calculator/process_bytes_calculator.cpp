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


#include "process_bytes_calculator.h"
#include "number_operation.h"
namespace Profiling {
namespace Parse {

PluginErrorCode ProcessBytesCalculator::Entry()
{
    std::string location("ProcessBytesCalculator");
    constexpr int defaultProcessByte = 0;
    AttributeMapInit();
    std::shared_ptr<InstrDetailTable> instrDetailTable = dataCenter_.GetDbPtr<InstrDetailTable>();
    if (instrDetailTable == nullptr) {
        return PluginErrorCode::NONBLOCKING_ERROR;
    }
    auto processByteResPtr = Utility::MakeShared<std::vector<int>>(instrDetailTable->GetSize());
    if (processByteResPtr == nullptr) {
        return PluginErrorCode::NONBLOCKING_ERROR;
    }

    for (uint32_t i = 0; i < instrDetailTable->GetSize(); i++) {
        MemOpInfo memOpInfo = {false, 0, 0, 0, 0, 0, 0, 0};
        MergeInfo* mergeInfo = instrDetailTable->QueryColumnValue<MergeInfo>(InstrDetailTable::MERGE_INFO, i);
        if (mergeInfo == nullptr) {
            instrDetailTable->UpdateColumnValue(InstrDetailTable::PROCESS_BYTES, i, defaultProcessByte);
            continue;
        }
        if (!GetMemOpInfo(*mergeInfo, memOpInfo)) {
            instrDetailTable->UpdateColumnValue(InstrDetailTable::PROCESS_BYTES, i, defaultProcessByte);
        } else {
            instrDetailTable->UpdateColumnValue(InstrDetailTable::PROCESS_BYTES, i,
                Utility::SafeMulAll<int>({static_cast<int>(memOpInfo.blockNum), static_cast<int>(memOpInfo.blockSize),
                static_cast<int>(memOpInfo.repeatTimes)}, location));
        }
    }
    return PluginErrorCode::SUCCESS;
}

}
}
