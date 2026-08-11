# VectorAdd 头开销优化案例

## 概述

本样例演示如何使用 **msOpProf** 工具定位算子类中 `TPipe` 作为类成员导致编译器无法对类内 Scalar 变量做常量折叠（Scalar Folding）优化的性能瓶颈，并通过将 `TPipe` 移出类对象、在核函数入口创建并传入指针，使能编译器优化，降低 Scalar 指令耗时。

## 适用场景

- 算子端到端耗时中，Scalar 初始化/指令分发开销占比过高。
- 算子类内部定义了 `TPipe` 对象（作为类成员创建并初始化），编译器对类内 Scalar 变量无法进行常量折叠和常量传播，导致 Scalar 指令不必要增加。
- 小数据量、单核计算量较小的算子。

## 支持的产品范围

- Ascend 950PR/Ascend 950DT
- Atlas A3 训练系列产品/Atlas A3 推理系列产品
- Atlas A2 训练系列产品/Atlas A2 推理系列产品

## 目录结构

```text
├── sample_overhead/
│   ├── CMakeLists.txt              // 编译工程文件
│   ├── overhead.asc                // Ascend C 算子实现（优化前版本）
│   ├── optimize.diff               // 优化前->优化后的 patch 文件
│   └── README.md                   // 本说明文件
```

## 样例描述

### 核心逻辑

本样例实现向量加法算子 `y[i] = x[i] + bias[i]`，数据规模为 5120 个 half 元素（10 KB）。算子内部按 `GetBlockIdx()` 和 `BLOCK_DIM` 将数据均分到各核，每核按 `TILE_LENGTH` 粒度循环执行 DataCopy→Add→DataCopy。

每个核分配 3 个 UB buffer（inQueueX、inQueueBias、outQueueY），每个 buffer 大小为 `TILE_LENGTH * sizeof(half)` 字节，总 UB 占用 = `3 * TILE_LENGTH * 2` 字节。

> **说明**：本案例重在性能优化演示，不包含标杆数据生成和精度校验步骤，数据规模可以核间均分，不涉及尾核处理。示例代码省略了 ACL Runtime API（如 aclInit、aclrtMalloc、aclrtSynchronizeStream）的返回值检查，生产环境应补充错误处理与资源释放。

### 优化改动

`TPipe` 是用来管理全局内存和同步的框架，用户可以调用 `TPipe` 的接口，为 TQue/TBuf 进行内存分配。在编写 Ascend C 算子过程中，经常用一个类存放计算所需的相关变量。

当 `TPipe` 对象在算子类的实现中定义并初始化时，`TPipe` 对象的内存空间在整个算子类对象的内存空间之中；需要注意的是，创建 `TPipe` 对象时，对象初始化会设置全局变量的 `TPipe` 指针，这导致算子类对象的内存有被外部污染的风险，此时编译器的编译优化将采取保守策略，不会对算子类对象中的 Scalar 变量进行常量折叠和常量传播。

因此，建议将 `TPipe` 对象创建于算子类外部，使得 `TPipe` 对象的内存空间独立于算子类对象的内存空间，触发编译器对算子类内 Scalar 的编译优化，减少算子 Scalar 指令耗时。

**优化前**：`TPipe` 对象由 `VectorAddKernel` 类内部创建并初始化，影响编译器 Scalar 折叠优化，在 NPU 侧导致 Scalar 指令不必要增加。

```cpp
class VectorAddKernel {
    ...
private:
    ...
    AscendC::TPipe pipe;    // 类内创建并初始化
    ...
};
```

**优化后**：改为由核函数入口创建 `TPipe` 对象，在 `VectorAddKernel` 类中保存 `TPipe` 指针使用。

```cpp
class VectorAddKernel {
    ...
private:
    ...
    AscendC::TPipe* pipe;   // 类内仅保存指针
    ...
};

__global__ __vector__ void vector_add_custom(GM_ADDR x, GM_ADDR bias, GM_ADDR y)
{
    AscendC::InitSocState();
    AscendC::TPipe pipe;    // 核函数入口创建
    VectorAddKernel op;
    op.Init(x, bias, y, &pipe);
    op.Process();
}
```

完整改动见 `optimize.diff`。

## 编译运行

### 环境准备

