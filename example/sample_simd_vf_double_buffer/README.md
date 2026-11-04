# SIMD VF 流水诊断案例

## 案例说明

本案例提供一个数值正确、存在性能瑕疵的融合向量算子，用于仿真流水诊断和优化练习。工程只包含待诊断版本，可调用 `msprof-op-simulator-pipeline-analysis` Skill，从新采集的流水、指令和源码中形成优化建议。

| 项目 | 规格 |
|---|---|
| 输入 / 输出 | x、y、z 均为 `[16,8192]`、half、连续 ND |
| 计算 | `t0 = half(x + y)`，再执行 14 轮 `t = half(half(t * x) + y)` |
| 编程模型 | SIMD 寄存器 VF，单核 |
| 分块 | 六个 20992 元素满块，一个 5120 元素尾块 |
| 输出二进制 | `demo` |

源码派生自 [asc-devkit 融合向量 Double Buffer 样例](https://gitcode.com/cann/asc-devkit/tree/fba3e6925684e20cae884aa86300814e9cc64d05/examples/02_simd_c_api/02_features/01_reg_vector_compute/00_add_double_buffer)，保留原版权声明和 [LICENSE](LICENSE)。

## msagent 使用指导

### 环境准备

请参照 msagent 官方[快速入门](https://gitcode.com/Ascend/msagent/blob/master/docs/zh/quick_start/msagent_quick_start.md)完成安装和 LLM 配置，工具配置见[配置与扩展指南](https://gitcode.com/Ascend/msagent/blob/master/docs/zh/user_guide/configuration-and-extension.md)。算子编译和仿真环境按下文“编译运行”中的官方文档配置。

### 基础采集分析

在本案例目录启动 Operator 子 agent：

```bash
msagent --agent Operator
```

进入会话后，用 `/skills msot-msopprof-operator-profiler` 加载 Skill，再输入以下提示词：

```text
请对当前目录的 add_double_buffer.asc 进行基础采集分析，使用 simulator 模式和 Ascend950PR_9599 仿真器。
按本 README 编译、生成输入，在新目录完整采集并验证精度，不复用历史结果。
结合本次 trace 和指令 CSV 汇总关键数据、主要瓶颈及优化建议，在本次实验目录按固定四段模板保存 basic-analysis-report.md，并记录命令和产物路径。
先交付分析报告，不修改算子源码。
```

### 流水空泡分析

在本案例目录启动 Operator 子 agent：

```bash
msagent --agent Operator
```

进入会话后，用 `/skills msprof-op-simulator-pipeline-analysis` 加载 Skill，再输入以下提示词：

```text
请重新编译当前目录的 add_double_buffer.asc，使用 Ascend950PR_9599 在新目录完整采集仿真数据并验证精度，不复用历史数据或报告。
结合本次 trace、原始 dump 和源码，分析 MTE2、VF/RVEC、MTE3 的空泡、重叠与同步依赖，区分启动、中段和尾部，追溯到具体指令、Buffer 和源码位置。
给出有证据支持的最小优化建议，在本次实验目录按固定四段模板保存 pipeline-analysis-report.md，保留实际命令、采集日志和精度结果。
先交付分析报告，不修改算子源码。
```

## 编译运行

### 环境准备

请参照官方文档完成开发环境配置：[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。仿真环境要求见 [msOpProf simulator 使用指南](../../docs/zh/user_guide/msopprof_simulator_user_guide.md)。

本案例需要 CANN 编译器、`dav-3510` 仿真器和 Python NumPy，不需要真实 NPU。

### 编译算子

在本案例目录下执行：

```bash
cmake -S . -B build
cmake --build build -j2
```

默认架构为 `dav-3510`，运行模式为 `sim`，递推次数为 14，已启用 `-g` 源码映射。

### 算子运行

生成输入，运行算子并采集仿真数据，然后校验精度：

```bash
cd build
python3 ../scripts/gen_data.py
msprof op simulator --soc-version=Ascend950PR_9599 --dump=true --output=profiling ./demo
python3 ../scripts/verify_result.py output/output.bin output/golden.bin
```

示例使用 `Ascend950PR_9599` 仿真器。精度校验成功时输出 `test pass!`。

### 性能分析

在 `build/profiling/OPPROF_*/` 中查看本次采集：

| 产物 | 观察目标 |
|---|---|
| `simulator/core*/trace.json` | 搬入、VF 计算、写回的空泡及重叠 |
| `simulator/core*/*_instr_exe.csv` | 指令耗时、次数和源码关联 |
| `dump/` | 同步指令、Buffer ID 和地址依赖 |

使用单核或汇总流水中的一份进行统计，避免重复计数；仿真时序不作为上板加速比。

### 注意事项

1. 在 `build` 目录生成数据并运行算子，程序从当前目录读取 `input/`，将结果写入 `output/`。
2. 修改 `FUSED_ITERATIONS` 时，生成数据也需指定相同的 `--fused-iterations`。
3. 分析时选择本次新生成的 `OPPROF_*` 目录，避免混用历史数据。
