# VectorAdd 核间负载均衡优化案例

## 概述

本样例演示如何使用 **msOpProf** 工具定位多核算力未充分利用的性能瓶颈，并通过核间负载均衡优化实现多核并行加速。

## 适用场景

- 算子端到端耗时高， `Block Dim` 为 1 或远小于设备可用核数，多核算力未充分利用。
- 数据搬运带宽未能通过多核充分利用。

## 支持的产品范围

- Ascend 950PR/Ascend 950DT
- Atlas A3 训练系列产品/Atlas A3 推理系列产品
- Atlas A2 训练系列产品/Atlas A2 推理系列产品

## 目录结构

```text
├── sample_inter_core_balance/
│   ├── CMakeLists.txt              // 编译工程文件
│   ├── inter_core_balance.asc      // Ascend C 算子实现（优化前版本）
│   ├── optimize.diff               // 优化前->优化后的 patch 文件
│   └── README.md                   // 本说明文件
```

## 样例描述

### 核心逻辑

本样例实现向量加法算子 `y[i] = x[i] + bias[i]`，数据规模为 80M 个 half 元素（160 MB）。算子内部按 `GetBlockIdx()` 和 `BLOCK_DIM` 将数据均分到各核，每核独立处理 `TOTAL_LENGTH / BLOCK_DIM` 个元素。

每个核的处理流程为：从 Global Memory 搬入（DataCopy）-> 向量加法（Add）-> 写回 Global Memory（DataCopy），按 TILE_LENGTH 粒度（2048 个 half）循环执行。
> **说明**：本案例重在性能优化演示，不包含标杆数据生成和精度校验步骤，数据规模可以核间均分，不涉及尾核处理。示例代码省略了 ACL Runtime API（如 aclInit、aclrtMalloc、aclrtSynchronizeStream）的返回值检查，生产环境应补充错误处理与资源释放。

### 优化改动

优化前 `BLOCK_DIM = 1`，单核串行处理全部 80M 元素；优化后 `BLOCK_DIM = 8`，8 核均分数据并行处理，每核仅处理 10M 元素。

完整改动见 `optimize.diff`，仅修改一行常量定义：

```diff
-constexpr uint32_t BLOCK_DIM = 1;
+constexpr uint32_t BLOCK_DIM = 8;
```

## 编译运行

### 环境准备

请参照官方文档完成开发环境配置：[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

### 编译算子

在 `sample_inter_core_balance/` 目录下执行：

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
|------|---------|---------|
| `OpBasicInfo.csv` | `Task Duration(us)` | 记录端到端耗时作为基线 |
| `OpBasicInfo.csv` | `Block Dim` | 若为 1 或远小于可用核数，说明多核算力未利用 |
| `OpBasicInfo.csv` | `Current Freq` / `Rated Freq` | 两者应接近或一致，否则存在降频 |
| `PipeUtilization.csv` | `aiv_time(us)` | 单核 AIV 执行时间，应接近 `Task Duration` |
| `PipeUtilization.csv` | `aiv_vec_ratio` | Vector 指令 cycle 占比 |
| `PipeUtilization.csv` | `aiv_mte2_ratio` | MTE2（GM->UB）占比 |
| `PipeUtilization.csv` | `aiv_mte3_ratio` | MTE3（UB->GM）占比 |
| `Memory.csv` | `GM_to_UB_bw_usage_rate(%)` | GM->UB 带宽利用率 |
| `Memory.csv` | `UB_to_GM_bw_usage_rate(%)` | UB->GM 带宽利用率 |

**实测优化前关键指标**：

| 指标 | 优化前实测值 | 说明 |
|------|------------|------|
| `Task Duration(us)` | 14975.22 | 端到端耗时高 |
| `Block Dim` | 1 | 仅使用 1 个核，其余核空闲 |
| `Current Freq / Rated Freq` | 1650 / 1650 | 满频运行，无降频 |
| `aiv_time(us)` | 14974.34 | 单核承担全部计算和搬运，接近端到端耗时 |
| `aiv_vec_ratio` | 11.11% | Vector 计算占比低 |
| `aiv_mte2_ratio` | 85.04% | MTE2（GM->UB）占比较高 |
| `aiv_mte3_ratio` | 39.56% | MTE3（UB->GM）占比，约为MTE2一半 |
| `GM_to_UB_bw_usage_rate(%)` | 9.48% | GM->UB带宽利用率低 |
| `UB_to_GM_bw_usage_rate(%)` | 5.59% | UB->GM带宽利用率低 |

分析结论：`Block Dim = 1` 表明只启动了 1 个 AIV 核，80M 元素的全部搬运和计算由单核串行完成，设备上其余核完全空闲，端到端时间受单核处理能力和带宽上限约束。

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
| `Task Duration(us)` | 14975.22 | 2041.40 | **7.33x 加速** |
| `Block Dim` | 1 | 8 | 8核并行 |
| `Current Freq / Rated Freq` | 1650 / 1650 | 1650 / 1650 | 满频运行，无降频 |
| `aiv_time(us)`（单核） | 14974.34 | 2004.97 ~ 2040.78 | 每核仅处理 10M 元素，单核时间约为优化前的 1/8 |
| `aiv_vec_ratio` | 11.11% | 10.19% ~ 10.37% | Vector 占比基本不变 |
| `aiv_mte2_ratio` | 85.04% | 86.03% ~ 86.27% | MTE2 占比基本不变 |
| `aiv_mte3_ratio` | 39.56% | 36.74% ~ 39.05% | MTE3 占比基本不变 |
| `GM_to_UB_bw_usage_rate(%)` | 9.48% | 8.70% ~ 8.85% | 单核带宽利用率不变，但 8 核并行总带宽提升近 8 倍 |
| `UB_to_GM_bw_usage_rate(%)` | 5.59% | 5.12% ~ 5.21% | 同上 |

#### 恢复源码

```bash
patch -R -p1 < optimize.diff
```

### 性能调优总结

| 优化前现象 | 调优动作 | 原因 |
|-----------|---------|------|
| 算子端到端耗时高，单核处理全部数据 | 将 `BLOCK_DIM` 从 1 调整为 8 | 算子已按 `GetBlockIdx()` 切分数据，增大核数即可多核并行 |

### 注意事项

1. 上板功能需要真实 NPU 环境，编译出的二进制应在目标设备上运行。
2. 实际加速比低于理想值，主要受设备总带宽限制、多核同步与调度开销等因素共同影响。
3. 本案例环境为Ascend 910B4 + CANN 9.1.0-beta.3，数据因环境不同可能会有差异，属于正常现象。