请参照官方文档完成开发环境配置：[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

### 编译算子

在 `sample_overhead/` 目录下执行：

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_ASC_ARCHITECTURES=dav-2201
make -j4
cd ..
```

编译完成后，`build` 目录下生成 `demo` 可执行文件。

- 编译选项说明

| 选项 | 可选值 | 说明 |
|------|--------|------|
| `CMAKE_ASC_ARCHITECTURES` | `dav-2201`、`dav-3510` | NPU 架构：dav-2201 对应 Atlas A2/A3 系列产品，dav-3510 对应 Ascend 950PR/Ascend 950DT |

> **注意**：切换架构前需清理 cmake 缓存，可在 build 目录下执行 `rm CMakeCache.txt` 后重新 cmake。

### 算子运行与性能采集

#### 优化前：采集基线数据

```bash
msopprof ./build/demo 0
```

命令执行后在当前目录生成 `OPPROF_{timestamp}_XXX/` 文件夹，包含以下 CSV 文件：

```text
OPPROF_{timestamp}_XXX/
├── OpBasicInfo.csv             // 算子基本信息（名称、核数、总耗时、频率）
├── PipeUtilization.csv         // 各流水线单元耗时和占比
├── ArithmeticUtilization.csv   // Cube/Vector 指令 cycle 占比和计算量
├── Memory.csv                  // 内存读写带宽和数据搬运量
├── MemoryL0.csv                // L0A/L0B/L0C 读写带宽
├── MemoryUB.csv                // UB 读写带宽（Vector/Scalar）
├── L2Cache.csv                 // L2 Cache 命中率
├── ResourceConflictRatio.csv   // Bank conflict 和资源冲突占比
└── visualize_data.bin          // MindStudio Insight 可视化文件
```

#### 优化前：分析性能瓶颈

重点查看以下 CSV 文件中的关键字段：

| 文件 | 关键字段 | 说明 |
|------|---------|------|
| `OpBasicInfo.csv` | `Task Duration(us)` | 端到端耗时 |
| `OpBasicInfo.csv` | `Block Dim` | 核数 |
| `OpBasicInfo.csv` | `Current Freq / Rated Freq` | 排除降频干扰 |
| `PipeUtilization.csv` | `aiv_time(us)` | 单核总时间（含启动开销） |
| `PipeUtilization.csv` | `aiv_scalar_ratio` | Scalar 指令占比 |
| `PipeUtilization.csv` | `aiv_vec_ratio` | Vector 占比 |
| `PipeUtilization.csv` | `aiv_mte2_ratio` | MTE2 占比 |
| `PipeUtilization.csv` | `aiv_mte3_ratio` | MTE3 占比 |
| `PipeUtilization.csv` | `aiv_scalar_time(us)` | Scalar 耗时 |
| `PipeUtilization.csv` | `aiv_vec_time(us)` | VEC 实际计算时间 |
| `PipeUtilization.csv` | `aiv_mte2_time(us)` | MTE2 实际搬运时间 |
| `PipeUtilization.csv` | `aiv_mte3_time(us)` | MTE3 实际搬运时间 |
| `ResourceConflictRatio.csv` | `aiv_vec_wait_ratio` | Vector 等待占比 |
| `ResourceConflictRatio.csv` | `aiv_mte2_wait_ratio` | MTE2 等待占比 |
| `ResourceConflictRatio.csv` | `aiv_mte3_wait_ratio` | MTE3 等待占比 |
| `ResourceConflictRatio.csv` | `aiv_vec_mte_cflt_ratio` | MTE 冲突占比 |
| `Memory.csv` | `GM_to_UB_datas(KB)` | GM→UB 总搬运量 |
| `Memory.csv` | `UB_to_GM_datas(KB)` | UB→GM 总搬运量 |
| `Memory.csv` | `aiv_mte2_instructions` | MTE2 指令条数 |
| `Memory.csv` | `aiv_mte3_instructions` | MTE3 指令条数 |
| `Memory.csv` | `GM_to_UB_bw_usage_rate(%)` | GM→UB 带宽利用率 |
| `Memory.csv` | `UB_to_GM_bw_usage_rate(%)` | UB→GM 带宽利用率 |
| `Memory.csv` | `aiv_gm_to_ub_bw(GB/s)` | GM→UB 实际带宽 |
| `Memory.csv` | `aiv_ub_to_gm_bw(GB/s)` | UB→GM 实际带宽 |

**实测优化前关键指标**：

| 指标 | 优化前实测值 | 说明 |
|------|------------|------|
| `Task Duration(us)` | 5.90 | 端到端耗时 |
| `Block Dim` | 1 | 单核处理全部 5120 个元素 |
| `Current Freq / Rated Freq` | 1650 / 1650 | 满频运行，无降频 |
| `aiv_time(us)` | 5.34 | 单核执行时间 |
| `aiv_scalar_ratio` | 61.85% | Scalar 占比高，头开销占主导 |
| `aiv_vec_ratio` | 5.22% | Vector 计算占比低 |
| `aiv_mte2_ratio` | 55.50% | MTE2 占比高 |
| `aiv_mte3_ratio` | 30.33% | MTE3 占比 |
| `aiv_scalar_time(us)` | 3.30 | Scalar 耗时远超实际计算与搬运之和 |
| `aiv_vec_time(us)` | 0.279 | VEC 计算时间 |
| `aiv_mte2_time(us)` | 2.96 | MTE2 搬运时间 |
| `aiv_mte3_time(us)` | 1.62 | MTE3 搬运时间 |
| `aiv_vec_wait_ratio` | 60.45% | Vector 等待占比 |
| `aiv_mte2_wait_ratio` | 51.68% | MTE2 等待占比 |
| `aiv_mte3_wait_ratio` | 61.35% | MTE3 等待占比 |
| `aiv_vec_mte_cflt_ratio` | 0.00% | 无 MTE 冲突 |
| `GM_to_UB_datas(KB)` | 20.00 | 搬运量 |
| `UB_to_GM_datas(KB)` | 10.00 | 搬运量 |
| `aiv_mte2_instructions` | 21 | MTE2 指令条数 |
| `aiv_mte3_instructions` | 11 | MTE3 指令条数 |
| `GM_to_UB_bw_usage_rate(%)` | 1.62% | 带宽利用率低 |
| `UB_to_GM_bw_usage_rate(%)` | 0.96% | 带宽利用率低 |
| `aiv_gm_to_ub_bw(GB/s)` | 3.57 | 实际带宽低 |
| `aiv_ub_to_gm_bw(GB/s)` | 1.79 | 实际带宽低 |

分析结论：`TPipe` 作为类成员时，编译器无法对类内 Scalar 变量（如 `tilesPerCore`、buffer 地址等）做常量折叠和常量传播，产生大量冗余 Load/Store 指令。`aiv_scalar_ratio` 高达 61.85%，`aiv_scalar_time`（3.30 us）是实际 VEC 计算（0.279 us）的近 12 倍，Scalar 初始化开销在单核执行时间中占绝对主导。

#### 应用优化并重新采集

```bash
# 应用优化 patch
patch -p1 < optimize.diff

# 重新编译（同一 build 目录）
cd build && make -j4 && cd ..

# 采集优化后数据
msopprof ./build/demo 0
```

#### 优化后：对比验证

**实测优化后关键指标**：

| 指标 | 优化前 | 优化后 | 对比结论 |
|------|-------:|-------:|---------|
| `Task Duration(us)` | 5.90 | 5.18 | **12.2% 提升** |
| `Block Dim` | 1 | 1 | 核数不变 |
| `Current Freq / Rated Freq` | 1650 / 1650 | 1650 / 1650 | 满频运行，无降频 |
| `aiv_time(us)` | 5.34 | 4.60 | 单核时间下降 |
| `aiv_scalar_ratio` | 61.85% | 56.03% | **头开销占比降低** |
| `aiv_vec_ratio` | 5.22% | 6.18% | Vector 占比提升 |
| `aiv_mte2_ratio` | 55.50% | 54.53% | MTE2 占比基本不变 |
| `aiv_mte3_ratio` | 30.33% | 35.23% | MTE3 占比上升 |
| `aiv_scalar_time(us)` | 3.30 | 2.58 | Scalar 耗时下降 |
| `aiv_vec_time(us)` | 0.279 | 0.284 | VEC 时间基本不变 |
| `aiv_mte2_time(us)` | 2.96 | 2.51 | MTE2 时间下降 |
| `aiv_mte3_time(us)` | 1.62 | 1.62 | MTE3 时间不变 |
| `aiv_vec_wait_ratio` | 60.45% | 56.70% | Vector 等待占比下降 |
| `aiv_mte2_wait_ratio` | 51.68% | 52.60% | MTE2 等待占比基本不变 |
| `aiv_mte3_wait_ratio` | 61.35% | 69.56% | MTE3 等待占比上升 |
| `aiv_vec_mte_cflt_ratio` | 0.00% | 0.00% | 无 MTE 冲突 |
| `GM_to_UB_datas(KB)` | 20.00 | 20.00 | 搬运量不变 |
| `UB_to_GM_datas(KB)` | 10.00 | 10.00 | 搬运量不变 |
| `aiv_mte2_instructions` | 21 | 21 | MTE2 指令条数不变 |
| `aiv_mte3_instructions` | 11 | 11 | MTE3 指令条数不变 |
| `GM_to_UB_bw_usage_rate(%)` | 1.62% | 1.89% | 带宽利用率提升 |
| `UB_to_GM_bw_usage_rate(%)` | 0.96% | 1.11% | 带宽利用率提升 |
| `aiv_gm_to_ub_bw(GB/s)` | 3.57 | 4.15 | 实际带宽提升 |
| `aiv_ub_to_gm_bw(GB/s)` | 1.79 | 2.07 | 实际带宽提升 |

#### 恢复源码

```bash
patch -R -p1 < optimize.diff
```

### 性能调优总结

| 优化前现象 | 调优动作 | 原因 |
|-----------|---------|------|
| `aiv_scalar_ratio` 高达 61.85%，`aiv_scalar_time`（3.30 us）占 `aiv_time` 约 2/3，是 VEC 实际计算时间的近 12 倍 | 将 `TPipe` 从类成员移出，改为核函数入口创建，类内仅保存 `TPipe*` 指针 | `TPipe` 类成员初始化会设置全局 `TPipe` 指针，使编译器对类内 Scalar 采取保守策略，不做常量折叠；移出后触发编译优化，`aiv_scalar_time` 从 3.30 us 降至 2.58 us（-21.8%），`aiv_scalar_ratio` 从 61.85% 降至 56.03%，`aiv_time` 降低约 13.9% |

### 注意事项

1. 上板功能需要真实 NPU 环境，编译出的二进制应在目标设备上运行。
2. 本案例环境为 Ascend 910B4 + CANN 9.1.0-beta.3，数据因环境不同可能会有差异，属于正常现象。
