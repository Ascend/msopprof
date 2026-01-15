# **典型案例**

## 采集Ascend C算子的性能数据

**概述**

展示如何使用msOpProf工具来上板调优一个Vector算子，该Vector算子可实现两个向量相加并输出结果的功能。本示例以Kernel直调算子调用场景为例进行介绍。

Kernel直调、单算子API调用和PyTorch框架三种算子调用场景下进行性能采集的操作步骤基本一致。

**前期准备**

- 单击[链接](https://gitee.com/ascend/samples/tree/master/operator/ascendc/0_introduction/3_add_kernellaunch/AddKernelInvocationNeo)获取样例工程，为进行算子上板和仿真调优做准备。

    > [!NOTE] 说明
    > 
    > - 此样例工程不支持<term>Atlas A3 训练系列产品</term>。
    > - 下载代码样例时，需执行以下命令指定分支版本。
    > 
    >    ```shell
    >    git clone https://gitee.com/ascend/samples.git -b v1.9-8.3.RC1
    >    ```

- 分别参考msopprof模式用户指南的“[使用前准备](./msopprof_user_guide.md#使用前准备)”和msopprof simulator模式用户指南的“[使用前准备](./msopprof_simulator_user_guide.md#使用前准备)”完成相关环境变量配置。

**操作步骤**

1. 基于样例工程的说明，并参考《Ascend C算子开发指南》中“Kernel直调算子开发 \>  [Kernel直调](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_0056.html)”章节，完成算子编译前的准备工作。
2. 构建单算子可执行文件。

    以Add算子为例，在样例工程的$\{git\_clone\_path\}/samples/operator/ascendc/0\_introduction/3\_add\_kernellaunch/AddKernelInvocationNeo目录下，执行以下命令，构建可执行文件。

    ```shell
    bash run.sh -r npu -v <soc_version>  # 运行在昇腾设备上的算子
    bash run.sh -r sim -v <soc_version>  # 运行在仿真器上的算子
    ```

    一键式编译运行脚本完成后，在工程目录下生成NPU侧可执行文件ascendc\_kernels\_bbit。

    > [!NOTE] 说明
    > 
    > - 本示例中可执行文件的名称（ascendc\_kernels\_bbit）仅为示例，具体以当前工程中用户实际编译的脚本为准。
    > - 在安装昇腾AI处理器的服务器上执行`npu-smi info`命令进行查询，获取**Chip Name**信息。实际配置值为AscendChip Name，例如**Chip Name**取值为*xxxyy*，实际配置值为Ascend*xxxyy*。

3. 导入环境变量。

    ```shell
    export LD_LIBRARY_PATH=${git_clone_path}/samples/operator/ascendc/0_introduction/3_add_kernellaunch/AddKernelInvocationNeo/out/lib/:$LD_LIBRARY_PATH
    ```

4. 采集算子性能数据。
    - 对于运行在昇腾设备上的算子，使用如下命令完成msopprof性能数据和精细化调优数据的采集。

        ```shell
        msprof op ascendc_kernels_bbit
        ```

    - 对于运行在仿真器上的算子，使用如下命令完成msopprof simulator性能数据、流水图和热点图数据的采集。

        ```shell
        msprof op simulator --soc-version=Ascendxxxyy ascendc_kernels_bbit  # xxxyy为用户实际使用的具体芯片类型
        ```

5. 查看算子性能数据。

## 通过指令流水图优化算子

**概述**

展示如何通过msOpProf工具的指令流水图特性，分析算子的瓶颈点，并实现Vector算子的性能优化。

**操作步骤**

1. 参考[msopprof simulator用户指南](./msopprof_simulator_user_guide.md)，将算子仿真性能数据采集得到的visualize\_data.bin文件导入MindStudio Insight，具体导入操作请参考《MindStudio Insight用户指南》的“[导入性能数据](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/basic_operations.md#%E5%AF%BC%E5%85%A5%E6%95%B0%E6%8D%AE)”章节。<a id="导入数据"></a>
2. 查看算子指令流水图。

    可以发现MTE2流水在VADD计算时，没有执行搬运指令，且MTE2流水为该算子的性能瓶颈，需提高MTE2的搬运效率以实现算子性能优化。

    ![](./figures/算子指令流水图1.png)

3. 对于MTE2搬运效率的提升有多种方式，此处以开启Ascend C算子的double buffer机制为例。

    例如样例算子核函数中，可通过将TPipe中InitBuffer的第二个参数（BUFFER\_NUM）值从1修改为2，开启double buffer，InitBuffer的使用可参考《Ascend C算子开发接口》中的“基础API \> 内存管理与同步控制 \> TPipe \>  [InitBuffer](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/ascendcopapi/atlasascendc_api_07_0110.html)”章节。

    ```shell
    constexpr int32_t BUFFER_NUM = 2;        # tensor num for each queue
    ...
    pipe.InitBuffer(inQueueY, BUFFER_NUM, 1024 * sizeof(half));
    ...
    ```

4. 重新执行[1](#导入数据)，查看优化后的指令流水图。

    在VADD指令计算时，MTE2上的搬运指令也同步执行，实现了更高效的数据搬运。

    ![](./figures/算子指令流水图2优化后.png)

## 采集MC2算子的性能数据

**概述**

展示如何使用msOpProf工具来上板调优一个MC2算子，并生成通算流水图。

本示例以Ascend CL单算子调用为例，其他调用场景请参见《[Ascend C算子开发指南](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_0001.html)》。

**前期准备**

- 完成MC2算子的开发。
- 参考msopprof模式用户指南的“[使用前准备](./msopprof_user_guide.md#使用前准备)”完成相关环境变量配置。

**操作步骤**

1. 请参考[算子编译部署](https://www.hiascend.com/document/detail/zh/mindstudio/82RC1/ODtools/Operatordevelopmenttools/atlasopdev_16_0024.html)，完成算子的编译部署。
    1. 在算子编译文件op\_kernel目录下的CMakeLists.txt中引入以下编译选项，使能MC2算子的AIC打点和代码行映射功能。

        ```shell
        add_ops_compile_options(ALL OPTIONS -DASCENDC_TIME_STAMP_ON, -g)
        ```

    2. 进入自定义算子工程目录下编译部署算子。

        ```shell
        ./build_out/custom_opp_<target_os>_<target_architecture>.run
        ```

2. 使用msOpProf采集MC2算子的性能数据。

    ```shell
    msprof op --output=$HOME/projects/output $HOME/projects/MyApp blockdim 1   # --output为可选参数,$HOME/projects/MyApp为使用的app,blockdim 1为用户app的可选参数 
    ```

3. 界面生成以下目录结构和性能数据文件，具体请参见[msopprof模式用户指南](./msopprof_user_guide.md)。
4. 将trace.json或visualize\_data.bin文件导入MindStudio Insight工具进行可视化呈现，具体请参见msopprof模式用户指南中的[计算内存热力图](./msopprof_user_guide.md#计算内存热力图)、[通算流水图](./msopprof_user_guide.md#通算流水图)和[Roofline瓶颈分析图](./msopprof_user_guide.md#roofline瓶颈分析图)。

## 配合mstx接口实现范围级重放

**概述**

展示如何使用msOpProf工具配合mstx接口实现范围级重放，以保留算子执行时上下文的L2Cache信息。

**前期准备**

准备算子工程，并在算子代码中添加mstx扩展接口确定范围级重放的范围，具体请参见[mstx扩展功能](./extended_functions.md#mstx扩展功能)和《[MindStudio mstx API参考](https://www.hiascend.com/document/detail/zh/mindstudio/82RC1/API/mstxAPIReference/msprof_tx_0001.html)》。

> [!NOTE] 说明
> 
> - mstxRangeStartA和mstxRangeEnd接口需成对调用，不支持交叉调用。每一对mstx API中包含的算子为一个重放范围，该重放范围内算子的Stream不能改变。
> - 每一个重放范围能采集的算子数量受[OpBasicInfo（算子基础信息）](./msopprof_performance_data.md#opbasicinfo算子基础信息)中算子Block Dim数量限制，建议不超过50个。
> - 使用该功能时，不支持与--aic-metrics=MemoryDetail、--aic-metrics=TimelineDetail及--aic-metrics=Source同时使能；不建议与--kill=on同时使能，否则可能导致采集的算子数据缺失。
> - 在进行范围级重放时，执行算子SynchronizeStream可能会失败，建议在mstxRangeEnd接口调用结束后再执行。
> - 该功能仅适用于<term>Atlas A3 训练系列产品/Atlas A3 推理系列产品</term>和<term>Atlas A2 训练系列产品/Atlas A2 推理系列产品</term>。

**调用示例**

以Python API接口方式（test.py文件）为例，说明msOpProf工具如何配合mstx接口实现范围级重放。

```python
import mstx
import torch
import torch_npu
 
x = torch.Tensor([1,2,3,4]).npu()
y = torch.Tensor([1,2,3,4]).npu()

a = x + y
range1_id = mstx.range_start("range1", None)
b = a - x
c = a * x
mstx.range_end(range1_id)
range2_id = mstx.range_start("range2", None)
d = x / y
range3_id = mstx.range_start("range3", None)
e = torch.abs(y)
mstx.range_end(range3_id)
f = x + e
mstx.range_end(range2_id)
```

**操作步骤**

- 单range范围级重放
    1. 执行以下命令，使能单一mstx API范围，以下命令将执行“range1”范围级重放。

        ```shell
        msprof op --replay-mode=range --mstx=on --mstx-include="range1" --launch-count=10 python3 test.py
        ```

    2. 工具生成Sub、Mul算子的调优数据，且两个算子之间的L2Cache信息会保留。具体性能文件介绍请参考[表2 msopprof模式文件介绍](./msopprof_user_guide.md#工具使用)。

        ```tex
        OPPROF_{timestamp}_XXX
        ├── Mul_XXX  // Mul_XXX为采集算子名称
        │   └── 0
        │       ├── dump
                        ...
        │       └── visualize_data.bin
        └── Sub_XXX
            └── 0
                ├── dump
                       ...
                └── visualize_data.bin
        ```

- 多range范围级重放
    1. 执行以下命令，使能所有mstx API范围。

        ```shell
        msprof op --replay-mode=range --mstx=on --launch-count=10 python3 test.py
        ```

    2. 工具将会先后执行“range1”和“range2”范围级重放，生成Sub、Mul、Div、Abs、Add算子的调优数据，每次重放算子之间的L2Cache信息会保留，但两次重放的L2Cache信息互相独立。但因为“range2”和“range3”存在范围交叉，则仅第一个范围生效，“range3”将无效。具体性能文件介绍请参考[表2 msopprof模式文件介绍](./msopprof_user_guide.md#工具使用)。

        ```tex
        OPPROF_{timestamp}_XXX
        ├── Abs_XXX  // Abs_XXX为采集算子名称
        │   └── 0
        │       ├── dump
                        ...
        │       └── visualize_data.bin
        ├── Add_XXX
        │   └── 0
        │       ├── dump
                        ...
        │       └── visualize_data.bin
        ├── Mul_XXX
        │   └── 0
        │       ├── dump
                        ...
        │       └── visualize_data.bin
        ├── RealDiv_XXX
        │   └── 0
        │       ├── dump
                        ...
        │       └── visualize_data.bin
        └── Sub_XXX
            └── 0
                ├── dump
                       ...
                └── visualize_data.bin
        ```
