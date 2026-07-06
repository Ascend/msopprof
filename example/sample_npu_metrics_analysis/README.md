# GroupedMatmul 上板优化分析案例

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
./build.sh before dav-3510 npu
```

编译优化后配置：

```bash
./build.sh after dav-3510 npu
```

`build.sh` 参数顺序不敏感：

```text
./build.sh [before|after] [dav-2201|dav-3510] [npu|sim]
```

本案例默认运行模式为 `npu`，默认架构为 `dav-3510`。

### 算子运行

#### 拉起算子

采集上板性能数据时，可使用 `msprof` 拉起算子：

```bash
cd build/before_npu_dav-3510
python3 scripts/gen_data.py
msopprof ./grouped_matmul_before 0   # 跑在 0 卡上

cd ../../build/after_npu_dav-3510
python3 scripts/gen_data.py
msopprof ./grouped_matmul_after 0    # 跑在 0 卡上
```

输出中重点查看 `OpBasicInfo.csv`、`PipeUtilization.csv`、`ResourceConflictRatio.csv`、`Memory*.csv` 和 `L2Cache.csv`。

#### 性能分析报告

优化前重点查看：

| 文件                          | 观察目标                                |
| --------------------------- | ----------------------------------- |
| `OpBasicInfo.csv`           | 端到端耗时、`Block Dim`、`Mix Block Dim`   |
| `PipeUtilization.csv`       | AIC/AIV 总时间、Cube 有效计算时间、Vector 计算时间 |
| `ResourceConflictRatio.csv` | AIV Vector、MTE2、MTE3 等等待比例          |
| `Memory*.csv`、`L2Cache.csv` | GM/L1/L0/UB 带宽和数据搬运量                |

优化前关键指标如下：

| 指标                            |                  优化前 | 判断                   |
| ----------------------------- | -------------------: | -------------------- |
| `Task Duration(us)`           |              168.109 | 基线端到端耗时较高            |
| `Block Dim` / `Mix Block Dim` |                8 / 8 | 每个逻辑核是 `AIC:AIV=1:1` |
| AIC 总时间                       | 162.093 到 167.380 us | AIC 时间接近端到端耗时        |
| AIC Cube 有效计算时间               |            79.438 us | Cube 只占 AIC 总时间约一半   |
| AIC Cube 占比                   |        47.5% 到 49.0% | 不是纯 Cube 算力瓶颈        |
| AIV `vec_time`                | 113.058 到 114.710 us | 单 AIV 后处理链路很重        |
| AIV `vec_wait_ratio`          |          0.83 到 0.91 | Vector 等待比例很高        |
| AIV `mte2_wait_ratio`         |        约 0.73 到 0.80 | GM 到 UB 搬运等待明显       |
| AIV `mte3_wait_ratio`         |        约 0.74 到 0.81 | UB 到 GM 写回等待明显       |

分析结论：主要瓶颈不是矩阵乘 Cube 计算不足，而是 Cube 结果写入 workspace 后，被单 AIV 后处理、单 workspace 槽同步以及 Vector 搬运/写回等待牵制。

#### 性能调优总结

根据优化前指标，调优动作按下面顺序确定：

| 优化前现象                               | 调优动作                          | 原因                                                                       |
| ----------------------------------- | ----------------------------- | ------------------------------------------------------------------------ |
| 单 AIV `vec_time` 约 114 us，后处理链路重    | 将 `AIC:AIV` 从 `1:1` 调整为 `1:2` | `VectorCompute` 已按 `vecCount % taskRation != subBlockIdx` 支持两个 AIV 分担后处理 |
| Cube 结果和 Vector 消费通过单 workspace 槽同步 | 将 `PIPELINE_DEPTH` 从 1 调整为 4  | `cubeTaskIdx % PIPELINE_DEPTH` 是环形槽设计，增加槽数可减少生产消费串行                      |
| AIV `mte2/mte3` 等待比例高               | 将 `BUFFER_NUM` 从 1 调整为 2      | Vector 队列开启 double buffer，增加搬运、计算、写回重叠空间                                 |

优化后重新编译运行 `after` 配置并采集，用于验证上述判断：

| 指标                    |                  优化前 |                  优化后 | 对比结论                   |
| --------------------- | -------------------: | -------------------: | ---------------------- |
| `Task Duration(us)`   |              168.109 |              111.989 | 下降 56.120 us，整体约 1.50x |
| `Mix Block Dim`       |                    8 |                   16 | `AIC:AIV=1:2` 生效       |
| AIC 总时间               | 162.093 到 167.380 us | 109.039 到 110.265 us | AIC 被下游牵制的问题缓解         |
| AIC Cube 有效计算时间       |            79.438 us |            79.438 us | 必要矩阵乘计算量未变化            |
| AIC Cube 占比           |        47.5% 到 49.0% |        72.0% 到 72.9% | 总时间缩短后 Cube 占比提高       |
| AIV `vec_time`        | 113.058 到 114.710 us |         约 50 到 63 us | 后处理由两个 AIV 分担          |
| AIV `vec_wait_ratio`  |          0.83 到 0.91 |          0.22 到 0.29 | Vector 等待显著下降          |
| AIV `mte2_wait_ratio` |        约 0.73 到 0.80 |        约 0.10 到 0.15 | 搬运等待明显缓解               |

#### 注意事项

1. 上板功能需要真实 NPU 环境，`npu` 模式编译出的二进制应在目标设备上运行。
2. 请在对应 build 目录下执行 `python3 scripts/gen_data.py`，否则算子运行时找不到当前工作目录下的 `input/`。
3. 每次运行会覆盖当前 build 目录下的 `output/output.bin`，对比 before/after 时请先保存需要保留的输出。
4. 采集前建议清理旧的 msopprof 输出目录，避免误读历史数据。
5. 继续优化时建议新增消融实验，例如只改 AIV 数、只改 pipeline depth、只改 buffer 数，避免过度归因。
6. 数据因环境不同可能会有差异，属于正常现象
