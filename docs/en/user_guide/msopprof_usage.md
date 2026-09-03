# msOpProf Usage Scenarios

## Collecting Profile Data of Ascend C Operators via Kernel Launch

**Overview**

This section demonstrates how to use msOpProf to collect profile data of Ascend C operators invoked through the kernel launch method, using the kernel call operator `<<<>>>` as an example.

For more details on kernel launch scenarios, refer to [Kernel Launch Operator Development](https://www.hiascend.com/document/detail/zh/CANNCommunityEdition/910/programug/Ascendcopdevg/docs/guide/%E7%BC%96%E7%A8%8B%E6%8C%87%E5%8D%97/%E9%99%84%E5%BD%95/%E5%9F%BA%E4%BA%8E%E6%A0%B7%E4%BE%8B%E5%B7%A5%E7%A8%8B%E5%AE%8C%E6%88%90Kernel%E7%9B%B4%E8%B0%83.md) in the *Ascend C Operator Development Guide*.

**Prerequisites**

- Click [Add Sample](https://gitcode.com/cann/asc-devkit/tree/9.0.0/examples/01_simd_cpp_api/00_introduction/01_add/basic_api_tque_add) to obtain the sample project, using the Add vector addition operator as an example.

    ```shell
    git clone https://gitcode.com/cann/asc-devkit.git -b 9.0.0
    ```

- Refer to [Preparations](../user_guide/msopprof_user_guide.md#preparations) in the msOpProf mode user guide and [Preparations](../user_guide/msopprof_simulator_user_guide.md#preparations) in the msOpProf simulator mode user guide to configure the relevant environment variables for collecting operator on-board and simulation tuning data.

**Procedure**

1. Based on the sample project instructions, build an operator executable file that can run on Ascend devices. After compilation, the executable file `add` is generated in the project directory.

    ```shell
    mkdir -p build && cd build;   # Create and enter the build directory
    cmake ..;make -j;             # Build the project
    ```

    > [!NOTE]
    >
    > The executable file name (`add`) in this example is for illustration only. Use the actual file name based on the compilation script in your current project.

2. Run the following command to collect on-board profile data and detailed tuning data using msOpProf. You can also refer to the [msOpProf mode commands](https://gitcode.com/Ascend/msopprof/blob/26.1.0/docs/en/user_guide/msopprof_user_guide.md#command-reference) for other command options.

    ```shell
    msprof op add
    ```

3. Modify the `CMakeLists.txt` compilation file of the sample project to build an operator executable file that can run on the simulator. After compilation, the executable file `add_sim` is generated in the project directory.

    ```shell
    target_compile_options(add_sim PRIVATE
        $<$<COMPILE_LANGUAGE:ASC>:--npu-arch=dav-XXXX>          # Select the corresponding npu-arch parameter based on the actual NPU hardware architecture
        -g                                                      # Enable this compilation option for code hotspot analysis and other features
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

4. Run the following command to collect msOpProf simulator profile data, pipeline charts, and hotspot data. You can also refer to the [msOpProf simulator mode commands](https://gitcode.com/Ascend/msopprof/blob/26.1.0/docs/en/user_guide/msopprof_simulator_user_guide.md#command-reference) for other command options.

    > [!NOTE]
    > 
    > The value of the `--soc-version` parameter can be obtained by running the following command: `python3 -c "import acl; print(acl.get_soc_name())"`.

    ```shell
    msprof op simulator --soc-version=Ascendxxxyy add_sim
    ```

5. The following screen output indicates that operator profile data collection is successful.

    ```shell
    [INFO] Profiling running finished. All task success.
    ```

6. To view the on-board and simulation profile data, import the collected `visualize_data.bin` file into MindStudio Insight. For detailed import instructions, refer to [Importing Data](https://gitcode.com/Ascend/msinsight/blob/26.1.0/docs/en/user_guide/basic_operations.md#importing-data) in the *MindStudio Insight User Guide*.

    > [!NOTE]
    >
    > Profile data files obtained from other operator invocation scenarios can be viewed in the same way.

## Collecting Profile Data of a Single Operator via API Call

**Overview**

This section demonstrates how to use msOpProf to collect profile data of a single operator invoked through an API call, using a custom operator project and an ACLNN single operator API call as examples.

For more details on single operator API call scenarios, refer to the **Engineering Operator Development** > [Single Operator API Call](https://www.hiascend.com/document/detail/zh/CANNCommunityEdition/910/programug/Ascendcopdevg/docs/guide/%E7%BC%96%E7%A8%8B%E6%8C%87%E5%8D%97/%E9%AB%98%E7%BA%A7%E7%BC%96%E7%A8%8B/Aclnn%E7%AE%97%E5%AD%90%E5%B7%A5%E7%A8%8B%E5%8C%96%E5%BC%80%E5%8F%91/%E5%8D%95%E7%AE%97%E5%AD%90API%E8%B0%83%E7%94%A8.md) in the *Ascend C Operator Development Guide*.

**Prerequisites**

- Click [Custom Operator Project Sample](https://gitcode.com/cann/asc-devkit/tree/9.0.0/examples/01_simd_cpp_api/02_features/00_compilation/custom_op/) to obtain a custom operator project.

- Refer to [Preparations](../user_guide/msopprof_user_guide.md#preparations) in the msOpProf mode user guide and [Preparations](../user_guide/msopprof_simulator_user_guide.md#preparations) in the msOpProf simulator mode user guide to configure the relevant environment variables for collecting operator on-board and simulation tuning data.

**Procedure**

1. Based on the [sample project instructions](https://gitcode.com/cann/asc-devkit/blob/9.0.0/examples/01_simd_cpp_api/02_features/00_compilation/custom_op/README.md), complete the compilation, packaging, and deployment of the custom operator.

    ```shell
    mkdir -p build && cd build
    cmake .. && make -j binary package
    ./custom_opp_*.run
    ```

2. Based on the [ACLNN single operator API call sample](https://gitcode.com/cann/asc-devkit/blob/9.0.0/examples/01_simd_cpp_api/02_features/01_invocation/aclnn_invocation/README.md), build the operator executable file. After compilation, the executable file `execute_add_op` is generated in the project directory. This file can run on both Ascend devices and the simulator.

    ```shell
    mkdir -p build; cd build
    cmake .. && make -j
    ```

3. Run the following command to collect on-board profile data and detailed tuning data using msOpProf.

    ```shell
    msprof op execute_add_op
    ```
    
4. Run the following command to collect msOpProf simulator profile data, pipeline charts, and hotspot data.

    > [!NOTE]
    > 
    > The value of the `--soc-version` parameter can be obtained by running the following command: `python3 -c "import acl; print(acl.get_soc_name())"`.

    ```shell
    msprof op simulator --soc-version=Ascendxxxyy execute_add_op
    ```

## Collecting Profile Data of PyTorch Framework Operators

**Overview**

For scenarios involving single operator invocation through the PyTorch framework, refer to the `OpPlugin` plugin in the [TorchNPU Supporting Software Libraries](https://www.hiascend.com/document/detail/en/Pytorch/2610/userguide/SuppLib/FrameworkPTAdapter/26.1.0/en/supported_suites_and_third_party_libraries/supported_suites_and_third_party_libraries.md).

In PyTorch framework operator invocation scenarios, the procedure for collecting profile data is essentially the same as that for the [Triton operator collection scenario](#collecting-profile-data-of-triton-operators).

## Collecting Profile Data of Triton Operators

**Overview**

This section demonstrates how to use msOpProf to collect profile data of Triton operators.

**Prerequisites**

- Click [triton-ascend Quick Start](https://gitcode.com/Ascend/triton-ascend/blob/main/docs/en/quick_start.md) to complete the installation and configuration of Triton and the Triton-Ascend plugin.

- Prepare a Triton operator implementation file. If you have not prepared a Triton operator yet, refer to the example in the procedure below.

- Refer to [Preparations](../user_guide/msopprof_user_guide.md#preparations) in the msOpProf mode user guide and [Preparations](../user_guide/msopprof_simulator_user_guide.md#preparations) in the msOpProf simulator mode user guide to configure the relevant environment variables for collecting operator on-board and simulation tuning data.

**Procedure**

1. Prepare a basic Triton operator sample `test_add.py`.

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

2. Run the following command to collect on-board profile data and detailed tuning data using msOpProf.

    ```shell
    msprof op python3 test_add.py
    ```

3. Run the following command to collect msOpProf simulator profile data, pipeline charts, and hotspot data.

    > [!NOTE]
    > 
    > The value of the `--soc-version` parameter can be obtained by running the following command: `python3 -c "import acl; print(acl.get_soc_name())"`.

    ```shell
    msprof op simulator --soc-version=Ascendxxxyy python3 test_add.py
    ```

    > [!NOTE]
    >
    > This sample operator removes redundant computations unrelated to the Triton operator, retaining only the `add_kernel` Triton operator whose simulation performance needs to be collected. This significantly reduces the overall simulation runtime. Even when the `--kernel-name` option is specified, the simulator still runs operators sequentially. Therefore, you are advised to minimize unnecessary operators before running the simulation.

## Collecting Profile Data of CATLASS Operators

**Overview**

This section demonstrates how to use msOpProf to collect profile data of CATLASS operators.

**Prerequisites**

- Click [CATLASS Community](https://gitcode.com/cann/catlass) to obtain the sample project.

    ```shell
    git clone https://gitcode.com/cann/catlass.git -b v1.5.0
    ```

- Refer to [Preparations](../user_guide/msopprof_user_guide.md#preparations) in the msOpProf mode user guide and [Preparations](../user_guide/msopprof_simulator_user_guide.md#preparations) in the msOpProf simulator mode user guide to configure the relevant environment variables for collecting operator on-board and simulation tuning data.

**Procedure**

1. Follow the [CATLASS Quick Start](https://gitcode.com/cann/catlass/blob/master/docs/en/1_Practice/01_quick_start.md) example to prepare the environment and compile the operator on-board executable file, using the `basic_matmul` sample as an example.

    ```shell
    bash scripts/build.sh 00_basic_matmul
    ```

2. Run the following command to collect on-board profile data and detailed tuning data using msOpProf.

    ```shell
    # Switch to the build output directory
    cd output/bin
    # ./00_basic_matmul m n k [deviceId]
    msprof op ./00_basic_matmul 256 512 1024 0
    ```

3. Add the `--simulator` option to the build script to compile the operator simulator executable file. Then, load the simulator binary path as prompted.

    ```shell
    bash scripts/build.sh --simulator 00_basic_matmul
    # Set the following based on the actual output after compilation
    export LD_LIBRARY_PATH=/usr/local/Ascend/ascend-toolkit/latest/tools/simulator/Ascendxxxyy/lib:$LD_LIBRARY_PATH
    export LD_PRELOAD=/usr/local/Ascend/ascend-toolkit/latest/tools/simulator/Ascendxxxyy/lib/libruntime_camodel.so:/usr/local/Ascend/ascend-toolkit/latest/tools/simulator/Ascendxxxyy/lib/libnpu_drv_camodel.so
    ```

4. Run the following command to collect msOpProf simulator profile data, pipeline charts, and hotspot data.

    > [!NOTE]
    > 
    > The value of the `--soc-version` parameter can be obtained by running the following command: `python3 -c "import acl; print(acl.get_soc_name())"`.

    ```shell
    # Switch to the build output directory
    cd output/bin
    # Executable name | Matrix m n k | Device ID (optional)
    msprof op simulator --soc-version=Ascendxxxyy ./00_basic_matmul 256 512 1024 0
    ```

## Collecting Profile Data of MC2 Operators

**Overview**

This section demonstrates how to use msOpProf to tune an MC2 operator on the board and generate a communication and computing pipeline chart.

This example uses the Ascend CL single operator invocation as an example. For other invocation scenarios, refer to the [Ascend C Operator Development Guide](https://www.hiascend.com/document/detail/zh/CANNCommunityEdition/910/programug/Ascendcopdevg/docs/guide/%E5%85%A5%E9%97%A8%E6%95%99%E7%A8%8B/Ascend-C%E6%A6%82%E8%BF%B0%E4%B8%8E%E5%AD%A6%E4%B9%A0%E8%B7%AF%E5%BE%84.md).

**Prerequisites**

- Complete the development of the MC2 operator.
- Refer to [Preparations](../user_guide/msopprof_user_guide.md#preparations) in the msOpProf mode user guide to configure the relevant environment variables.

**Procedure**

1. Refer to [Operator Compilation and Deployment](https://gitcode.com/Ascend/msopgen/blob/26.1.0/docs/en/user_guide/msopgen_user_guide.md) to complete the compilation and deployment of the operator.
     1. In the `CMakeLists.txt` file located in the `op_kernel` directory of the operator compilation files, add the following compilation options to enable the AIC timestamping and code line mapping features for the MC2 operator.

        ```shell
        add_ops_compile_options(ALL OPTIONS -DASCENDC_TIME_STAMP_ON -g)
        ```

     2. Enter the custom operator project directory and compile and deploy the operator.

        ```shell
        ./build_out/custom_opp_<target_os>_<target_architecture>.run
        ```

2. Use msOpProf to collect profile data of the MC2 operator.

    ```shell
    msprof op --output=$HOME/projects/output $HOME/projects/MyApp blockdim 1   # The --output option is optional; $HOME/projects/MyApp is the app; blockdim 1 is an optional parameter for the user app 
    ```

3. The following directory structure and profile data files are generated. For details, refer to the [msOpProf mode user guide](../user_guide/msopprof_user_guide.md).
4. Import the `trace.json` or `visualize_data.bin` file into MindStudio Insight for visualization. For details, refer to [Computing Memory Heatmap](../user_guide/msopprof_user_guide.md#computing-memory-heatmap), [Communication and Computing Pipeline Chart](../user_guide/msopprof_user_guide.md#communication-and-computing-pipeline-chart), and [Roofline Bottleneck Analysis Chart](../user_guide/msopprof_user_guide.md#roofline-bottleneck-analysis-chart) in the msOpProf mode user guide.
