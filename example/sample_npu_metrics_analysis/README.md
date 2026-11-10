# GroupedMatmul 上板优化分析案例

## 案例说明

本案例通过 GroupedMatmul 工程，演示如何结合上板指标定位瓶颈，并提出 AIV 并行、workspace 流水和 Vector 双缓冲等优化建议。

本案例提供两种可独立执行的演示方案：

- [工程手动执行演示](#工程手动执行演示)：编译并运行 `before`、`after` 配置，完成上板指标采集、精度校验和性能对比。
- [msAgent 演示](#msagent-演示)：仅使用 `before` 配置，由 msAgent 从编译开始，完成指标采集、瓶颈分析并输出分析报告和原始数据。

输入规模：

```text
x             [1024, 1024]       int8    ND
weight        [8, 1024, 8192]    int8    NZ
groupList     [8]                int64   ND, [128] * 8
scale         [8, 8192]          float   ND
perTokenScale [1024]             float   ND
y             [1024, 8192]       half    ND
```

## 工程手动执行演示

### 环境准备

请参照官方文档完成开发环境配置：[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

以下使用 `dav-3510`、`npu` 模式和 0 卡，设备编号请按实际环境调整。

### 编译算子

工程提供以下优化前后两套配置：

| 配置 | AIC:AIV | Workspace slots | Vector buffer | 输出二进制 |
|---|---:|---:|---:|---|
| `before` | 1:1 | 1 | 1 | `grouped_matmul_before` |
| `after` | 1:2 | 4 | 2 | `grouped_matmul_after` |

在本案例目录分别编译两套配置：

```bash
./build.sh before dav-3510 npu
./build.sh after dav-3510 npu
```

`build.sh` 参数顺序不敏感：

```text
./build.sh [before|after] [dav-2201|dav-3510] [npu|sim]
```

本案例默认运行模式为 `npu`，默认架构为 `dav-3510`。

### 算子运行

先在 `before` 构建目录中生成输入，通过 `msopprof` 拉起算子，再校验输出：

```bash
cd build/before_npu_dav-3510
python3 scripts/gen_data.py
msopprof ./grouped_matmul_before 0   # 跑在 0 卡上
python3 scripts/verify_result.py output/output.bin output/golden.bin
```

随后切换到同级的 `after` 构建目录，使用相同流程采集和校验：

```bash
cd ../after_npu_dav-3510
python3 scripts/gen_data.py
msopprof ./grouped_matmul_after 0
python3 scripts/verify_result.py output/output.bin output/golden.bin
```

两套配置使用相同的数据生成脚本和固定随机种子，生成一致的输入与 golden。两次精度校验均应输出 `test pass!`。采集时保持相同设备、采集指标和运行设置，分别保留各构建目录中的原始采集数据和算子输出。

### 性能分析

重点查看以下采集产物：

| 文件                          | 观察目标                                |
| --------------------------- | ----------------------------------- |
| `OpBasicInfo.csv`           | 端到端耗时、`Block Dim`、`Mix Block Dim`   |
| `PipeUtilization.csv`       | AIC/AIV 总时间、Cube 有效计算时间、Vector 计算时间 |
| `ResourceConflictRatio.csv` | AIV Vector、MTE2、MTE3 等等待比例          |
| `Memory*.csv`、`L2Cache.csv` | GM/L1/L0/UB 带宽和数据搬运量                |

### 性能对比

将两套配置的采集结果按同一指标对照，结合精度校验结果判断优化效果。以下为示例数据，实际结果以本次采集为准：

| 指标 | before | after | 对比结论 |
|---|---:|---:|---|
| `Task Duration(us)` | 168.109 | 111.989 | 耗时下降约 33.4%，整体加速约 1.50x |
| `Block Dim` / `Mix Block Dim` | 8 / 8 | 8 / 16 | `AIC:AIV=1:2` 生效 |
| AIC 总时间 | 162.093 到 167.380 us | 109.039 到 110.265 us | AIC 被下游牵制的问题缓解 |
| AIC Cube 有效计算时间 | 79.438 us | 79.438 us | 必要矩阵乘计算量未变化 |
| AIC Cube 占比 | 47.5% 到 49.0% | 72.0% 到 72.9% | 总时间缩短后 Cube 占比提高 |
| AIV `vec_time` | 113.058 到 114.710 us | 约 50 到 63 us | 后处理由两个 AIV 分担 |
| AIV `vec_wait_ratio` | 0.83 到 0.91 | 0.22 到 0.29 | Vector 等待下降 |
| AIV `mte2_wait_ratio` | 约 0.73 到 0.80 | 约 0.10 到 0.15 | 搬运等待缓解 |

三项配置同时变化，示例体现的是组合收益；确认各项改动的独立贡献时，应分别进行消融实验。只有两套配置的精度均通过校验，性能对比才有意义。

### 注意事项

1. 上板功能需要真实 NPU 环境，`npu` 模式编译出的二进制应在目标设备上运行。
2. 请在对应 build 目录下执行 `python3 scripts/gen_data.py`，否则算子运行时找不到当前工作目录下的 `input/`。
3. 每次运行会覆盖当前 build 目录下的 `output/output.bin`，请先保存需要保留的输出。
4. 分析时选择与当前二进制和配置对应的采集目录，避免混用采集数据。

## msAgent 演示

### 步骤一：准备环境并启动

请参照[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)配置开发环境，并准备可用的 NPU 设备。参照 msAgent 官方[快速入门](https://gitcode.com/Ascend/msagent/blob/master/docs/zh/quick_start/msagent_quick_start.md)完成安装和 LLM 配置，工具配置见[配置与扩展指南](https://gitcode.com/Ascend/msagent/blob/master/docs/zh/user_guide/configuration-and-extension.md)。

以下使用 `dav-3510`、`npu` 模式和 0 卡，完成“启动 msAgent → 编译、采集并输出交付件 → 查看结果”。

本演示仅使用 `before` 配置，由 msAgent 从源码编译开始完成流程，无需预先执行手动演示或准备构建产物。

在本案例目录启动 Operator 子 agent：

```bash
msagent --agent Operator
```

进入会话后加载上板调优 Skill：

```text
/skills msot-msopprof-operator-profiler
```

### 步骤二：编译工程、采集分析并输出交付件

输入提示词：

```text
请在当前案例目录编译 before 工程，在 0 卡采集上板指标并校验精度，结合源码分析瓶颈、给出优化建议。输出分析报告和关键指标图，保留原始数据及校验结果并列出保存路径，暂不修改源码。
```

msAgent 执行以下流程：

1. 在本案例目录执行 `./build.sh before dav-3510 npu`，生成 `build/before_npu_dav-3510/grouped_matmul_before`。
2. 进入 `build/before_npu_dav-3510`，执行 `python3 scripts/gen_data.py` 生成输入和 golden。
3. 执行 `msopprof ./grouped_matmul_before 0`，采集 `Default` 指标。
4. 执行 `python3 scripts/verify_result.py output/output.bin output/golden.bin`，确认输出 `test pass!`。
5. 分析本次采集数据，输出报告、关键指标图、原始数据、算子输出及精度校验结果。

报告结合 `OpBasicInfo.csv`、`PipeUtilization.csv`、`ResourceConflictRatio.csv`、`Memory*.csv` 和 `L2Cache.csv`，分析 Cube 计算、Vector 后处理、workspace 同步及搬运等待，并给出优化建议。

完成后输出各交付件的保存路径，供下一步查看。

### 步骤三：查看结果

根据步骤二输出的保存路径，查看以下交付件：

| 结果 | 查看内容 |
|---|---|
| `npu-analysis-report.md` | 关键指标、源码依赖、瓶颈判断和优化建议 |
| 关键指标图 | 端到端耗时、AIC/AIV 耗时、Cube 占比和等待比例 |
| 原始采集数据 | 与报告对应的原始 CSV |
| 算子输出与精度结果 | 算子输出、golden 及 `test pass!` 校验结果 |

#### 瓶颈分析

示例指标如下，实际结果以采集数据为准：

| 指标                            |                  采集值 | 判断                   |
| ----------------------------- | -------------------: | -------------------- |
| `Task Duration(us)`           |              168.109 | 结合流水指标判断端到端瓶颈         |
| `Block Dim` / `Mix Block Dim` |                8 / 8 | 每个逻辑核是 `AIC:AIV=1:1` |
| AIC 总时间                       | 162.093 到 167.380 us | AIC 时间接近端到端耗时        |
| AIC Cube 有效计算时间               |            79.438 us | Cube 只占 AIC 总时间约一半   |
| AIC Cube 占比                   |        47.5% 到 49.0% | 不是纯 Cube 算力瓶颈        |
| AIV `vec_time`                | 113.058 到 114.710 us | 单 AIV 后处理链路很重        |
| AIV `vec_wait_ratio`          |          0.83 到 0.91 | Vector 等待比例很高        |
| AIV `mte2_wait_ratio`         |        约 0.73 到 0.80 | GM 到 UB 搬运等待明显       |
| AIV `mte3_wait_ratio`         |        约 0.74 到 0.81 | UB 到 GM 写回等待明显       |

报告结合源码分析 Cube 结果写入 workspace 后，单 AIV 后处理、单 workspace 槽同步以及 Vector 搬运/写回等待对流水的牵制，不能仅凭等待比例判定为带宽瓶颈。

#### 优化建议

根据采集指标和源码依赖，报告可提出以下调优建议：

| 观测现象                               | 调优建议                          | 原因                                                                       |
| ----------------------------------- | ----------------------------- | ------------------------------------------------------------------------ |
| 单 AIV `vec_time` 约 114 us，后处理链路重    | 将 `AIC:AIV` 从 `1:1` 调整为 `1:2` | `VectorCompute` 已按 `vecCount % taskRation != subBlockIdx` 支持两个 AIV 分担后处理 |
| Cube 结果和 Vector 消费通过单 workspace 槽同步 | 将 `PIPELINE_DEPTH` 从 1 调整为 4  | `cubeTaskIdx % PIPELINE_DEPTH` 是环形槽设计，增加槽数可减少生产消费串行                      |
| AIV `mte2/mte3` 等待比例高               | 将 `BUFFER_NUM` 从 1 调整为 2      | Vector 队列开启 double buffer，增加搬运、计算、写回重叠空间                                 |

这些建议供后续调优参考，当前演示的交付件为本次采集数据及分析结果。
