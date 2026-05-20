# msOpProf使用场景

## 采集Kernel直调方式Ascend C算子的性能数据

**概述**

展示如何使用msOpProf工具采集Kernel直调方式Ascend C算子的性能数据，以内核调用符<<<>>>方式调用算子为例。

Kernel直调场景，详细信息可参考《Ascend C算子开发指南》中“[Kernel直调算子开发](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_0052.html)”章节。

**前期准备**

- 单击[Add样例](https://gitcode.com/cann/asc-devkit/tree/master/examples/01_simd_cpp_api/00_introduction/01_vector/basic_api_tque_add)获取样例工程，以Add向量加法算子为例。

    ```shell
    git clone https://gitcode.com/cann/asc-devkit.git -b 9.0.0
    ```

- 分别参考msopprof模式用户指南的“[使用前准备](../user_guide/msopprof_user_guide.md#使用前准备)”和msopprof simulator模式用户指南的“[使用前准备](../user_guide/msopprof_simulator_user_guide.md#使用前准备)”完成相关环境变量配置，为采集算子上板和仿真调优数据做准备。

**操作步骤**

1. 基于样例工程的说明，构建可运行在昇腾设备上的算子可执行文件，编译完成后，在工程目录下生成可执行文件add。

    ```shell
    mkdir -p build && cd build;   # 创建并进入build目录
    cmake ..;make -j;             # 编译工程
    ```

    > [!NOTE] 说明
    > 
    > 本示例中可执行文件的名称（add）仅为示例，具体以当前工程中用户实际编译的脚本为准。

2. 使用如下命令完成msopprof上板性能数据和精细化调优数据的采集，也可参考[msopprof模式命令](https://gitcode.com/Ascend/msopprof/blob/master/docs/zh/user_guide/msopprof_user_guide.md#命令参考)指定其他命令参数。

    ```shell
    msprof op add
    ```

3. 修改样例工程的编译文件CMakeLists.txt，构建可运行在仿真器上的算子可执行文件，编译完成后，在工程目录下生成可执行文件add\_sim。

    ```shell
    target_compile_options(add_sim PRIVATE
        $<$<COMPILE_LANGUAGE:ASC>:--npu-arch=dav-XXXX>          # 需根据实际部署的 NPU 硬件架构选择对应的 `npu-arch` 参数
        -g                                                      # 代码热点图等功能需要开启该编译选项
        -O2
    )
    target_link_directories(add_sim PRIVATE
        $ENV{ASCEND_HOME_PATH}/tools/simulator/Ascendxxxyy/lib
    )
    target_link_libraries(add_sim PRIVATE
        runtime_camodel
        npu_drv
    )
    ```

4. 使用如下命令完成msopprof simulator性能数据、流水图和热点图数据的采集，也可参考[msopprof simulator模式命令](https://gitcode.com/Ascend/msopprof/blob/master/docs/zh/user_guide/msopprof_simulator_user_guide.md#命令参考)指定其他命令参数。

    > [!NOTE]说明
    > 参数 `--soc-version` 的值可通过执行以下命令获取：`python3 -c "import acl; print(acl.get_soc_name())"`。

    ```shell
    msprof op simulator --soc-version=Ascendxxxyy add_sim
    ```

5. 出现如下打屏回显，表示算子性能数据采集成功。

    ```shell
    [INFO] Profiling running finished. All task success.
    ```

6. 分别查看算子上板和仿真的性能数据，可将采集得到的visualize\_data.bin文件导入MindStudio Insight，具体导入操作请参考《MindStudio Insight用户指南》的“[导入性能数据](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/basic_operations.md#导入数据)”章节。<a id="导入数据"></a>

    > [!NOTE] 说明
    > 
    > 其它算子调用场景下获取的性能数据文件，可使用相同方式查看。

## 采集API调用单算子的性能数据

**概述**

展示如何使用msOpProf工具采集API调用单算子的性能数据，以自定义算子工程和aclnn单算子API调用为例。

单算子API调用场景，详细信息可参考《Ascend C算子开发指南》中“工程化算子开发 \>  [单算子API调用](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_0070.html)”章节。

**前期准备**

- 单击[自定义算子工程样例](https://gitcode.com/cann/asc-devkit/tree/master/examples/01_simd_cpp_api/02_features/00_compilation/custom_op/)，获取自定义算子工程。

- 分别参考msopprof模式用户指南的“[使用前准备](../user_guide/msopprof_user_guide.md#使用前准备)”和msopprof simulator模式用户指南的“[使用前准备](../user_guide/msopprof_simulator_user_guide.md#使用前准备)”完成相关环境变量配置，为采集算子上板和仿真调优数据做准备。

**操作步骤**

1. 基于[样例工程说明](https://gitcode.com/cann/asc-devkit/blob/master/examples/01_simd_cpp_api/02_features/00_compilation/custom_op/README.md)，完成自定义算子的编译、打包及部署。

    ```shell
    mkdir -p build && cd build
    cmake .. && make -j binary package
    ./custom_opp_*.run
    ```

2. 基于[aclnn单算子API调用样例](https://gitcode.com/cann/asc-devkit/blob/master/examples/01_simd_cpp_api/02_features/01_invocation/aclnn_invocation/README.md)，构建算子可执行文件。编译完成后，在工程目录下生成可执行文件execute_add_op，该文件可运行在昇腾设备和仿真器上。

    ```shell
    mkdir -p build; cd build
    cmake .. && make -j
    ```

3. 使用如下命令完成msopprof上板性能数据和精细化调优数据的采集。

    ```shell
    msprof op execute_add_op
    ```
    
4. 使用如下命令完成msopprof simulator性能数据、流水图和热点图数据的采集。

    > [!NOTE]说明
    > 参数 `--soc-version` 的值可通过执行以下命令获取：`python3 -c "import acl; print(acl.get_soc_name())"`。

    ```shell
    msprof op simulator --soc-version=Ascendxxxyy execute_add_op
    ```

## 采集PyTorch框架算子的性能数据

**概述**

通过PyTorch框架进行单算子调用的场景，详细信息可参考《Ascend Extension for PyTorch 套件与三方库支持清单》中“[昇腾自研插件](https://www.hiascend.com/document/detail/zh/Pytorch/720/modthirdparty/modparts/thirdpart_0009.html)”章节中OpPlugin插件。

PyTorch框架算子调用场景下，进行性能数据采集的操作步骤与[采集triton算子场景](https://gitcode.com/Ascend/msopprof/blob/master/docs/zh/user_guide/msopprof_usage.md#采集triton算子的性能数据)基本一致。

## 采集triton算子的性能数据

**概述**

展示如何使用msOpProf工具采集triton算子的性能数据。

**前期准备**

- 单击[triton-ascend快速入门](https://gitcode.com/Ascend/triton-ascend/blob/main/docs/zh/quick_start.md)，完成Triton及Triton-Ascend插件的安装和配置。

- 自备Triton算子实现文件。若用户尚未准备Triton算子，可参考操作步骤中的示例。

- 分别参考msopprof模式用户指南的“[使用前准备](../user_guide/msopprof_user_guide.md#使用前准备)”和msopprof simulator模式用户指南的“[使用前准备](../user_guide/msopprof_simulator_user_guide.md#使用前准备)”完成相关环境变量配置，为采集算子上板和仿真调优数据做准备。

**操作步骤**

1. 准备基础的triton算子样例test_add.py。

    ```shell
    import torch
    import torch_npu

    import triton
    import triton.language as tl
    
    M : tl.constexpr = 128
    N : tl.constexpr = 32

    @triton.jit
    def add_kernel(output_ptr, x_ptr, y_ptr):
        offsets = tl.arange(0, M * N)
        x = tl.load(x_ptr + offsets)
        y = tl.load(y_ptr + offsets)
        output = x + y
        tl.store(output_ptr + offsets, output)

    z = torch.randn((M, N), dtype=torch.float32).npu()
    res = torch.empty_like(z)
    add_kernel[8, 1, 1](res, z, z)
    ```

2. 使用如下命令完成msopprof上板性能数据和精细化调优数据的采集。

    ```shell
    msprof op python3 test_add.py
    ```

3. 使用如下命令完成msopprof simulator性能数据、流水图和热点图数据的采集。

    > [!NOTE]说明
    > 参数 `--soc-version` 的值可通过执行以下命令获取：`python3 -c "import acl; print(acl.get_soc_name())"`。

    ```shell
    msprof op simulator --soc-version=Ascendxxxyy python3 test_add.py
    ```

    > [!NOTE] 说明
    > 
    > 该样例算子已经去除了其它非triton算子的冗余计算，仅保留一个需要采集仿真性能的triton算子add_kernel，可极大节约仿真运行的整体耗时。即使指定了--kernel-name的情况下，仿真器仍会按照算子顺序依次运行，因此建议仿真运行前减少非必要的算子。

## 采集catlass算子的性能数据

**概述**

展示如何使用msOpProf工具采集catlass算子的性能数据。

**前期准备**

- 单击[catlass社区](https://gitcode.com/cann/catlass)获取样例工程。

    ```shell
    git clone https://gitcode.com/cann/catlass.git -b v1.5.0
    ```

- 分别参考msopprof模式用户指南的“[使用前准备](../user_guide/msopprof_user_guide.md#使用前准备)”和msopprof simulator模式用户指南的“[使用前准备](../user_guide/msopprof_simulator_user_guide.md#使用前准备)”完成相关环境变量配置，为采集算子上板和仿真调优数据做准备。

**操作步骤**

1. 按照[catlass快速入门](https://gitcode.com/cann/catlass/blob/master/docs/zh/1_Practice/01_quick_start.md)的示例，准备环境并编译算子上板可执行文件，以basic_matmul样例为例。

    ```shell
    bash scripts/build.sh 00_basic_matmul
    ```

2. 使用如下命令完成msopprof上板性能数据和精细化调优数据的采集。

    ```shell
    # 切换至编译产物目录
    cd output/bin
    # ./00_basic_matmul m n k [deviceId]
    msprof op ./00_basic_matmul 256 512 1024 0
    ```

3. 编译脚本增加选项--simulator，编译算子仿真可执行文件，并根据提示，加载仿真器二进制路径。

    ```shell
    bash scripts/build.sh --simulator 00_basic_matmul
    # 以下根据编译后的实际输出执行
    export LD_LIBRARY_PATH=/usr/local/Ascend/ascend-toolkit/latest/tools/simulator/Ascendxxxyy/lib:$LD_LIBRARY_PATH
    export LD_PRELOAD=/usr/local/Ascend/ascend-toolkit/latest/tools/simulator/Ascendxxxyy/lib/libruntime_camodel.so:/usr/local/Ascend/ascend-toolkit/latest/tools/simulator/Ascendxxxyy/lib/libnpu_drv_camodel.so
    ```

4. 使用如下命令完成msopprof simulator性能数据、流水图和热点图数据的采集。

    > [!NOTE]说明
    > 参数 `--soc-version` 的值可通过执行以下命令获取：`python3 -c "import acl; print(acl.get_soc_name())"`。

    ```shell
    # 切换至编译产物目录
    cd output/bin
    # 可执行文件名 |矩阵m轴|n轴|k轴|Device ID（可选）
    msprof op simulator --soc-version=Ascendxxxyy ./00_basic_matmul 256 512 1024 0
    ```

## 采集MC2算子的性能数据

**概述**

展示如何使用msOpProf工具来上板调优一个MC2算子，并生成通算流水图。

本示例以Ascend CL单算子调用为例，其他调用场景请参见《[Ascend C算子开发指南](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_0001.html)》。

**前期准备**

- 完成MC2算子的开发。
- 参考msopprof模式用户指南的“[使用前准备](../user_guide/msopprof_user_guide.md#使用前准备)”完成相关环境变量配置。

**操作步骤**

1. 请参考[算子编译部署](https://www.hiascend.com/document/detail/zh/mindstudio/82RC1/ODtools/Operatordevelopmenttools/atlasopdev_16_0024.html)，完成算子的编译部署。
    1. 在算子编译文件op\_kernel目录下的CMakeLists.txt中引入以下编译选项，使能MC2算子的AIC打点和代码行映射功能。

        ```shell
        add_ops_compile_options(ALL OPTIONS -DASCENDC_TIME_STAMP_ON -g)
        ```

    2. 进入自定义算子工程目录下编译部署算子。

        ```shell
        ./build_out/custom_opp_<target_os>_<target_architecture>.run
        ```

2. 使用msOpProf采集MC2算子的性能数据。

    ```shell
    msprof op --output=$HOME/projects/output $HOME/projects/MyApp blockdim 1   # --output为可选参数,$HOME/projects/MyApp为使用的app,blockdim 1为用户app的可选参数 
    ```

3. 界面生成以下目录结构和性能数据文件，具体请参见[msopprof模式用户指南](../user_guide/msopprof_user_guide.md)。
4. 将trace.json或visualize\_data.bin文件导入MindStudio Insight工具进行可视化呈现，具体请参见msopprof模式用户指南中的[计算内存热力图](../user_guide/msopprof_user_guide.md#计算内存热力图)、[通算流水图](../user_guide/msopprof_user_guide.md#通算流水图)和[Roofline瓶颈分析图](../user_guide/msopprof_user_guide.md#roofline瓶颈分析图)。
