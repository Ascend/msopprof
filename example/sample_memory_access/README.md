# VectorAdd 内存访问优化案例

## 概述

本样例演示如何使用 **msOpProf** 工具定位 UB-GM 之间数据搬运效率低的性能瓶颈，并通过增大 `TILE_LENGTH`（单次搬运粒度）来减少搬运次数、提升带宽利用率。

## 适用场景

- 算子端到端耗时高，`TILE_LENGTH` 远小于 UB 单次可容纳的最大元素数。
- UB-GM 之间搬运次数过多、单次搬运量过小，带宽利用率低。

## 支持的产品范围

- Ascend 950PR/Ascend 950DT
- Atlas A3 训练系列产品/Atlas A3 推理系列产品
- Atlas A2 训练系列产品/Atlas A2 推理系列产品

## 目录结构

```text
├── sample_memory_access/
│   ├── CMakeLists.txt              // 编译工程文件
│   ├── memory_access.asc           // Ascend C 算子实现（优化前版本）
│   ├── optimize.diff               // 优化前->优化后的 patch 文件
│   └── README.md                   // 本说明文件
```

## 样例描述

### 核心逻辑

本样例实现向量加法算子 `y[i] = x[i] + bias[i]`，数据规模为 80M 个 half 元素（160 MB），8 核并行处理。算子内部按 `TILE_LENGTH` 粒度循环执行 DataCopy→Add→DataCopy。

每个核分配 3 个 UB buffer（inQueueX、inQueueBias、outQueueY），每个 buffer 大小为 `TILE_LENGTH * sizeof(half)` 字节，总 UB 占用 = `3 * TILE_LENGTH * 2` 字节。

> **说明**：本案例重在性能优化演示，不包含标杆数据生成和精度校验步骤，数据规模可以核间均分，不涉及尾核处理。示例代码省略了 ACL Runtime API（如 aclInit、aclrtMalloc、aclrtSynchronizeStream）的返回值检查，生产环境应补充错误处理与资源释放。

### 优化改动

优化前 `TILE_LENGTH = 2048`，每次搬运仅 4KB，每核需循环 5120 次 TILE；优化后 `TILE_LENGTH = 20480`，每次搬运 40KB，每核仅需循环 512 次 tile，单次搬运量大幅增加，减少搬运次数。

完整改动见 `optimize.diff`，仅修改一行常量定义：

```diff
-constexpr uint32_t TILE_LENGTH = 2048;
+constexpr uint32_t TILE_LENGTH = 20480;
```

## 编译运行

### 环境准备

