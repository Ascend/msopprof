# MindStudio Ops Profiler 开发环境搭建及编译和UT方法

<br>

## 1. 预备知识

请参考《[msOpProf 架构设计说明书](./architecture.md)》学习代码框架及核心流程。

## 2. 开发环境准备

- 硬件环境请参见《[昇腾产品形态说明](https://www.hiascend.com/document/detail/zh/AscendFAQ/ProduTech/productform/hardwaredesc_0001.html)》。

- 请按照以下文档进行环境配置：《[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)》。

- 编译工具CMake版本应大于或等于3.20.2，小于或等于3.31.10。
 
- 安装Python库的numpy依赖，执行```pip install numpy```安装。

- 生成run包依赖pigz，一般系统自带，执行```pigz --version```确认是否已安装。

## 3. 编译打包

分为如下两种方式，优缺点如下：

| 方法 | 适用场景 | 优点 | 缺点 |
|------|---------|------|------|
| 一键式脚本 | 首次构建、CI/CD 流水线 | 零配置，一步到位 | 无法单独执行某一步骤 |
| 分步骤脚本 | 日常开发、增量编译 | 灵活，效率高 | 需要多步操作 |

### 3.1 方法一：一键式脚本

```shell
python build.py
```

### 3.2 方法二：分步骤脚本

#### 3.2.1 下载依赖

```shell
python download_dependencies.py
```

#### 3.2.2 编译打包

##### 3.2.2.1 启动编译

执行如下命令启动编译：

```shell
mkdir build
cd build
cmake ../cmake   # 如果只做编译，不打run包，这里需要执行的是cmake ..
make -j$(nproc)  # -j 是并行编译的 job 数量，可自行指定；nproc 不可用时请手动填数字（例如 -j8）。
```

>[!NOTE] 说明    
> **debug 版本编译方法**    
> 如果想进行 gdb 或 vscode 图形化断点调试等，需要编译 debug 版本，方法如下：   
> 在执行如上 cmake 命令时增加参数 -DCMAKE_BUILD_TYPE=Debug，例如：`cmake ../cmake -DCMAKE_BUILD_TYPE=Debug`

若 `output` 目录下的 `mindstudio-opprof_xxx_xxx.run` 文件的生成时间已更新为当前编译时间，则表明编译与打包已成功完成。

##### 3.2.2.2 编译结果说明

编译结果生成到 output 目录下：

```text
output/
|-- bin                                                  # 可执行bin文件，可直接调用调优功能
|-- filelist.csv                                         # run包打包配置文件
|-- lib64                                                # 各种动态库和静态库
|-- mindstudio-opprof/                                   # run包打包目录
|-- mindstudio-opprof_xxx_xxx.run                        # 安装包
|-- parser_install.sh                                    # run包安装解析公共脚本
|-- version.info                                         # 版本信息
```

#### 3.2.3 清理/重新编译

删除构建目录，重新执行[第 3.2.2.1 节](#3221-启动编译)：   

```shell
rm -rf build
```

## 4. 执行UT测试

### 4.1 方法一：一键式脚本

```shell
python build.py test
```

### 4.2 方法二：分步骤脚本

#### 4.2.1 下载依赖

```shell
python download_dependencies.py test
```

#### 4.2.2 执行UT测试

>[!NOTE] 说明    
> **UT 的 CMake 入口说明**    
> UT 构建使用工程根目录的 `CMakeLists.txt`（即 `cmake ..`，不是`cmake ../cmake`），只编译测试与依赖，不包含 `.run` 打包流程。

```shell
mkdir build_ut
cd build_ut
cmake .. -DBUILD_TESTS=ON
make -j$(nproc) # -j 是并行编译的 job 数量，可自行指定；nproc 不可用时请手动填数字（例如 -j8）。
./test/ut/msopt_test --gtest_output=xml:test_detail.xml
```

输出类似如下跑的用例数和通过用例数相同即表示成功：

```text
[==========] 526 tests from 75 test cases ran. (24571 ms total)
[  PASSED  ] 526 tests.
```

#### 4.2.3 清理/重新编译

删除构建目录，重新执行[第 4.2.2 节](#422-执行ut测试)：   

```shell
rm -rf build_ut  
```

## 5. FAQ

### 5.1 编译时执行make为什么没有生成run包？   

有可能执行 cmake 时用了 `cmake ..`，cmake 命令说明如下：   

- `cmake ..`：只编译本工程，make install 会把东西装到 output/，但不会调用 makeself，所以不会生成 Ascend-mindstudio-sanitizer-xxx.run。
- `cmake ../cmake`：用的是 cmake/CMakeLists.txt 的「超级构建」，会先编译+安装，再执行 parser.py + makeself，在 output/ 里生成 .run 包。
