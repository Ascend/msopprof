# MindStudio Ops Profiler Development Environment Setup, Compilation, and UT Methods

<br>

## 1. Background Knowledge Required

For details about the code framework and core process, see the [msOpProf Architecture Design Specifications](./architecture.md).

## 2. Development Environment Setup

- For hardware environment requirements, see [Ascend Product Models](https://www.hiascend.com/document/detail/zh/AscendFAQ/ProduTech/productform/hardwaredesc_0001.html).

- Set up the environment by referring to the [Operator Tool Development Environment Setup Guide](https://gitcode.com/Ascend/msot/blob/master/docs/en/common/dev_env_setup.md).

- The CMake version must be between 3.20.2 and 3.31.10 (inclusive).
 
- Run the `pip install numpy` command to install the NumPy Python dependency library.

- The generated .run package depends on pigz, which is typically provided by the system. Run the `pigz --version` command to check whether pigz has been installed.

## 3. Building and Packaging

There are two methods, each with its own advantages and disadvantages:

| Method| Application Scenario| Advantages| Disadvantages|
|------|---------|------|------|
| One-click script| Initial build and CI/CD pipeline| Zero configuration, one-step setup| Steps cannot be executed independently.|
| Step-by-step script| Routine development and incremental build| Flexibility and efficiency| Multiple steps are required.|

### 3.1 Method 1: One-click Script

```shell
python build.py
```

### 3.2 Method 2: Step-by-Step Script

#### 3.2.1 Downloading Dependencies

```shell
python download_dependencies.py
```

#### 3.2.2 Building and Packaging

##### 3.2.2.1 Starting Build

Run the following commands to start the build:

```shell
mkdir build
cd build
cmake ../cmake   # If you only need to build the project and do not need to generate a .runfile, run cmake .. instead.
make -j$(nproc)  # -j indicates the number of parallel build jobs, which can be specified as required. If nproc is unavailable, manually enter a number (for example, -j8).
```

>[!NOTE]NOTE   
> **Debug version build**   
> To perform GDB or VS Code graphical breakpoint debugging, build the debug version. The procedure is as follows:  
> Add `-DCMAKE_BUILD_TYPE=Debug` when running the preceding CMake command, for example, `cmake ../cmake -DCMAKE_BUILD_TYPE=Debug`.

If the generation time of the `mindstudio-opprof_xxx_xxx.run` file in the `output` directory is updated to the current build time, the building and packaging are successful.

##### 3.2.2.2 Build Result Description

The build result is generated in the `output` directory:

```text
output/
|-- bin                                                  # Executable binary file, which can directly call the tuning function
|-- filelist.csv                                         # Configuration file for packaging the .run package
|-- lib64                                                # Various dynamic and static libraries
|-- mindstudio-opprof/                                   # Directory for packing the .run package
|-- mindstudio-opprof_xxx_xxx.run                        # Installation package
|-- parser_install.sh                                    # Public script for parsing .run package installation
|-- version.info                                         # Version information
```

#### 3.2.3 Cleanup/Rebuild

Delete the build directory and perform [3.2.2.1](#3221-starting-build).  

```shell
rm -rf build
```

## 4. Running UT

### 4.1 Method 1: One-Click Script

```shell
python build.py test
```

### 4.2 Method 2: Step-by-Step Script

#### 4.2.1 Downloading Dependencies

```shell
python download_dependencies.py test
```

#### 4.2.2 Running UT

>[!NOTE]NOTE   
> **CMake entry description for UT**   
> The UT build uses `CMakeLists.txt` in the root directory (that is, `cmake ..` instead of `cmake ../cmake`) of the project. Only the test and dependency are built, and the .run packaging process is not included.

```shell
mkdir build_ut
cd build_ut
cmake .. -DBUILD_TESTS=ON
make -j$(nproc) # -j indicates the number of parallel build jobs, which can be specified as required. If nproc is unavailable, manually enter a number (for example, -j8).
./test/ut/msopt_test --gtest_output=xml:test_detail.xml
```

If the number of executed test cases is the same as the number of passed test cases, the test is successful. The output is similar to the following:

```text
[==========] 526 tests from 75 test cases ran. (24571 ms total)
[  PASSED  ] 526 tests.
```

#### 4.2.3 Cleanup/Rebuild

Delete the build directory and perform [4.2.2](#422-running-ut).  

```shell
rm -rf build_ut  
```

## 5. FAQ

### 5.1 Why Is No .run Package Generated When I Run the make Command During Building?  

It is possible that `cmake ..` is used when running the `cmake` command. The `cmake` command is described as follows:  

- `cmake ..`: Only the current project is built. The `make install` command installs the project to the `output/` directory, but does not call `makeself`. Therefore, no `Ascend-mindstudio-sanitizer-xxx.run` file is generated.
- `cmake ../cmake`: The "super build" of `cmake/CMakeLists.txt` is used. The project is built and installed first, and then `parser.py` and `makeself` are executed to generate the .run file in the `output/` directory.
