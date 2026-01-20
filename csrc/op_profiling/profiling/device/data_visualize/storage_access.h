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


#ifndef __STORAGE_ACCESS_H__
#define __STORAGE_ACCESS_H__

#include "data_visualize_const.h"
#include "basic_op_and_pmu.h"
#include "pmu_calculator.h"

namespace Visualize {

struct StorageCubeTableData {
    std::vector<MemInfoCache> cache;
    std::vector<MemInfoPipe> pipe;
    std::vector<MemInfoAiCore> gm;
    std::vector<MemInfoAiCore> cube;
    std::vector<MemInfoAiCore> l0a;
    std::vector<MemInfoAiCore> l0b;
    std::vector<MemInfoAiCore> l0c;
    std::vector<MemInfoAiCore> l1;
    std::vector<MemInfoAiCore> l2cache;
};
struct StorageVecTableData {
    std::vector<MemInfoCache> cache;
    std::vector<MemInfoPipe> pipe;
    std::vector<MemInfoAiCore> gm;
    std::vector<MemInfoAiCore> ub;
    std::vector<MemInfoAiCore> vec;
    std::vector<MemInfoAiCore> dcache;
    std::vector<MemInfoAiCore> l2cache;
};
class StorageAccess {
public:
    StorageAccess(std::shared_ptr<OpBasicInfo> &opBasicInfoObj, std::shared_ptr<BasicPmu> &basicPmuObj,
                  std::unique_ptr<Visualize::PmuCalculator> &pmuCalculatorObj)
        : opBasicInfoObj_(opBasicInfoObj), basicPmuObj_(basicPmuObj), pmuCalculatorObj_(pmuCalculatorObj) {}

    void StorageAccessToJson(bool isKernelScale, bool isMemoryDetail);
    void ShowAdviceInfo();
    void ClearStorageAccessJson();
    nlohmann::json GetStorageAccessHeatMap() {return StorageAccessHeatMap_;};
    nlohmann::json GetStorageAccessTable() {return StorageAccessTable_;};
    std::map<std::string, float> GetPipeLineRatio() {return pipeLineRatio_;};

    void GenCoreUsageAdvice(const std::string &coreName, float ratio, std::vector<nlohmann::json> &advice);
    virtual std::vector<nlohmann::json> GenPipeAdvice(std::string opType);
    float GetDurCalBandWidth();
    std::vector<nlohmann::json> GetAiCoreMemTableJson(std::map<std::string, uint64_t> &cycMap);

    // 后面移动到计算类中VisualizeBasicPmu310P 910B
    virtual void PreProcess(bool isKernelScale, bool isMemoryDetail) = 0;
    virtual std::map<std::string, std::vector<std::string>> GenTableHead() = 0;
    virtual void LoadLineMap(const std::string &opType) = 0;
    virtual void LoadMapData(const std::string &opType, MemMapDetail &memMapDetail) = 0;
    virtual std::vector<nlohmann::json> GetAiCoreMemUnitJson(const std::string &opType) = 0;
    virtual std::vector<std::string> GetTableValue(const std::string &tableName, size_t colum,
        std::map<std::string, uint64_t> &cycMap, const std::map<std::string, std::vector<std::string>> &head) = 0;

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

    // PipeMapForGetTableValue
    std::map<std::string, std::string> pipeMap_ = {{"Pipe Cube", "Cube"}, {"Pipe Vector Core0", "Vector"},
                                                   {"Pipe Vector Core1", "Vector1"}};

    // HeadForGetTableHead
    std::map<std::string, std::vector<std::string>> head_ = { {"Default", {"", "requests", "throughput(GB/s)"}},
        {"Cache", {"", "hit", "miss", "total", "hit rate(%)"}},
        {"Pipe", {"", "instructions", "cycle", "wait cycles", "active rate(%)", "active bw(GB/s)"}},
        {"Pipe Cube", {"", "instructions", "cycle", "wait cycles", "active rate(%)", "active bw(GB/s)"}},
        {"Pipe Vector Core0", {"", "instructions", "cycle", "wait cycles", "active rate(%)", "active bw(GB/s)"}},
        {"Pipe Vector Core1", {"", "instructions", "cycle", "wait cycles", "active rate(%)", "active bw(GB/s)"}}};

