# **MindStudio Ops Profiler工具快速入门**

## 概述

**简介**

MindStudio Ops Profiler（算子调优工具，msOpProf）主要作用于算子开发的性能优化阶段，通过使用msOpProf工具，开发者可以确保算子在不同硬件平台上都能高效运行，从而提高软件的整体性能和用户体验。

本文档以一个简单样例介绍msOpProf算子调优工具的使用方法。

**环境准备**

请参考[MindStudio Ops Profiler安装指南](./msopprof_install_guide.md)安装msOpProf工具。

## 操作步骤

1. 在\$\{git\_clone\_path\}/samples/operator/ascendc/0\_introduction/1\_add\_frameworklaunch目录下执行以下命令，生成自定义算子工程，进行host侧和kernel侧的算子实现。$\{git\_clone\_path\}为sample仓的存放路径。

    ```bash
    bash install.sh -v Ascendxxxyy    # xxxyy为用户实际使用的具体芯片类型
    ```

2. 在$\{git\_clone\_path\}/samples/operator/ascendc/0\_introduction/1\_add\_frameworklaunch/CustomOp目录下执行以下命令行，重新编译部署算子。

    ```bash
    bash build.sh
    ./build_out/custom_opp_<target_os>_<target_architecture>.run   // 当前目录下run包的名称
    ```

3. 切换到$\{git\_clone\_path\}/samples/operator/ascendc/0\_introduction/1\_add\_frameworklaunch/AclNNInvocation目录，执行以下命令生成可执行文件。

    ```bash
    ./run.sh
    ```

4. 指定算子依赖的动态库路径，将动态库so文件加载进来。$\{INSTALL\_DIR\}为CANN软件安装后文件存储路径。

    ```bash
    export LD_LIBRARY_PATH=${INSTALL_DIR}/opp/vendors/customize/op_api/lib:$LD_LIBRARY_PATH
    ```

5. 使用msOpProf工具进行调优。
    - 使用msopprof进行上板调优。
        1. 切换到$\{git\_clone\_path\}/samples/operator/ascendc/0\_introduction/1\_add\_frameworklaunch/AclNNInvocation/output目录，执行以下命令，开启上板调优。

            ```bash
            msprof op --output=./output_data ./execute_add_op
            ```

        2. 生成以下结果目录。

            ```text
            OPPROF_{timestamp}_XXX/
            ├── ArithmeticUtilization.csv
            ├── dump
            ├── L2Cache.csv
            ├── Memory.csv
            ├── MemoryL0.csv
            ├── MemoryUB.csv
            ├── OpBasicInfo.csv
            ├── PipeUtilization.csv
            ├── ResourceConflictRatio.csv
            └── visualize_data.bin
            ```

        3. 将visualize\_data.bin文件导入MindStudio Insight工具，将上板结果可视化，具体操作请参见msopprof模式用户指南的“[工具使用](./msopprof_user_guide.md#工具使用)”章节。

    - 使用msopprof simulator进行仿真调优。
        1. 请参考msopprof simulator模式用户指南的“[工具使用](./msopprof_simulator_user_guide.md#工具使用)”章节，完成msopprof simulator配置。
        2. 进入$\{git\_clone\_path\}/samples/operator/ascendc/0\_introduction/1\_add\_frameworklaunch/AclNNInvocation/output目录，执行以下命令，开启仿真调优。

            ```bash
            msprof op simulator --soc-version=Ascendxxxyy --output=./output_data ./execute_add_op
            ```

        3. 生成以下结果目录。

            ```text
            OPPROF_{timestamp}_XXX/
            ├── dump
            └── simulator
                ├── core0.veccore0
                ├── core0.veccore1
                ├── core1.veccore0
                ├── core1.veccore1
                ├── core2.veccore0
                ├── core2.veccore1
                ├── core3.veccore0
                ├── core3.veccore1
                ├── trace.json
                └── visualize_data.bin
            ```

        4. 将trace.json和visualize\_data.bin文件导入MindStudio Insight工具，将仿真结果可视化，具体操作请参见msopprof simulator模式用户指南的“[工具使用](./msopprof_simulator_user_guide.md#工具使用)”章节。
