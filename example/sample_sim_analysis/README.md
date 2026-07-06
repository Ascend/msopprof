# GroupedMatmul 仿真优化分析案例

## 案例说明

| 配置       | AIC:AIV | Workspace slots | Vector buffer | 输出二进制                   |
| -------- | ------: | --------------: | ------------: | ----------------------- |
| `before` |     1:1 |               1 |             1 | `grouped_matmul_before` |
| `after`  |     1:2 |               4 |             2 | `grouped_matmul_after`  |

输入规模：

```text
x             [1024, 1024]       int8    ND
weight        [8, 1024, 8192]    int8    NZ
groupList     [8]                int64   ND, [128] * 8
scale         [8, 8192]          float   ND
perTokenScale [1024]             float   ND
y             [1024, 8192]       half    ND
```

## 编译运行

### 环境准备

请参照官方文档完成开发环境配置：[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

### 编译算子

编译优化前配置：

```bash
./build.sh before dav-3510 sim
```

编译优化后配置：

```bash
./build.sh after dav-3510 sim
```

`build.sh` 参数顺序不敏感：

```text
./build.sh [before|after] [dav-2201|dav-3510] [npu|sim]
```

本案例默认运行模式为 `sim`，默认架构为 `dav-3510`。

### 算子运行

#### 拉起算子

使用仿真器拉起优化前算子：

```bash
cd build/before_sim_dav-3510
python3 scripts/gen_data.py
msopprof simulator --soc-version=Ascend950PR_9599 ./grouped_matmul_before
```

使用仿真器拉起优化后算子：

```bash
cd build/after_sim_dav-3510
python3 scripts/gen_data.py
msopprof simulator --soc-version=Ascend950PR_9599 ./grouped_matmul_after
```

仿真交付件重点查看：

```text
sim_OPPROF_*/simulator/trace.json
sim_OPPROF_*/simulator/core*/*_instr_exe.csv
sim_OPPROF_*/dump/*rvec.IDU.dump
```

当前仿真可能中断，只生成部分日志。此时不要使用全局 trace span、首尾时间差或全局尾部位置做结论，应聚焦同一 core、同一 pipe 上相邻指令事件之间的流水空洞间隔，以及 RVec 阻塞原因。

#### 性能分析报告

本案例的分析顺序是：先拿优化前工程采集仿真 trace，基于局部流水间隔和 RVec 阻塞判断瓶颈；再根据这些信号确定优化动作；最后运行优化后工程，只用于验证前面的判断是否收敛。

拿到优化前工程后重点查看：

| 文件                                             | 观察目标                        |
| ---------------------------------------------- | --------------------------- |
| `sim_OPPROF_*/simulator/trace.json`            | 查看同一 core、同一 pipe 上相邻指令事件间隔 |
| `sim_OPPROF_*/simulator/core*/*_instr_exe.csv` | 按 pipe 和指令排序热点              |
| `sim_OPPROF_*/dump/*rvec.IDU.dump`             | RVec 发射、寄存器、VF 等阻塞原因        |

相邻指令流水空洞按下面口径统计：

```text
gap = next.ts - (prev.ts + prev.dur)
```

优化前局部流水间隔的关键现象如下：

| 局部指标                              |                                                   优化前 | 判断                           |
| --------------------------------- | ----------------------------------------------------: | ---------------------------- |
| 活跃 `veccore` 数                    |                                                     8 | 每个逻辑核只有 1 个 AIV              |
| Vector `MTE2` pipe 空洞 p90         |                                             11.318 us | workspace/GM 到 UB 搬入存在长间隔    |
| Vector `MTE3` pipe 空洞 p50 / p90   |                                     2.809 / 16.299 us | 写回流水存在明显长空洞                  |
| Vector `RVECEX` pipe 空洞 p99       |                                              3.276 us | Vector 计算发射呈 burst，中间有大空洞    |
| Vector `RVECLD` pipe 空洞 p99       |                                              3.275 us | RVec load 也存在相同级别空洞          |
| Vector `RVECST` pipe 空洞 p90 / p99 |                                      1.775 / 3.280 us | RVec store 间隔偏大              |
| Cube `CUBE` pipe 空洞 p50 / p90     |                                      3.297 / 5.570 us | Cube 侧也有等待，但不是单纯 MMAD 连续计算问题 |
| Cube `MTE1` pipe 空洞 p90           |                                              4.875 us | Cube 输入侧存在同步/搬运间隔            |
| RVec `PERF IDU_BLOCK` 总数          |                                                 14641 | RVec 阻塞严重                    |
| 高频阻塞原因                            | `OOO no avail phy vreg`、`VEC dispatch number reached` | 发射和物理寄存器资源压力高                |

从优化前局部间隔可得到结论：Vector 侧不是稳定连续流水，而是“搬入/计算/写回”之间存在明显空洞；单 AIV 上 RVec 发射和物理寄存器压力高，同时 AIC/AIV 通过 `PIPELINE_DEPTH=1` 的单 workspace 槽生产消费，流水覆盖不足。

#### 性能调优总结

根据优化前局部流水信号，调优动作按下面顺序确定：

| 优化前局部信号                                    | 调优动作                          | 原因                                                                       |
| ------------------------------------------ | ----------------------------- | ------------------------------------------------------------------------ |
| RVec IDU 阻塞高，Vector `RVEC*` pipe 出现 us 级空洞 | 将 `AIC:AIV` 从 `1:1` 调整为 `1:2` | `VectorCompute` 已按 `vecCount % taskRation != subBlockIdx` 支持两个 AIV 分担后处理 |
| AIC/AIV 通过一个 workspace 槽生产消费               | 将 `PIPELINE_DEPTH` 从 1 调整为 4  | 环形 workspace 槽更多，AIC 可连续产出多个中间结果                                         |
| Vector `MTE2/MTE3/RVEC*` pipe 间隔明显         | 将 `BUFFER_NUM` 从 1 调整为 2      | Vector 队列开启 double buffer，增加搬运、计算、写回重叠空间                                 |

优化后重新编译运行 `after` 配置并采集 trace，用于验证上述判断：

| 局部指标                              |               优化前 |              优化后 | 变化                  |
| --------------------------------- | ----------------: | ---------------: | ------------------- |
| 活跃 `veccore` 数                    |                 8 |               16 | 每个 AIC 对应两个 AIV     |
| Vector `RVECEX` pipe 空洞 p99       |          3.276 us |         0.015 us | 计算发射空洞明显收敛          |
| Vector `RVECLD` pipe 空洞 p99       |          3.275 us |         0.008 us | load 空洞明显收敛         |
| Vector `RVECST` pipe 空洞 p90 / p99 |  1.775 / 3.280 us | 0.016 / 0.016 us | store 间隔明显收敛        |
| Vector `MTE3` pipe 空洞 p50 / p90   | 2.809 / 16.299 us | 0.010 / 0.010 us | 写回局部流水更紧凑           |
| Vector `MTE2` pipe 空洞 p90         |         11.318 us |        11.254 us | 搬入仍可见长间隔，不作为本轮主要收敛点 |
| RVec `PERF IDU_BLOCK` 总数          |             14641 |              586 | 阻塞大幅下降              |
| 逐周期 `instr blocked` 行数            |             20546 |             1107 | 发射和寄存器压力缓解          |

优化后局部 trace 中 `veccore` 数从 8 增到 16，`WAIT_FLAG_DEV` 数也从 8 增到 16，说明 `__mix__(1,2)` 已生效。RVec 计算、load、store 的局部空洞和 IDU 阻塞均明显收敛。仿真案例的重点是“从局部流水间隔出发”定位问题：先看到 Vector pipe 之间的空洞和 RVec 阻塞，再映射到 AIV 并行度、workspace 槽深和 UB double buffer，而不是从全局 trace 结束时间或最终耗时倒推结论。

#### 注意事项

1. 请在对应 build 目录下执行 `python3 scripts/gen_data.py`，否则算子运行时找不到当前工作目录下的 `input/`。
2. 当前工程仿真时间过长，建议采集5-10分钟后手动终止，仿真可中断，只有部分日志时不要使用全局 trace span、首尾时间差或全局尾部位置做结论。
3. 仿真模式通过 `msopprof simulator` 拉起，以获得 `OPPROF_*` 交付件。
4. `*_instr_exe.csv` 的 `running_time(us)` 是指令行聚合时间，只适合做热点排序，不等价于端到端耗时。
5. 仿真 trace 与上板 msopprof 的指标口径不同，不要混用局部流水间隔和上板 `Task Duration` 做直接等价比较。
6. 每次仿真会生成新的 `OPPROF_*` 目录，分析前请确认使用的是最新一次运行的交付件。
7. 数据因环境不同可能会有差异，属于正常现象