    nlohmann::json StorageAccessHeatMap_;
    nlohmann::json StorageAccessTable_;

protected:
    std::shared_ptr<OpBasicInfo> &opBasicInfoObj_;
    std::shared_ptr<BasicPmu> &basicPmuObj_;
    std::unique_ptr<Visualize::PmuCalculator> &pmuCalculatorObj_;

    std::vector<nlohmann::json> heatMapJson_;
    std::vector<nlohmann::json> mapTableJson_;

    std::map<std::string, float> peakMap_;
    std::set<std::string> adviceInfoRecord_;
    std::map<std::string, MemInfoAiCore> ApiDataTransMemInfo_;
    std::map<std::string, std::vector<MemInfoAiCore>> memInfoAiCoreMap_;
    std::map<std::string, std::vector<MemInfoCache>> memInfoCacheMap_;
    std::map<std::string, float> activeRate_;
    std::map<std::string, std::vector<MemInfoPipe>> memInfoPipeMap_;
    std::map<std::string, std::vector<std::string>> tableLineAiCore_;
    bool kernelScale_ {false};
    bool memoryDetail_ {false};
    std::map<std::string, float> pipeLineRatio_ = {};
    // Ideal ratio is 0.8 by default: Throughput/Theoretical bandwidth >= 0.8*[active cycles/op total cycles]
    static constexpr float idealRatio = 0.8f;
};

class StorageAccess910B : public StorageAccess {
public:
    StorageAccess910B(std::shared_ptr<OpBasicInfo> &opBasicInfoObj, std::shared_ptr<BasicPmu> &basicPmuObj,
                      std::unique_ptr<Visualize::PmuCalculator> &pmuCalculatorObj)
        : StorageAccess(opBasicInfoObj, basicPmuObj, pmuCalculatorObj)
        {}
    void GetPipePeak(std::map<std::string, uint64_t> basicPmu, MemMapDetail &memMapDetail, const std::string &opType,
                     Profiling::Calculate &cal);
    void PreProcess(bool isKernelScale, bool isMemoryDetail) override;
    // 后面移动到计算类中VisualizeBasicPmu310P 910B
    std::map<std::string, std::vector<std::string>> GenTableHead() override;
    void LoadLineMap(const std::string &opType) override;
    void LoadMapData(const std::string &opType, MemMapDetail &memMapDetail) override;
    std::vector<nlohmann::json> GetAiCoreMemUnitJson(const std::string &opType) override;
    std::vector<std::string> GetTableValue(const std::string &tableName, size_t colum,
        std::map<std::string, uint64_t> &cycMap, const std::map<std::string, std::vector<std::string>> &head) override;

    std::map<std::string, uint64_t> GetDataCube910B(std::map<uint64_t, uint64_t> &eventMap) const;
    std::map<std::string, uint64_t> GetDataVector910BMix(MemMapDetail &memMapDetail) const;
    void AddBasicPmu910B(const std::string &opType, MemMapDetail &memMapDetail,
                         std::map<std::string, uint64_t> &basicPmu) const;
    std::map<std::string, std::vector<MemInfoAiCore>> GetMemInfoAiCoreMapCube910B(std::map<std::string,
        uint64_t> &basicPmu, Profiling::Calculate &cal) const;
    std::map<std::string, std::vector<MemInfoAiCore>> GetMemInfoAiCoreMapVec910B(std::map<std::string,
        uint64_t> &basicPmu, Profiling::Calculate &cal) const;
    std::map<std::string, std::vector<MemInfoAiCore>> GetMemInfoAiCoreMapMix910B(std::map<std::string,
        uint64_t> &basicPmu, Profiling::Calculate &cal) const;
    std::map<std::string, std::string> CalCulateForVecOrCubeBw(const std::map<std::string, uint64_t> &basicPmu,
        const std::string &opType, Profiling::Calculate &cal, std::map<std::string, uint64_t> &dbiRequest) const;
    std::map<std::string, std::string> CalCulateForMixBw(const std::map<std::string, uint64_t> &basicPmu,
        Profiling::Calculate &cal, std::map<std::string, uint64_t> &dbiRequest) const;
private:
    std::map<PipeAll, MemInfoAiCore> GetCubeMap();
    void SetMemInfo910B(const std::string &opType, std::map<std::string, uint64_t> &basicPmu,
        Profiling::Calculate &cal, std::map<std::string, uint64_t> &dbiRequest);
    void TableSplit(std::map<std::string, uint64_t> &basicPmu, Profiling::Calculate &cal,
                    std::map<std::string, uint64_t> &dbiRequest);
    void LoadApiMemInfo(const MemMapDetail &memMapDetail, Profiling::Calculate &cal);
};

class StorageAccess310P : public StorageAccess {
public:
    StorageAccess310P(std::shared_ptr<OpBasicInfo> &opBasicInfoObj, std::shared_ptr<BasicPmu> &basicPmuObj,
                      std::unique_ptr<Visualize::PmuCalculator> &pmuCalculatorObj)
        : StorageAccess(opBasicInfoObj, basicPmuObj, pmuCalculatorObj)
        {}
    void PreProcess(bool isKernelScale, bool isMemoryDetail) override;
    // 后面移动到计算类中VisualizeBasicPmu310P 910B
    std::map<std::string, std::vector<std::string>> GenTableHead() override;
    void LoadLineMap(const std::string &opType) override;
    void LoadMapData(const std::string &opType, MemMapDetail &memMapDetail) override;
    std::vector<nlohmann::json> GetAiCoreMemUnitJson(const std::string &opType) override;
    std::vector<std::string> GetTableValue(const std::string &tableName, size_t colum,
        std::map<std::string, uint64_t> &cycMap, const std::map<std::string, std::vector<std::string>> &head) override;

