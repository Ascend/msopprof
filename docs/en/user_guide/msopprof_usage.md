# msOpProf Usage Scenarios

## Collecting Profile Data of Ascend C Operators via Kernel Launch

**Overview**

This section demonstrates how to use msOpProf to collect profile data of Ascend C operators invoked through the kernel launch method, using the kernel call operator `<<<>>>` as an example.

For more details on kernel launch scenarios, refer to [Direct Kernel Invocation Based on a Sample Project](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/programug/Ascendcopdevg/docs/en/guide/programming_guide/appendix/completing_kernel_launch_based_on_sample_project.md) in the *Ascend C Operator Development Guide*.

**Prerequisites**

- Click [Add Sample](https://atomgit.com/cann/asc-devkit/tree/9.0.0/examples/01_simd_cpp_api/00_introduction/01_add/basic_api_tque_add) to obtain the sample project, using the Add vector addition operator as an example.

    ```shell
    git clone https://atomgit.com/cann/asc-devkit.git -b 9.0.0
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

2. Run the following command to collect on-board profile data and detailed tuning data using msOpProf. You can also refer to the [msOpProf mode commands](https://atomgit.com/Ascend/msopprof/blob/26.2.0/docs/en/user_guide/msopprof_user_guide.md#command-reference) for other command options.

    ```shell
    msopprof add
    ```

3. Modify the `CMakeLists.txt` compilation file of the sample project to build an operator executable file that can run on the simulator. After compilation, the executable file `add_sim` is generated in the project directory.

    ```shell
    target_compile_options(add_sim PRIVATE
        $<$<COMPILE_LANGUAGE:ASC>:--npu-arch=dav-XXXX>          # Select the corresponding npu-arch parameter based on the actual NPU hardware architecture
        -g                                                      # Enable this compilation option for code hot spot analysis and other features
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

4. Run the following command to collect msOpProf simulator profile data, pipeline charts, and hot spot data. You can also refer to the [msOpProf simulator mode commands](./msopprof_simulator_user_guide.md#command-reference) for other command options.

    > [!NOTE]
    >
    > The value of the `--soc-version` parameter can be obtained by running the following command: `python3 -c "import acl; print(acl.get_soc_name())"`.

    ```shell
    msopprof simulator --soc-version=Ascendxxxyy add_sim
    ```

5. The following screen output indicates that operator profile data collection is successful.

    ```shell
    [INFO] Profiling running finished. All task success.
    ```

6. To view the on-board and simulation profile data, import the collected `visualize_data.bin` file into MindStudio Insight. For detailed import instructions, refer to [Importing Data](https://atomgit.com/Ascend/msinsight/blob/26.2.0/docs/en/user_guide/basic_operations.md#importing-data) in the *MindStudio Insight User Guide*.<a id="import-data"></a>

    > [!NOTE]
    >
    > Profile data files obtained from other operator invocation scenarios can be viewed in the same way.

## Collecting Profile Data of a Single Operator via API Call

**Overview**

This section demonstrates how to use msOpProf to collect profile data of a single operator invoked through an API call, using a custom operator project and an ACLNN single operator API call as examples.

For more details on single operator API call scenarios, refer to the **Engineering Operator Development** > [Single Operator API Call](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/programug/Ascendcopdevg/docs/en/guide/programming_guide/advanced_programming/aclnn_operator_development/single_operator_api_call.md) in the *Ascend C Operator Development Guide*.

**Prerequisites**

- Click [Custom Operator Project Sample](https://atomgit.com/cann/asc-devkit/tree/9.0.0/examples/01_simd_cpp_api/02_features/00_compilation/custom_op/) to obtain a custom operator project.

- Refer to [Preparations](../user_guide/msopprof_user_guide.md#preparations) in the msOpProf mode user guide and [Preparations](../user_guide/msopprof_simulator_user_guide.md#preparations) in the msOpProf simulator mode user guide to configure the relevant environment variables for collecting operator on-board and simulation tuning data.

**Procedure**

1. Based on the [sample project instructions](https://atomgit.com/cann/asc-devkit/blob/9.0.0/examples/01_simd_cpp_api/02_features/00_compilation/custom_op/README.md), complete the compilation, packaging, and deployment of the custom operator.

    ```shell
    mkdir -p build && cd build
    cmake .. && make -j binary package
    ./custom_opp_*.run
    ```

2. Based on the [ACLNN single operator API call sample](https://atomgit.com/cann/asc-devkit/blob/9.0.0/examples/01_simd_cpp_api/02_features/01_invocation/aclnn_invocation/README.md), build the operator executable file. After compilation, the executable file `execute_add_op` is generated in the project directory. This file can run on both Ascend devices and the simulator.

    ```shell
    mkdir -p build; cd build
    cmake .. && make -j
    ```

3. Run the following command to collect on-board profile data and detailed tuning data using msOpProf.

    ```shell
    msopprof execute_add_op
    ```
    
4. Run the following command to collect msOpProf simulator profile data, pipeline charts, and hot spot data.

    > [!NOTE]
    >
    > The value of the `--soc-version` parameter can be obtained by running the following command: `python3 -c "import acl; print(acl.get_soc_name())"`.

    ```shell
    msopprof simulator --soc-version=Ascendxxxyy execute_add_op
    ```

## Collecting Profile Data of PyTorch Framework Operators

**Overview**

For scenarios involving single operator invocation through the PyTorch framework, refer to the `OpPlugin` plugin in the [TorchNPU Supporting Software Libraries](https://atomgit.com/Ascend/docs/blob/master/FrameworkPTAdapter/26.1.0/en/supported_suites_and_third_party_libraries/supported_suites_and_third_party_libraries.md).

In PyTorch framework operator invocation scenarios, the procedure for collecting profile data is essentially the same as that for the [Triton operator collection scenario](#collecting-profile-data-of-triton-operators).

## Collecting Profile Data of Triton Operators

**Overview**

This section demonstrates how to use msOpProf to collect profile data of Triton operators.

**Prerequisites**

- Click [triton-ascend Quick Start](https://atomgit.com/Ascend/triton-ascend/blob/main/docs/en/quick_start.md) to complete the installation and configuration of Triton and the Triton-Ascend plugin.

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
    msopprof python3 test_add.py
    ```

3. Run the following command to collect msOpProf simulator profile data, pipeline charts, and hot spot data.

    > [!NOTE]
    >
    > The value of the `--soc-version` parameter can be obtained by running the following command: `python3 -c "import acl; print(acl.get_soc_name())"`.

    ```shell
    msopprof simulator --soc-version=Ascendxxxyy python3 test_add.py
    ```

    > [!NOTE]
    >
    > This sample operator removes redundant computations unrelated to the Triton operator, retaining only the `add_kernel` Triton operator whose simulation performance needs to be collected. This significantly reduces the overall simulation runtime. Even when the `--kernel-name` option is specified, the simulator still runs operators sequentially. Therefore, you are advised to minimize unnecessary operators before running the simulation.

## Collecting Profile Data of CATLASS Operators

**Overview**

This section demonstrates how to use msOpProf to collect profile data of CATLASS operators.

**Prerequisites**

- Click [CATLASS Community](https://atomgit.com/cann/catlass) to obtain the sample project.

    ```shell
    git clone https://atomgit.com/cann/catlass.git -b v1.5.0
    ```

- Refer to [Preparations](../user_guide/msopprof_user_guide.md#preparations) in the msOpProf mode user guide and [Preparations](../user_guide/msopprof_simulator_user_guide.md#preparations) in the msOpProf simulator mode user guide to configure the relevant environment variables for collecting operator on-board and simulation tuning data.

**Procedure**

1. Follow the [CATLASS Quick Start](https://atomgit.com/cann/catlass/blob/master/docs/en/1_Practice/01_quick_start.md) example to prepare the environment and compile the operator on-board executable file, using the `basic_matmul` sample as an example.

    ```shell
    bash scripts/build.sh 00_basic_matmul
    ```

2. Run the following command to collect on-board profile data and detailed tuning data using msOpProf.

    ```shell
    # Switch to the build output directory
    cd output/bin
    # ./00_basic_matmul m n k [deviceId]
    msopprof ./00_basic_matmul 256 512 1024 0
    ```

3. Add the `--simulator` option to the build script to compile the operator simulator executable file. Then, load the simulator binary path as prompted.

    ```shell
    bash scripts/build.sh --simulator 00_basic_matmul
    # Set the following based on the actual output after compilation
    export LD_LIBRARY_PATH=/usr/local/Ascend/ascend-toolkit/latest/tools/simulator/Ascendxxxyy/lib:$LD_LIBRARY_PATH
    export LD_PRELOAD=/usr/local/Ascend/ascend-toolkit/latest/tools/simulator/Ascendxxxyy/lib/libruntime_camodel.so:/usr/local/Ascend/ascend-toolkit/latest/tools/simulator/Ascendxxxyy/lib/libnpu_drv_camodel.so
    ```

4. Run the following command to collect msOpProf simulator profile data, pipeline charts, and hot spot data.

    > [!NOTE]
    >
    > The value of the `--soc-version` parameter can be obtained by running the following command: `python3 -c "import acl; print(acl.get_soc_name())"`.

    ```shell
    # Switch to the build output directory
    cd output/bin
    # Executable name | Matrix m n k | Device ID (optional)
    msopprof simulator --soc-version=Ascendxxxyy ./00_basic_matmul 256 512 1024 0
    ```

## Collecting Profile Data of CATLASS DSL Operators

**Overview**

This section demonstrates how to use msOpProf to collect profile data of CATLASS DSL operators. For an introduction to CATLASS DSL, refer to the [CATLASS DSL Community Documentation](https://atomgit.com/cann/catlass/blob/master/python/tla_dsl/README.md).

**Prerequisites**

- Click [CATLASS DSL Quick Start](https://atomgit.com/cann/catlass/blob/master/python/tla_dsl/docs/en/quick_start.md) to complete the environment configuration and source code build.

- Prepare a CATLASS DSL operator implementation file. If you have not prepared a CATLASS DSL operator yet, refer to the example in the procedure below.

- Refer to [Preparations](../user_guide/msopprof_user_guide.md#preparations) in the msOpProf mode user guide and [Preparations](../user_guide/msopprof_simulator_user_guide.md#preparations) in the msOpProf simulator mode user guide to configure the relevant environment variables for collecting operator on-board and simulation tuning data.

> [!NOTE]
>
> CATLASS DSL supports the same scope as Triton. Currently, only Atlas 950PR&950DT products are supported. The `-g` option, the code hot spot analysis feature, and the code line mapping capability are not supported.

**Procedure**

1. Prepare a basic CATLASS DSL operator sample `catlass_dsl_matmul.py`.

    ```python
    # catlass_dsl_matmul.py
    from catlass import tla
    from catlass.tla.runtime import from_dlpack
    import torch
    import torch_npu

    M, K, N = 16, 16, 16

    @tla.kernel
    def matmul_kernel(a: tla.Tensor, b: tla.Tensor, c: tla.Tensor) -> None:
        l0c_data_ready = tla.flag("l0c_data_ready", tla.arch.CUBE, tla.arch.FIX)
        l1_loaded = tla.flag("l1_loaded", tla.arch.MTE2, tla.arch.MTE1)
        l0_loaded = tla.flag("l0_loaded", tla.arch.MTE1, tla.arch.CUBE)

        l1a_ptr = tla.allocate(M * K, tla.Float16, tla.AddressSpace.l1, 512)
        l1b_ptr = tla.allocate(K * N, tla.Float16, tla.AddressSpace.l1, 512)
        l0a_ptr = tla.allocate(M * K, tla.Float16, tla.AddressSpace.l0a, 512)
        l0b_ptr = tla.allocate(K * N, tla.Float16, tla.AddressSpace.l0b, 512)
        l0c_ptr = tla.allocate(M * N, tla.Float32, tla.AddressSpace.l0c, 512)

        with tla.cube():
            # GM -> L1
            l1_a = tla.make_tensor_like(l1a_ptr, a, tla.arch.zN)
            l1_b = tla.make_tensor_like(l1b_ptr, b, tla.arch.zN)
            tla.copy(l1_a, a)
            tla.copy(l1_b, b)

            tla.set_flag(l1_loaded)
            tla.wait_flag(l1_loaded)

            # L1 -> L0A / L0B
            l0_a = tla.make_tensor_like(l0a_ptr, l1_a, tla.arch.zN)
            l0_b = tla.make_tensor_like(l0b_ptr, l1_b, tla.arch.nZ)
            l0_c = tla.make_tensor_like(l0c_ptr, c, tla.arch.L0Clayout)
            tla.copy(l0_a, l1_a)
            tla.copy(l0_b, l1_b)

            tla.set_flag(l0_loaded)
            tla.wait_flag(l0_loaded)

            # Cube: C = A x B
            tla.mmad(l0_c, l0_a, l0_b, init_c=True)

            # L0C -> GM
            tla.set_flag(l0c_data_ready)
            tla.wait_flag(l0c_data_ready)
            tla.copy(c, l0_c)

    def main() -> int:
        torch.npu.set_device(0)
        torch.manual_seed(0)
        a = torch.rand(M, K, dtype=torch.float16, device="cpu") * 10.0 - 5.0
        b = torch.rand(K, N, dtype=torch.float16, device="cpu") * 10.0 - 5.0
        c = torch.rand(M, N, dtype=torch.float32, device="cpu") * 10.0 - 5.0
        ref = a.float() @ b.float()

        a = a.contiguous().npu()
        b = b.contiguous().npu()
        c = c.contiguous().npu()
        a_tensor = from_dlpack(a, layout_tag=tla.arch.RowMajor, origin_shape=(M, K))
        b_tensor = from_dlpack(b, layout_tag=tla.arch.RowMajor, origin_shape=(K, N))
        c_tensor = from_dlpack(c, layout_tag=tla.arch.RowMajor, origin_shape=(M, N))

        artifact = tla.compile(
            matmul_kernel,
            a_tensor,
            b_tensor,
            c_tensor,
            options="--npu-arch 3510",
        )
        artifact(a_tensor, b_tensor, c_tensor, block_num=1)
        torch.npu.synchronize()

        passed = torch.allclose(c.cpu(), ref)
        print("Passed." if passed else "Failed.")

    main()

    ```

2. Run the following command to collect on-board profile data and detailed tuning data using msOpProf.

    ```shell
    msopprof python3 catlass_dsl_matmul.py
    ```

3. Run the following command to collect msOpProf simulator profile data, pipeline charts, and hot spot data.

    > [!NOTE]
    >
    > The value of the `--soc-version` parameter can be obtained by running the following command: `python3 -c "import acl; print(acl.get_soc_name())"`.

    ```shell
    msopprof simulator --soc-version=Ascendxxxyy python3 catlass_dsl_matmul.py
    ```

## Collecting Profile Data of MC2 Operators

**Overview**

This section demonstrates how to use msOpProf to tune an MC2 operator on the board and generate a communication and computing pipeline chart.

This example uses the Ascend CL single operator invocation as an example. For other invocation scenarios, refer to the [Ascend C Operator Development Guide](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/programug/Ascendcopdevg/docs/en/guide/getting_started/ascend_c_overview_and_learning_path.md).

**Prerequisites**

- Complete the development of the MC2 operator.
- Refer to [Preparations](../user_guide/msopprof_user_guide.md#preparations) in the msOpProf mode user guide to configure the relevant environment variables.

**Procedure**

1. Refer to [Operator Compilation and Deployment](https://atomgit.com/Ascend/msopgen/blob/26.2.0/docs/en/user_guide/msopgen_user_guide.md) to complete the compilation and deployment of the operator.
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
    msopprof --output=$HOME/projects/output $HOME/projects/MyApp blockdim 1   # The --output option is optional; $HOME/projects/MyApp is the app; blockdim 1 is an optional parameter for the user app 
    ```

3. The following directory structure and profile data files are generated. For details, refer to the [msOpProf mode user guide](../user_guide/msopprof_user_guide.md).
4. Import the `trace.json` or `visualize_data.bin` file into MindStudio Insight for visualization. For details, refer to [Computing Memory Heatmap](../user_guide/msopprof_user_guide.md#computing-memory-heatmap), [Communication and Computing Pipeline Chart](../user_guide/msopprof_user_guide.md#communication-and-computing-pipeline-chart), and [Roofline Bottleneck Analysis Chart](../user_guide/msopprof_user_guide.md#roofline-bottleneck-analysis-chart) in the msOpProf mode user guide.
