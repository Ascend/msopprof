# msOpProf 算子性能调优工具快速入门

<br>

## 1. 概述

msOpProf 工具用于采集和分析运行在昇腾 AI 处理器上的算子的关键性能指标，用户可根据输出的性能数据，快速定位算子的软、硬件性能瓶颈，提升算子性能的分析效率。
本文档基于入门教程中开发的简易加法算子，演示 msOpProf 工具的核心功能，帮助初学者直观体会其在算子开发过程中带来的高效性与便捷性。

### 1.1 建议

本章节以您已完成《[算子开发工具快速入门](https://gitcode.com/Ascend/msot/blob/master/docs/zh/quick_start/op_tool_quick_start.md)》的全流程操作为前提；若尚未体验，建议先完成该指南以获得更佳的学习效果。

### 1.2 环境准备

请严格按照《[昇腾 AI 算子开发工具链学习环境安装指南](https://gitcode.com/Ascend/msot/blob/master/docs/zh/quick_start/installation_guide.md)》完成环境安装与工作区配置。
即使您已具备类似环境，也需按该指南重新执行一遍，以确保所有依赖组件、环境变量等完整且一致。

## 2. 操作步骤

### 2.1 【环境】运行环境预检

#### 2.1.1 确认 Python 依赖包已安装

执行以下命令，若输出"All is OK"，则表明所需 Python 包及其版本均满足规范：

```shell
python3 -c "import numpy, sympy, scipy, attrs, psutil, decorator; from packaging import version; assert version.parse(numpy.__version__) <= version.parse('1.26.4'); print('All is OK')"
```

若报错，请参照[1.2 环境准备](#12-环境准备)进行正确安装。

### 2.2 【前提】算子工程准备完成

按照<a href="https://gitcode.com/Ascend/msot/blob/master/docs/zh/quick_start/op_tool_quick_start.md" target="_blank">《昇腾算子开发工具链快速入门》</a>中的指导，完成 2.1 节和 2.3 节。

### 2.3 【调优】分析算子性能（msOpProf）

若算子性能未达预期，可借助 msOpProf 工具采集运行时性能数据，进行深入分析与优化，确保算子在不同昇腾硬件平台上高效执行。先跟着操作体验效果，原理部分可稍后阅读。

#### 2.3.1 修改编译选项并重新编译部署

**1. 修改编译选项**

在 Kernel 侧 CMakeLists.txt 首行插入如下配置，开启调试信息：

```shell
# 进入对应目录进行文件备份
cd ~/ot_demo/workspace/src/AddCustom
cp -f op_kernel/CMakeLists.txt op_kernel/CMakeLists.txt.bak

# 将以下配置内容插入CMakeLists.txt 首行
printf '%s\n' "if(COMMAND add_ops_compile_options)" "  add_ops_compile_options(ALL OPTIONS -g)" "elseif(COMMAND npu_op_kernel_options)" "  npu_op_kernel_options(ascendc_kernels ALL OPTIONS -g)" "endif()" | cat - op_kernel/CMakeLists.txt > tmp && mv -f tmp op_kernel/CMakeLists.txt;
```

**2. 重新编译部署算子**

```shell
bash ./build.sh
MY_OP_PKG=$(find ./build_out -maxdepth 1 -name "custom_opp_*.run" | head -1) && bash $MY_OP_PKG
```

#### 2.3.2 启动真机与仿真采集

> [!NOTE]
> 
> **知识点：上板和仿真采集信息的区别**   
> 上板：可精确捕获算子运行耗时、各 Pipe 使用情况、内存带宽、Cache 行为等真实硬件特性，而这些往往是仿真器难以高保真复现的关键指标。   
> 仿真：在指令流追踪、代码热点定位等方面提供更完整、稳定的分析能力，但对内存访问延迟、带宽瓶颈等硬件相关行为的模拟精度有限。    
> 因此，建议结合两种方式，互补优势，实现全面性能诊断。若某些场景下您没有真实硬件（NPU 卡），可以使用仿真模式进行初步的性能估算和热点分析。    

##### 2.3.2.1 上板性能采集   

执行如下命令：

```shell
cd ~/ot_demo/workspace/src/caller/build
msopprof --output=./msopprof_output_npu ./execute_add_op
```

##### 2.3.2.2 仿真器性能采集   

> [!NOTE]说明
> 参数 `--soc-version` 的值可通过执行以下命令获取：`python3 -c "import acl; print(acl.get_soc_name())"`。

```shell
msopprof simulator --soc-version={Ascendxxxyy} --output=./msopprof_output_sim ./execute_add_op
```

#### 2.3.3 查看性能数据结果

工具在指定 `--output` 目录下生成 .csv 和 .bin 格式的结果文件，生成的目录结构请参见[目录结构参考](../user_guide/msopprof_simulator_user_guide.md#目录结构参考)，若输出没有报错，则认为执行成功：

**csv 文件**   
例如，MemoryUB.csv 文件打开后可以看到如下信息：
数据显示任务被均分为 8 个 block，全部调度至 Vector Core 执行。例如，Block 0 的带宽（1.02GB/s）明显高于 Block 1（0.77GB/s），如果差异过大，可能提示有优化空间。

| block_id | sub_block_id | aiv_time(us) | aiv_total_cycles | aiv_ub_read_bw_vector(GB/s) | aiv_ub_write_bw_vector(GB/s) | 
|:--------:|:------------:|:------------:|:----------------:|:---------------------------:|:----------------------------:|
|    0     |   vector0    |  7.456666  |      13422      |          1.023164           |           0.511582           | 
|    1     |   vector0    |  9.914444  |      17846      |          0.769523           |           0.384762           | 
|    2     |   vector0    |  10.001111 |      18002      |          0.762855           |           0.381427           | 
|    3     |   vector0    |  9.684444  |      17432      |          0.787799           |           0.393899           | 
|    4     |   vector0    |  9.747222  |      17545      |          0.782725           |           0.391363           | 
|    5     |   vector0    |  9.062222  |      16312      |          0.84189            |           0.420945           | 
|    6     |   vector0    |  9.293889  |      16729      |          0.820904           |           0.410452           | 
|    7     |   vector0    |  8.658889  |      15586      |          0.881105           |           0.440553           | 

**bin 文件**   
请学习下节内容。

#### 2.3.4 通过 MindStudio Insight 图形化查看性能数据结果

上面的 bin 文件可使用 `MindStudio Insight` 工具打开，以图形化方式直观展示各类性能视图，例如：计算内存热力图、Cache 热力图以及算子代码热点图等。

##### 2.3.4.1 安装 MindStudio Insight

请参考<a href="https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/install_guide/mindstudio_insight_install_guide.md" target="_blank">《MindStudio Insight工具文档》</a>安装 Insight 工具。

##### 2.3.4.2 用 MindStudio Insight 查看

安装后是单机程序，简单操作如下：点击左上角 Import Data，将 visualize_data.bin 导入，然后打开 Details 页面，即可看到很多详细图表。
详细操作及图表具体含义请参考<a href="https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/basic_operations.md" target="_blank">《MindStudio Insight工具文档》</a>学习。

#### 2.3.5 恢复被修改的文件

执行如下命令：

```shell
cd ~/ot_demo/workspace/src/AddCustom
cp -f op_kernel/CMakeLists.txt.bak op_kernel/CMakeLists.txt
```
