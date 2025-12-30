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


#ifndef __PMU_CALCULATOR_H__
#define __PMU_CALCULATOR_H__

#include "data_visualize_const.h"
#include "basic_op_and_pmu.h"

namespace Visualize {

// 公共计算类
class PmuCalculator {
public:
    explicit PmuCalculator() = default;
    std::map<std::string, std::uint64_t> GetBasicPmu(const MemMapDetail &memMapDetail) const;

    float CalculatePer(std::uint64_t value1, std::uint64_t value2) const;
    std::map<std::string, std::uint64_t> GetCycleMap(const std::string &opType, MemMapDetail &detail) const;
    std::map<std::string, std::vector<std::string>> GetTableLineAiCore() { return tableLineAiCore_; };
    std::map<std::string, std::vector<MemInfoPipe>> GetMemInfoPipeMap() { return memInfoPipeMap_; };
    virtual void GetMaxBwRateByGmType(std::map<std::string, std::map<std::string, float>> &maxBwRate) const {};
    virtual std::map<std::string, std::map<std::string, float>> GetBandWidthByWeight(std::uint64_t l0aDatas,
        std::uint64_t l0bDatas, std::uint64_t l0cToGmDatas = 0, std::uint64_t l0cToL1Datas = 0) const { return {}; };
    virtual std::map<std::string, float> GetPipeBwMap(const std::string &socVersion) { return {}; };
    void Init(std::shared_ptr<BasicPmu> &basicPmuObj);

protected:
    const std::vector<std::string> vectorMem_ = {"Vector Write UB", "Vector Read UB"};
    const std::vector<std::string> gm_ = {"Read Main Memory", "Write Main Memory"};
    const std::vector<std::string> pipeCube_ = {"MTE1", "MTE2", "MTE3", "FIXP", "Scalar"};
    const std::vector<std::string> pipeVec_ = {"MTE2", "MTE3", "Scalar"};
    const std::map<std::string, std::vector<std::string>> cubeTable_ = {
        {"L1", {"L1 Read MTE", "L1 Write MTE", "L1 Write L0A/L0B", "L1 Read L0C", "L1 Write GM",
            "L1 Read GM"}},
        {"L0A", {"L0A Read MTE", "L0A Write Cube", "L0A Read L1/GM"}},
        {"L0B", {"L0B Read MTE", "L0B Write Cube", "L0B Read L1/GM"}},
        {"Cube", {"Cube Read L0A", "Cube Read L0B", "Cube Write L0C", "Cube Read L0C"}},
        {"L0C", {"L0C Read Cube", "L0C Write Cube", "L0C Write GM", "L0C Write L1"}}
    };

    std::map<std::string, std::vector<MemInfoPipe>> memInfoPipeMap_;
    std::map<std::string, std::vector<std::string>> tableLineAiCore_;

private:
    virtual void LoadLineMap(const std::string &opType) {};
    virtual void SetMemInfoPipeMap(const std::string &opType, MemMapDetail &memMapDetail) {};
};

class PmuCalculator910B : public PmuCalculator {
public:
    explicit PmuCalculator910B() = default;

    std::map<std::string, std::map<std::string, float>> GetBandWidthByWeight(std::uint64_t l0aDatas,
        std::uint64_t l0bDatas, std::uint64_t l0cToGmDatas = 0, std::uint64_t l0cToL1Datas = 0) const override;
    void GetMaxBwRateByGmType(std::map<std::string, std::map<std::string, float>> &maxBwRate) const override;
private:
    void LoadLineMap(const std::string &opType) override;
    void SetMemInfoPipeMap(const std::string &opType, MemMapDetail &memMapDetail) override;
    void AddBasicPmu910B(const std::string &opType, MemMapDetail &memMapDetail,
                                        std::map<std::string, std::uint64_t> &basicPmu) const;
    void SetMemInfo910B(const std::string &opType, std::map<std::string, std::uint64_t> &basicPmu);
    void TableSplit(std::map<std::string, std::uint64_t> &basicPmu);
    std::map<std::string, std::uint64_t> GetDataCube910B(std::map<std::uint64_t, std::uint64_t> &eventMap) const;
    std::map<std::string, std::uint64_t> GetDataVector910BMix(MemMapDetail &memMapDetail) const;
    float GetDurCalBandWidth(std::unique_ptr<OpBasicInfo> &opBasicInfoObj) const;
};

class PmuCalculator310P : public PmuCalculator {
public:
    explicit PmuCalculator310P() = default;
private:
    void LoadLineMap(const std::string &opType) override;
    void SetMemInfoPipeMap(const std::string &opType, MemMapDetail &memMapDetail) override;
};

class PmuCalculatorA5 : public PmuCalculator {
public:
    explicit PmuCalculatorA5() = default;
private:
    void LoadLineMap(const std::string &opType) override {};
    void SetMemInfoPipeMap(const std::string &opType, MemMapDetail &memMapDetail) override {};
    std::map<std::string, float> GetPipeBwMap(const std::string &socVersion) override;
};

}
#endif // __PMU_CALCULATOR_H__
