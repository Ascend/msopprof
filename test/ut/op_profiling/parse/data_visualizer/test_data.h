#ifndef MSOPT_TEST_DATA_H
#define MSOPT_TEST_DATA_H
#include "parse/data_visualizer/sim_visualizer_config.h"
#include "profiling/simulator/data_parse/sim_defs.h"
#include "filesystem.h"
#include "smart_pointer.h"

using namespace Profiling::Parse;
using namespace Utility;
using namespace Profiling;
namespace Visualize {
static Profiling::Pc2CodeMap pc2code;

inline std::shared_ptr<std::map<std::string, SimData>> GetSimData()
{
    std::map<std::string, SimData> dataMap;
    std::shared_ptr<std::map<std::string, SimData>> dataMapPtr;
    std::shared_ptr<InstrDetailTable> instrPtr;
    std::shared_ptr<CacheDetailTable> cachePtr;
    std::shared_ptr<UserMarkStruct> userMarkPtr;
    MergeInfo m1;
    m1.icacheTick = UINT64_MAX;
    m1.pc = 0x10f86004;
    m1.startTick = 11035;
    m1.endTick = 11036;
    m1.pipe = "VECTOR";
    m1.name = SET_FLAG;
    m1.detail = "PIPE:VEC,TRIGGERPIPE:MTE3,FLAGID:0,";
    MergeInfo m2;
    m2.icacheTick = UINT64_MAX;
    m2.pc = 0x10f86000;
    m2.startTick = 11030;
    m2.endTick = 11036;
    m2.pipe = "MTE3";
    m2.name = WAIT_FLAG;
    m2.detail = "PIPE:VEC,TRIGGERPIPE:MTE3,FLAGID:0,";
    std::vector<MergeInfo> mergeVec {m1, m2};
    InstrDetailTable instr(mergeVec);

    MergeInfo cache;
    cache.pc = 0x10f86008;
    cache.startTick = 11038;
    cache.endTick = 11038;
    cache.name = "0x10f86008";
    cache.pipe = "CACHEMISS";
    std::vector<MergeInfo> cacheVec {cache};
    cachePtr = Utility::MakeShared<CacheDetailTable>(cacheVec);
    instrPtr = Utility::MakeShared<InstrDetailTable>(instr);

    std::map<std::string, std::vector<UserMarkInfo>> userMarkInfos;
    UserMarkInfo uu;
    uu.startTick = 11040;
    uu.endTick = 11050;
    uu.startPc = 0x10f86000;
    uu.endPc = 0x10f86004;
    MergeInfo userMark;
    userMark.pc = 0x10f86000;
    userMark.pipe = "USERMARK";
    userMark.name = "Mark 0x1";
    userMark.startTick = 11040;
    userMark.endTick = 11050;
    std::vector<MergeInfo> userMarkInstrs = {userMark};
    userMarkInfos["Mark 0x1"] = std::vector<UserMarkInfo> {uu};
    UserMarkStruct userMarkStruct {userMarkInfos, userMarkInstrs};
    userMarkPtr = Utility::MakeShared<UserMarkStruct>(userMarkStruct);

    SimData data = {instrPtr, cachePtr, userMarkPtr};
    dataMap["core0.veccore0"] = data;
    dataMapPtr = MakeShared<std::map<std::string, SimData>>(dataMap);
    return dataMapPtr;
}

inline SimVisualizerConfig GetVisualizeConfig(const std::string &outputPath, const ChipProductType &chipType)
{
    pc2code[0x10f86000] = {
            "/Ascend/ascend-toolkit/8.1.RC1/aarch64-linux/tikcpp/tikcfw/impl/dav_c220/kernel_operator_common_impl.h:0",
            "/op_kernel/binary/ascend910b/kernel_meta/AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b_2034756_kernel.cpp:30"};
    pc2code[0x1195e004] ={
            "/Ascend/ascend-toolkit/8.1.RC1/aarch64-linux/tikcpp/tikcfw/impl/dav_c220/kernel_operator_common_impl.h:0",
            "/op_kernel/binary/ascend910b/kernel_meta/AddCustom_1e04ee05ab491cc5ae9c3d5c9ee8950b_2034756_kernel.cpp:60"};

    Profiling::Parse::SimVisualizerConfig config910B{"test/ut/resources/dump/output/910B", pc2code, 1,
                                                     ChipProductType::ASCEND910B1};
    SimVisualizerConfig config {outputPath, pc2code, 1, chipType};
    return config;
}

}
#endif //MSOPT_TEST_DATA_H
