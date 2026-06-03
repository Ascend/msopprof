# MindStudio Ops Profiler Quick Start

<br>

## 1. Overview

The msOpProf performance analysis tool is used to collect and analyze key performance metrics of operators running on Ascend AI Processors. You can efficiently locate software and hardware performance bottlenecks of operators based on the output performance data, thereby enhancing the overall efficiency of operator performance analysis.
This document demonstrates the core functions of msOpProf based on the simple addition operator developed in the introductory tutorial. It helps beginners intuitively experience the efficiency and convenience the tool brings to the operator development process.

### 1.1 Recommendations

This document assumes that you have completed all operations in <a href="https://gitcode.com/Ascend/msot/blob/master/docs/zh/quick_start/op_tool_quick_start.md" target="_blank">Ascend Operator Development Toolchain Quick Start</a>. If you have not done so, complete that guide first for a better learning experience.

### 1.2 Environment Preparation

Strictly follow the <a href="https://gitcode.com/Ascend/msot/blob/master/docs/zh/quick_start/installation_guide.md" target="_blank">Ascend AI Operator Development Toolchain Learning Environment Installation Guide</a> to complete the environment installation and workspace configuration.
Even if you have a similar environment, perform the steps in the guide again to ensure that all dependent components and environment variables are complete and consistent.

## 2. Procedure

### 2.1 [Environment] Pre-checking the Runtime Environment

#### 2.1.1 Verifying Installation of Python Dependencies

Run the following command. If "All is OK" is displayed, the required Python packages and their versions meet the specifications:

```shell
python3 -c "import numpy, sympy, scipy, attrs, psutil, decorator; from packaging import version; assert version.parse(numpy.__version__) <= version.parse('1.26.4'); print('All is OK')"
```

If an error occurs, refer to [Section 1.2](#12-environment-preparation) for correct installation.

### 2.2 [Prerequisite] Completing Operator Project Preparation

Follow the instructions in <a href="https://gitcode.com/Ascend/msot/blob/master/docs/zh/quick_start/op_tool_quick_start.md" target="_blank">Ascend Operator Development Toolchain Quick Start</a> to complete sections 2.1 and 2.3.

### 2.3 [Tuning] Analyzing Operator Performance (msOpProf)

If operator performance does not meet expectations, you can use msOpProf to collect runtime performance data for in-depth analysis and optimization, ensuring efficient execution across different Ascend hardware platforms. Follow the operations first to experience the effect. You can read the principles later.

#### 2.3.1 Modifying Compilation Options and Re-deploying

**1. Modify the compilation options.**
In the first line of the `CMakeLists.txt` file on the kernel side, insert a configuration line to enable debugging information:

```shell
cd ~/ot_demo/workspace/src/AddCustom
\cp -f op_kernel/CMakeLists.txt op_kernel/CMakeLists.txt.orig.bak
sed -i "1i\\add_ops_compile_options(ALL OPTIONS -g)" op_kernel/CMakeLists.txt
```

**2. Re-compile and deploy the operator.**

```shell
bash ./build.sh
MY_OP_PKG=$(find ./build_out -maxdepth 1 -name "custom_opp_*.run" | head -1) && bash $MY_OP_PKG
```

#### 2.3.2 Starting Collection on On-board Hardware and Simulator

>[!NOTE]NOTE  
>**Knowledge point: Difference between on-board hardware and simulator collection**  
>On-board hardware: precisely captures real hardware characteristics such as operator execution time, pipe usage, memory bandwidth, and cache behavior, which are often difficult to reproduce with high fidelity on a simulator.  
>Simulator: provides more complete and stable analysis capabilities in instruction stream tracing and code hot spot location, but has limited simulation accuracy for hardware-related behaviors like memory access latency and bandwidth bottlenecks.   
>Therefore, combine both methods to leverage their complementary advantages for comprehensive performance diagnosis. If you do not have on-board hardware (NPU) in some scenarios, use the simulator mode for preliminary performance estimation and hotspot analysis.   

##### 2.3.2.1 Performance Collection on On-board Hardware  

Run the following commands:

```shell
cd ~/ot_demo/workspace/src/caller/build
msprof op --output=./msprof_output_npu ./execute_add_op
```

##### 2.3.2.2 Performance Collection on Simulator  

Refer to <a href="https://gitcode.com/Ascend/msot/blob/master/docs/en/quick_start/get_chip_soc_type.md" target="_blank">Chip SoC Type Acquisition Method</a> to obtain the chip type, and use it as the value of the `--soc-version` parameter.  

```shell
msprof op simulator --soc-version=Ascendxxxyy --output=./msprof_output_sim ./execute_add_op
```

#### 2.3.3 Viewing Performance Data Results

The tool generates results in .csv and .bin files in the directory specified by `--output`. If no error is reported, the execution is successful.

**.csv files**  
For example, you can see the following information after opening the `MemoryUB.csv` file:
The data shows that the task is equally divided into eight blocks, all of which are scheduled to the Vector Core for execution. For example, the bandwidth of Block 0 (1.02 GB/s) is significantly higher than that of Block 1 (0.77 GB/s). If the difference is too large, it may indicate room for optimization.

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

**.bin files**  
See the following section.

#### 2.3.4 Visualizing Performance Data Results via MindStudio Insight

The aforementioned .bin file can be opened using the MindStudio Insight tool to visualize various performance views, such as computing memory heatmaps, cache heatmaps, and operator code hot spot maps.

##### 2.3.4.1 Installing MindStudio Insight

Refer to the <a href="https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/mindstudio_insight_install_guide.md" target="_blank">MindStudio Insight Tool Documentation</a> to install the tool.

##### 2.3.4.2 Viewing with MindStudio Insight

MindStudio Insight is a standalone application after installation. Perform the following operations: click **Import Data** in the upper left corner, import `visualize_data.bin`, and then open the **Details** page to see many detailed charts.
For detailed operations and the specific meanings of the charts, refer to the <a href="https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/basic_operations.md" target="_blank">MindStudio Insight Tool Documentation</a>.

#### 2.3.5 Restoring Modified Files

Run the following commands:

```shell
cd ~/ot_demo/workspace/src/AddCustom
\cp -f op_kernel/CMakeLists.txt.orig.bak op_kernel/CMakeLists.txt
```