    std::map<std::string, std::vector<MemInfoAiCore>> GetMemInfoAiCoreMap310P(std::map<std::string,
        uint64_t> &basicPmu, Profiling::Calculate &cal) const;
};

class StorageAccessA5 : public StorageAccess {
public:
    StorageAccessA5(std::shared_ptr<OpBasicInfo> &opBasicInfoObj, std::shared_ptr<BasicPmu> &basicPmuObj,
                    std::unique_ptr<Visualize::PmuCalculator> &pmuCalculatorObj)
        : StorageAccess(opBasicInfoObj, basicPmuObj, pmuCalculatorObj)
        {}
    std::map<std::string, std::vector<std::string>> GenTableHead() override;
    void PreProcess(bool isKernelScale, bool isMemoryDetail) override;
    void LoadLineMap(const std::string &opType) override;
    void LoadMapData(const std::string &opType, MemMapDetail &memMapDetail) override;
    std::vector<nlohmann::json> GetAiCoreMemUnitJson(const std::string &opType) override;
    std::vector<std::string> GetTableValue(const std::string &tableName, size_t colum,
        std::map<std::string, uint64_t> &cycMap, const std::map<std::string, std::vector<std::string>> &head) override;
    std::vector<nlohmann::json> GenPipeAdvice(std::string opType) override;
private:
    StorageCubeTableData LoadCubeData(const std::map<uint64_t, uint64_t> &eventMap, uint64_t totalCycles, int64_t freq, float time);
    std::vector<MemInfoPipe> LoadCubePipeData(std::map<uint64_t, uint64_t> &eventMap, uint64_t totalCycles,
                                              int64_t freq, float time, const std::map<std::string, float> &pipeData);
    StorageVecTableData LoadVecData(const std::map<uint64_t, uint64_t> &eventMap, uint64_t totalCycles, int64_t freq, float time, int subCoreId = 0);
    std::vector<MemInfoPipe> LoadVecPipeData(std::map<uint64_t, uint64_t> &eventMap, uint64_t totalCycles,
                                             int64_t freq, float time, const std::map<std::string, float> &mteData);
    std::vector<MemInfoCache> LoadCacheData(std::map<uint64_t, uint64_t> &eventMap) const;
    std::map<PipeAll, MemInfoAiCore> GetVecMap();
    std::map<PipeAll, MemInfoAiCore> GetCubeMap();
    std::map<PipeAll, MemInfoAiCore> GetMixMap();
    std::map<std::string, uint64_t> GetCycleMap(const std::string &opType, MemMapDetail &detail) const;
    MemInfoAiCore CalAiCore(float time, uint64_t pmu, Profiling::TransportType type, bool calPeak = false);
    std::map<std::string, float> mteBwMap_;
    std::set<std::string> advicePipe_;
    uint64_t fixpToUbVec0_ = 0;
    uint64_t fixpToUbVec1_ = 0;
};
}
#endif // __STORAGE_ACCESS_H__