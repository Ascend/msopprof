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

#ifndef __OP_AND_PMU_H__
#define __OP_AND_PMU_H__

#include "json.hpp"
#include "data_visualize_const.h"
#include "profiling/device/data_parse/metric_data_handler.h"


namespace Visualize {

class BasicPmu {
public:
    explicit BasicPmu(std::unique_ptr<Profiling::DataHandler> &handler)
    {
        computeLoadBlockDetailVec_ = handler->GetComputeLoadBlockDetailVec();
        MemMapDetails_ = handler->GetMemMapDetail();
        blockIdCoreIdPairVec_ = handler->GetBlockIdCoreIdPairVec();
        totalPmuData_ = handler->GetTotalPmuData();
        l2CacheEvict_ = handler->GetL2CacheEvict();
    }
    // 计算负载类接口
    inline std::vector<ComputeLoadBlockDetail> GetComputeLoadBlockDetail() const
    {
        return computeLoadBlockDetailVec_;
    }
    // 存储访问类接口
    inline std::vector<MemMapDetail> GetMemMapDetails() const { return MemMapDetails_; }
    // Occupancy类接口
    std::vector<std::pair<uint16_t, uint16_t>> GetBlockIdCoreIdPairVec() const { return blockIdCoreIdPairVec_; }

    // roofline类的接口
    inline int64_t GetL2cacheEvict() { return l2CacheEvict_; };
    Profiling::BlockPmuMapType GetTotalPmuData() const { return totalPmuData_; }

    // 计算负载类的常规数据,被计算负载类的方法调用
    const std::vector<DetailInfo> detailInfoVecAic_ = {
        {"Cube All Active", UnitType::PER, 0 },
        {"Cube FP", UnitType::PER, 0},
        {"Cube INT", UnitType::PER, 0},
        {"Cube Wait", UnitType::PER, 0},
        {"Cube All Active", UnitType::INSTR, 1},
        {"Cube FP", UnitType::INSTR, 1},
        {"Cube INT", UnitType::INSTR, 1},
    };
    const std::vector<DetailInfo> detailInfoVecAiv_ = {
        {"Vector All Active", UnitType::PER, 0},
        {"Vector All Active", UnitType::INSTR, 1},
        {"Vector S32", UnitType::INSTR, 1},
        {"Vector FP32", UnitType::INSTR, 1},
        {"Vector FP16", UnitType::INSTR, 1},
        {"Vector S16", UnitType::INSTR, 1},
        {"Vector Misc", UnitType::INSTR, 1},
        {"Vector Fusion", UnitType::INSTR, 1},
        {"Vector DB Para", UnitType::INSTR, 1},
        {"Vector All Block", UnitType::INSTR, 1},
        {"Vector Bank Group Conflict Block", UnitType::INSTR, 1},
        {"Vector Bank Conflict Block", UnitType::INSTR, 1},
        {"Vector VALU Resource Conflict Block", UnitType::INSTR, 1},
        {"Vector MTE Urgent Request Block", UnitType::INSTR, 1},
        {"Vector Wait", UnitType::US, 1},
    };

private:
    // StorageAccess的数据，310P 910B有差异，多态实现
    std::vector<ComputeLoadBlockDetail> computeLoadBlockDetailVec_; // 计算负载类需要(Set)的数据
    std::vector<MemMapDetail> MemMapDetails_; // 存储访问需要(Set)的数据
    Profiling::BlockPmuMapType totalPmuData_; // roofline需要Set的数据
    int64_t l2CacheEvict_ = -1; // roofline和内存热力图需要L2cache evict
    std::vector<std::pair<uint16_t, uint16_t>> blockIdCoreIdPairVec_; // occupancy需要Set的数据
};

class OpBasicInfo {
public:
    explicit OpBasicInfo(std::unique_ptr<Profiling::DataHandler> &handler) : handler_(handler)
    {
        SetOpName();
        SetSoc();
        SetOpType();
        SetBlockDim();
        SetMixBlockDim();
        SetDeviceId();
        SetPid();
        SetCurFreq();
        SetRatedFreq();
        SetDuration();
        SetBlockDetail();
    }

    inline std::string GetOpName() const { return opName_; }
    inline std::string GetSoc() const { return soc_; }
    inline std::string GetOpType() const { return opType_; }
    inline std::string GetBlockDim() const { return blockDim_; }
    inline std::string GetMixBlockDim() const { return mixBlockDim_; }
    inline std::string GetDeviceId() { return deviceId_; }
    inline std::string GetPid() { return pid_; }
    inline std::string GetCurFreq() { return curFreq_; }
    inline std::string GetRatedFreq() { return ratedFreq_; }
    inline float GetDuration() const { return duration_; }
    inline std::map<uint16_t, std::vector<float>> GetBlockDetail() const { return blockDetailMap_; }
    inline std::vector<std::string> GetAdvice() const { return advice_; }

    inline void SetOpName() { opName_ = handler_->GetOpName(); }
    inline void SetSoc() { soc_ = handler_->GetSoc(); }
    inline void SetOpType() { opType_ = handler_->GetOpType(); }
    inline void SetBlockDim() { blockDim_ = handler_->GetBlockDim(); }
    inline void SetMixBlockDim() { mixBlockDim_ = handler_->GetMixBlockDim(); }
    inline void SetDeviceId() { deviceId_ = handler_->GetDeviceId(); }
    inline void SetPid() { pid_ = handler_->GetPid(); }
    inline void SetCurFreq() { curFreq_ = handler_->GetCurFreq(); }
    inline void SetRatedFreq() { ratedFreq_ = handler_->GetRatedFreq(); }
    inline void SetDuration() { duration_ = handler_->GetDuration(); }

    inline void SetBlockDetail()
    {
        blockDetailMap_ = std::move(handler_->GetBlockDetailMap());
    }
    inline void SetAdvice(std::vector<std::string> advice) { advice_ = std::move(advice); }
    void ClearOpBasicInfo();
    void OpBasicInfoToJson();
    void ClearOpBasicFileJson();
    void ShowOpBasicInfo();
    nlohmann::json opBasicFileJson_;
    std::vector<nlohmann::json> GenFreAdvice();
    std::vector<std::string> GetBlockDetailHeader(bool is310P, bool isMix) const;

private:
    std::unique_ptr<Profiling::DataHandler> &handler_;
    std::string opName_;
    std::string soc_;
    std::string opType_;
    std::string blockDim_;
    std::string mixBlockDim_;   // mix_block_dim for mix op
    std::string deviceId_;
    std::string pid_;
    std::string curFreq_;
    std::string ratedFreq_;
    float duration_;
    std::map<uint16_t, std::vector<float>> blockDetailMap_;
    std::vector<std::string> advice_;
};
}
#endif // __OP_AND_PMU_H__