请参照官方文档完成开发环境配置：[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

### 编译算子

在 `sample_memory_access/` 目录下执行：

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
| `OpBasicInfo.csv` | `Task Duration(us)` | 记录端到端耗时作为基线 |
| `OpBasicInfo.csv` | `Block Dim` | 确认多核已启用 |
| `OpBasicInfo.csv` | `Current Freq / Rated Freq` | 排除降频干扰 |
| `PipeUtilization.csv` | `aiv_time(us)` | 单核 AIV 执行时间 |
| `PipeUtilization.csv` | `aiv_mte2_ratio` | MTE2（GM→UB）占比 |
| `PipeUtilization.csv` | `aiv_mte3_ratio` | MTE3（UB→GM）占比 |
| `PipeUtilization.csv` | `aiv_vec_ratio` | Vector 指令 cycle 占比 |
| `Memory.csv` | `aiv_mte2_instructions` | MTE2 指令条数 |
| `Memory.csv` | `aiv_mte3_instructions` | MTE3 指令条数 |
| `Memory.csv` | `GM_to_UB_datas(KB)` | GM→UB 总搬运量 |
| `Memory.csv` | `UB_to_GM_datas(KB)` | UB→GM 总搬运量 |
| `Memory.csv` | `GM_to_UB_bw_usage_rate(%)` | GM→UB 带宽利用率 |
| `Memory.csv` | `UB_to_GM_bw_usage_rate(%)` | UB→GM 带宽利用率 |
| `Memory.csv` | `aiv_gm_to_ub_bw(GB/s)` | GM→UB 实际带宽 |
| `Memory.csv` | `aiv_ub_to_gm_bw(GB/s)` | UB→GM 实际带宽 |

**实测优化前关键指标**：

| 指标 | 优化前实测值 | 说明 |
|------|------------|------|
| `Task Duration(us)` | 2049.78 | 端到端耗时高 |
| `Block Dim` | 8 | 8核并行 |
| `Current Freq / Rated Freq` | 1650 / 1650 | 满频运行，无降频 |
| `aiv_time(us)`（单核） | 2006.67 ~ 2049.09 | 各核执行时间 |
| `aiv_mte2_ratio` | 86.04% ~ 86.33% | MTE2（GM→UB）占比高，每次仅搬运 4KB |
| `aiv_mte3_ratio` | 38.87% ~ 40.75% | MTE3（UB→GM）占比 |
| `aiv_vec_ratio` | 10.15% ~ 10.36% | Vector 计算占比低 |
| `aiv_mte2_instructions`（单核） | 10241 | MTE2 指令条数多 |
| `aiv_mte3_instructions`（单核） | 5121 | MTE3 指令条数多 |
| `GM_to_UB_datas(KB)` | 40960.00 | GM→UB 总搬运量（单核） |
| `UB_to_GM_datas(KB)` | 20480.00 | UB→GM 总搬运量（单核） |
| `GM_to_UB_bw_usage_rate(%)` | 8.66% ~ 8.85% | GM→UB 带宽利用率低 |
| `UB_to_GM_bw_usage_rate(%)` | 5.10% ~ 5.21% | UB→GM 带宽利用率低 |
| `aiv_gm_to_ub_bw(GB/s)` | 19.06 ~ 19.47 | GM→UB 实际带宽低 |
| `aiv_ub_to_gm_bw(GB/s)` | 9.53 ~ 9.73 | UB→GM 实际带宽低 |

分析结论：`TILE_LENGTH = 2048` 导致每核需执行 5120 次 tile 循环（MTE2 指令 10241 条/核），每次 DataCopy 仅搬运 4KB 数据，实际有效带宽利用率低（GM→UB 仅 8.66%~8.85%，UB→GM 仅 5.10%~5.21%）。

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
| `Task Duration(us)` | 2049.78 | 892.02 | **2.30x 加速** |
| `Block Dim` | 8 | 8 | 核数不变 |
| `Current Freq / Rated Freq` | 1650 / 1650 | 1650 / 1650 | 满频运行，无降频 |
| `aiv_time(us)`（单核） | 2006.67 ~ 2049.09 | 771.19 ~ 891.39 | 单核时间降至约 40% |
| `aiv_mte2_ratio` | 86.04% ~ 86.33% | 84.70% ~ 86.76% | MTE2 占比基本不变 |
| `aiv_mte3_ratio` | 38.87% ~ 40.75% | 24.92% ~ 29.29% | MTE3 占比下降，大粒度搬运效率提升 |
| `aiv_vec_ratio` | 10.15% ~ 10.36% | 12.33% ~ 14.10% | Vector 占比提升 |
| `aiv_mte2_instructions`（单核） | 10241 | 1025 | GM→UB 指令次数减少约 10x |
| `aiv_mte3_instructions`（单核） | 5121 | 513 | UB→GM 指令次数减少约 10x |
| `GM_to_UB_datas(KB)` | 40960.00 | 40960.00 | 总搬运量不变 |
| `UB_to_GM_datas(KB)` | 20480.00 | 20480.00 | 总搬运量不变 |
| `GM_to_UB_bw_usage_rate(%)` | 8.66% ~ 8.85% | 19.91% ~ 23.02% | GM→UB 带宽利用率提升约 2.3x |
| `UB_to_GM_bw_usage_rate(%)` | 5.10% ~ 5.21% | 11.73% ~ 13.56% | UB→GM 带宽利用率提升约 2.3x |
| `aiv_gm_to_ub_bw(GB/s)` | 19.06 ~ 19.47 | 43.82 ~ 50.65 | GM→UB 实际带宽提升约 2.3x |
| `aiv_ub_to_gm_bw(GB/s)` | 9.53 ~ 9.73 | 21.91 ~ 25.33 | UB→GM 实际带宽提升约 2.3x |

#### 恢复源码

```bash
patch -R -p1 < optimize.diff
```

### 性能调优总结

| 优化前现象 | 调优动作 | 原因 |
|-----------|---------|------|
| UB-GM 搬运次数多、单次搬运量小、带宽利用率低 | 将 `TILE_LENGTH` 从 2048 增大至 20480（满足 UB 容量约束） | 大粒度搬运减少 DataCopy 调用次数，摊薄启动开销，提升有效带宽利用率 |

### 注意事项

1. 上板功能需要真实 NPU 环境，编译出的二进制应在目标设备上运行。
2. 本案例环境为 Ascend 910B4 + CANN 9.1.0-beta.3，数据因环境不同可能会有差异，属于正常现象。
