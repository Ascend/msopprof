# msOpProf Usage Scenarios

## Collecting Performance Data of Kernel Launch Ascend C Operators

**Overview**

This section demonstrates how to use the msOpProf tool to collect performance data of Ascend C operators in kernel launch mode, using the kernel launch operator `<<<>>>` invocation as an example.

For details about the kernel launch scenario, see the [Kernel Launch Operator Development](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_0052.html) chapter in the *Ascend C Operator Development Guide*.

**Preparations**

- Click the [Add sample](https://gitcode.com/cann/asc-devkit/tree/master/examples/01_simd_cpp_api/00_introduction/01_vector/basic_api_tque_add) to obtain the sample project. The Add vector addition operator is used as an example.

    ```shell
    git clone https://gitcode.com/cann/asc-devkit.git -b 9.0.0
    ```

- Refer to the [Preparations](../user_guide/msopprof_user_guide.md#preparations) section of the *msOpProf Mode User Guide* and the [Preparations](../user_guide/msopprof_simulator_user_guide.md#preparations) section of the *msOpProf Simulator Mode User Guide* to configure the required environment variables for on-board and simulator tuning data collection.

**Procedure**

1. Based on the instructions in the sample project, build an operator executable file that can run on Ascend devices. After the compilation is complete, the executable file `add` is generated in the project directory.

    ```shell
    mkdir -p build && cd build;   # Create and enter the build directory.
    cmake ..;make -j;             # Build the project.
    ```

    > [!NOTE] NOTE
    > 
    > The executable file name (`add`) in this example is for reference only. Use the actual compiled file name in the current project.

2. Run the following command to collect msopprof on-board performance data and refined tuning data. You can also specify other command parameters by referring to the [msopprof Mode Command Reference](https://gitcode.com/Ascend/msopprof/blob/master/docs/en/user_guide/msopprof_user_guide.md#command-reference).

    ```shell
    msprof op add
    ```

3. Modify the build file `CMakeLists.txt` in the sample project to build an operator executable file that can run on the simulator. After the compilation is complete, the executable file `add_sim` is generated in the project directory.

    ```shell
    target_compile_options(add_sim PRIVATE
        $<$<COMPILE_LANGUAGE:ASC>:--npu-arch=dav-XXXX>          # Select the corresponding 'npu-arch' parameter based on the actual deployed NPU hardware architecture.
        -g                                                      # This compilation option is required for features such as the code hot spot map.
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

4. Run the following command to collect msopprof simulator performance data, pipeline chart data, and hot spot map data. You can also specify other command parameters by referring to the [msopprof Simulator Mode Command Reference](https://gitcode.com/Ascend/msopprof/blob/master/docs/en/user_guide/msopprof_simulator_user_guide.md#command-reference).

    > [!NOTE] NOTE
    > The value of the parameter `--soc-version` can be obtained by running the following command: `python3 -c "import acl; print(acl.get_soc_name())"`.

    ```shell
    msprof op simulator --soc-version=Ascendxxxyy add_sim
    ```

5. If the following message is displayed, the operator performance data collection is successful.

    ```shell
    [INFO] Profiling running finished. All task success.
    ```

6. View the on-board and simulation performance data of the operator. You can import the generated `visualize_data.bin` file into MindStudio Insight. For details about the import operation, see the [Importing Profile Data](https://gitcode.com/Ascend/msinsight/blob/master/docs/en/user_guide/basic_operations.md#%E5%AF%BC%E5%85%A5%E6%95%B0%E6%8D%AE) chapter in the *MindStudio Insight User Guide*. <a id="import-data"></a>

    > [!NOTE] NOTE
    > 
    > The performance data files obtained in other operator invocation scenarios can be viewed in the same way.

## Collecting Performance Data of Single-Operator API Calls

**Overview**

This section demonstrates how to use the msOpProf tool to collect performance data of single-operator API calls, using a custom operator project and the aclnn single-operator API invocation as an example.

For details about the single-operator API invocation scenario, see the **Project-based Operator Development** > [Single-Operator API Invocation](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_0070.html) chapter in the *Ascend C Operator Development Guide*.

**Preparations**

- Click the [custom operator project sample](https://gitcode.com/cann/asc-devkit/tree/master/examples/01_simd_cpp_api/02_features/00_compilation/custom_op/) to obtain the custom operator project.

- Refer to the [Preparations](../user_guide/msopprof_user_guide.md#preparations) section of the *msOpProf Mode User Guide* and the [Preparations](../user_guide/msopprof_simulator_user_guide.md#preparations) section of the *msOpProf Simulator Mode User Guide* to configure the required environment variables for on-board and simulator tuning data collection.

**Procedure**

1. Based on the [sample project description](https://gitcode.com/cann/asc-devkit/blob/9.0.0/examples/01_simd_cpp_api/02_features/00_compilation/custom_op/README.md), compile, package, and deploy the custom operator.

    ```shell
    mkdir -p build && cd build
    cmake .. && make -j binary package
    ./custom_opp_*.run
    ```

2. Based on the [aclnn single-operator API invocation sample](https://gitcode.com/cann/asc-devkit/blob/9.0.0/examples/01_simd_cpp_api/02_features/01_invocation/aclnn_invocation/README.md), build the operator executable file. After the compilation is complete, the executable file `execute_add_op` is generated in the project directory. This file can run on both Ascend devices and the simulator.

    ```shell
    mkdir -p build; cd build
    cmake .. && make -j
    ```

3. Run the following command to collect msopprof on-board performance data and refined tuning data.

    ```shell
    msprof op execute_add_op
    ```
    
4. Run the following command to collect msopprof simulator performance data, pipeline chart data, and hot spot map data.

    > [!NOTE] NOTE
    > The value of the parameter `--soc-version` can be obtained by running the following command: `python3 -c "import acl; print(acl.get_soc_name())"`.

    ```shell
    msprof op simulator --soc-version=Ascendxxxyy execute_add_op
    ```

## Collecting Performance Data of PyTorch Framework Operators

**Overview**

For the scenario of single-operator invocation through the PyTorch framework, for details, see the OpPlugin in the [Ascend-developed Plugins](https://www.hiascend.com/document/detail/zh/Pytorch/720/modthirdparty/modparts/thirdpart_0009.html) chapter of the *Ascend Extension for PyTorch Suite and Third-party Library Support List*.

The procedure for collecting performance data in the PyTorch framework operator invocation scenario is basically the same as that in the [Collecting Performance Data of Triton Operators](https://gitcode.com/Ascend/msopprof/blob/master/docs/en/user_guide/msopprof_usage.md#collecting-performance-data-of-triton-operators) scenario.

## Collecting Performance Data of Triton Operators

**Overview**

This section demonstrates how to use the msOpProf tool to collect performance data of Triton operators.

**Preparations**

- Click [Triton-Ascend Quick Start](https://gitcode.com/Ascend/triton-ascend/blob/main/docs/en/quick_start.md) to complete the installation and configuration of Triton and the Triton-Ascend plugin.

- Prepare the Triton operator implementation file. If you have not prepared a Triton operator, refer to the example in the procedure.

- Refer to the [Preparations](../user_guide/msopprof_user_guide.md#preparations) section of the *msOpProf Mode User Guide* and the [Preparations](../user_guide/msopprof_simulator_user_guide.md#preparations) section of the *msOpProf Simulator Mode User Guide* to configure the required environment variables for on-board and simulator tuning data collection.

**Procedure**

1. Prepare a basic Triton operator sample file `test_add.py`.

    ```python
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

2. Run the following command to collect msopprof on-board performance data and refined tuning data.

    ```shell
    msprof op python3 test_add.py
    ```

3. Run the following command to collect msopprof simulator performance data, pipeline chart data, and hot spot map data.

    > [!NOTE] NOTE
    > The value of the parameter `--soc-version` can be obtained by running the following command: `python3 -c "import acl; print(acl.get_soc_name())"`.

    ```shell
    msprof op simulator --soc-version=Ascendxxxyy python3 test_add.py
    ```

    > [!NOTE] NOTE
    > 
    > This sample operator has removed the redundant computation of non-Triton operators, retaining only one Triton operator `add_kernel` for which the simulation performance needs to be collected. This can greatly reduce the overall simulation runtime. Even when `--kernel-name` is specified, the simulator still runs operators sequentially. Therefore, you are advised to minimize the number of unnecessary operators before simulation.

## Collecting Performance Data of Catlass Operators

**Overview**

This section demonstrates how to use the msOpProf tool to collect performance data of Catlass operators.

**Preparations**

- Click the [Catlass Community](https://gitcode.com/cann/catlass) to obtain the sample project.

    ```shell
    git clone https://gitcode.com/cann/catlass.git -b v1.5.0
    ```

- Refer to the [Preparations](../user_guide/msopprof_user_guide.md#preparations) section of the *msOpProf Mode User Guide* and the [Preparations](../user_guide/msopprof_simulator_user_guide.md#preparations) section of the *msOpProf Simulator Mode User Guide* to configure the required environment variables for on-board and simulator tuning data collection.

**Procedure**

1. Follow the example in the [Catlass Quick Start](https://gitcode.com/cann/catlass/blob/master/docs/en/1_Practice/01_quick_start.md) guide to prepare the environment and compile the on-board operator executable file. The `basic_matmul` sample is used as an example.

    ```shell
    bash scripts/build.sh 00_basic_matmul
    ```

2. Run the following command to collect msopprof on-board performance data and refined tuning data.

    ```shell
    # Switch to the build output directory.
    cd output/bin
    # ./00_basic_matmul m n k [deviceId]
    msprof op ./00_basic_matmul 256 512 1024 0
    ```

3. Add the `--simulator` option to the build script to compile the operator simulation executable file, and load the simulator binary path as prompted.

    ```shell
    bash scripts/build.sh --simulator 00_basic_matmul
    # Execute the following commands based on the actual output after compilation:
    export LD_LIBRARY_PATH=/usr/local/Ascend/ascend-toolkit/latest/tools/simulator/Ascendxxxyy/lib:$LD_LIBRARY_PATH
    export LD_PRELOAD=/usr/local/Ascend/ascend-toolkit/latest/tools/simulator/Ascendxxxyy/lib/libruntime_camodel.so:/usr/local/Ascend/ascend-toolkit/latest/tools/simulator/Ascendxxxyy/lib/libnpu_drv_camodel.so
    ```

4. Run the following command to collect msopprof simulator performance data, pipeline chart data, and hot spot map data.

    > [!NOTE] NOTE
    > The value of the parameter `--soc-version` can be obtained by running the following command: `python3 -c "import acl; print(acl.get_soc_name())"`.

    ```shell
    # Switch to the build output directory.
    cd output/bin
    # Executable file name |m axis|n axis|k axis|Device ID (optional)
    msprof op simulator --soc-version=Ascendxxxyy ./00_basic_matmul 256 512 1024 0
    ```

## Collecting Performance Data of MC2 Operators

**Overview**

This section demonstrates how to use the msOpProf tool to tune an MC2 operator on the board and generate a communication and computing pipeline chart.

This example uses AscendCL single-operator call as an example. For other invocation scenarios, see the [Ascend C Operator Development Guide](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_0001.html).

**Preparations**

- Complete the development of the MC2 operator.
- Refer to the [Preparations](../user_guide/msopprof_user_guide.md#preparations) section of the *msOpProf Mode User Guide* to configure the required environment variables.

**Procedure**

1. Refer to [Operator Compilation and Deployment](https://www.hiascend.com/document/detail/zh/mindstudio/82RC1/ODtools/Operatordevelopmenttools/atlasopdev_16_0024.html) to compile and deploy the operator.
    1. Add the following compilation options to the `CMakeLists.txt` file in the `op_kernel` directory of the operator build file to enable the AIC instrumentation and code line mapping functions of the MC2 operator.

        ```shell
        add_ops_compile_options(ALL OPTIONS -DASCENDC_TIME_STAMP_ON -g)
        ```

    2. Go to the custom operator project directory and compile and deploy the operator.

        ```shell
        ./build_out/custom_opp_<target_os>_<target_architecture>.run
        ```

2. Use msOpProf to collect the performance data of the MC2 operator.

    ```shell
    msprof op --output=$HOME/projects/output $HOME/projects/MyApp blockdim 1   # --output is an optional parameter. $HOME/projects/MyApp is the application. blockdim 1 is an optional parameter of the user application.
    ```

3. The following directory structure and performance data files are generated. For details, see the [*msOpProf Mode User Guide*](../user_guide/msopprof_user_guide.md).
4. Import the `trace.json` or `visualize_data.bin` file into MindStudio Insight for visual presentation. For details, see the [Computing Memory Heatmap](../user_guide/msopprof_user_guide.md#computing-memory-heatmap), [Communication and Computing Pipeline Chart](../user_guide/msopprof_user_guide.md#communication-and-computing-pipeline-chart), and [Roofline Bottleneck Analysis Chart](../user_guide/msopprof_user_guide.md#roofline-bottleneck-analysis-chart) sections in the *msOpProf Mode User Guide*.
