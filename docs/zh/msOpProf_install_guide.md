# **MindStudio Ops Profiler安装指南**
 
## 安装说明
 
MindStudio Ops Profiler（算子调优工具，msOpProf）用于采集和分析运行在昇腾AI处理器上算子的关键性能指标，本文主要介绍msOpProf工具的安装方法。
 
## 安装前准备
 
**配置用户名和密钥**
 
为了确保代码能够下载成功，需提前在环境中配置git仓库的用户名和密码。
 
执行以下命令，配置git存储用户密码，并通过git submodule下载.gitmodules中的子仓，在下载过程中可能会提示需要输入用户名和密码，输入后git会记住授权信息，后续就不会再需要输入用户名和密码了。
 
```shell
git config --global credential.helper store
git submodule update --init --recursive --depth=1
```
 
**准备工具**
 
- 软件包构建之前，需要确保已安装编译器bisheng，并且其可执行文件所在路径在环境变量${PATH}中。请确保bisheng的版本为2025-11-25T20:00:35+08:00 clang version 15.0.5 (clang-5c68a1cb1231 flang-5c68a1cb1231)或更新的版本。
 
- 编译工具CMake版本应大于或等于3.20.2，小于或等于3.31.10。
 
- 安装Python库的numpy依赖，执行```pip install numpy```安装。
 
## 安装步骤
 
### 软件包构建
 
- 命令行方式  
  执行如下命令，构建软件包。

    ```shell
    mkdir build
    cd build
    cmake ..  && make -j8 install  # 该命令只做编译，如果要生成run包，请使用cmake ../cmake && make -j8
    ```
 
- 一键式脚本方式  
  通过一键式脚本构建软件包。

    ```shell
    python build.py
    ```

    > [!NOTE]  说明  
    > 如果本地更改了依赖子仓中的代码，不想构建过程中执行更新动作，可以执行```python build.py local```。
 
### UT测试
 
- 命令行方式  
 
    ```shell
    mkdir build_ut
    cd build_ut
    cmake .. -DBUILD_TEST=on
    make -j8 install
    ./test/ut/msopt_test --gtest_output=xml:test_detail.xml
    ```
 
- 一键式脚本方式  
 
    ```shell
    python build.py test
    ```
 
### 安装软件包  
 
1. 软件包构建成功后，生成的是run包。安装前需给run包添加可执行权限。
 
    ```shell
    chmod +x Ascend-mindstudio-opprof-*.run
    ```
 
2. run包默认保存在output目录下，需将run包拷贝到运行环境中，执行以下命令安装。
 
    ```shell
    ./Ascend-mindstudio-opprof-*.run --run   
    ```
 
    > [!NOTE]  说明  
    > 如果环境中配置过ASCEND_HOME_PATH变量，则会安装到\${ASCEND_HOME_PATH}目录下；否则会默认安装到${HOME}/Ascend目录下。<br>
    > 如果要指定路径安装，则需添加```--install-path```，例如```./Ascend-mindstudio-opprof-*.run  --install-path=./test --run```，则将此run包安装到当前目录下的test目录下。
 
## 安装后配置
 
软件包安装成功后，需设置环境变量，确保算子功能可以正常运行。
 
```shell
export ASCEND_HOME_PATH=$HOME/Ascend  # 或export ASCEND_HOME_PATH=$PWD/xxx（指定路径安装场景）
export PATH=$ASCEND_HOME_PATH/bin:$PATH
export LD_LIBRARY_PATH=$ASCEND_HOME_PATH/lib64:$LD_LIBRARY_PATH
```
 
## 升级
 
如需使用构建产物run包替换运行环境原有已安装的mindstudio-opprof包，执行如下安装操作：
 
```shell
./Ascend-mindstudio-opprof-*.run --run 
```  
 
> [!NOTE]  说明
>
> - 默认会升级到${HOME}/Ascend目录下的mindstudio-opprof，如果老版本是通过指定路径安装的，则需添加```--install-path```，例如```./Ascend-mindstudio-opprof-*.run  --install-path=./test --run```，其中test是老版本的安装路径。
> - 安装过程中，会提示是否替换原有安装包```do you want to overwrite current installation? [y/n]```，输入y后，安装包会自动完成升级操作。
 
## 卸载
 
执行以下命令，卸载软件。
 
```shell
./Ascend-mindstudio-opprof-*.run --uninstall 
```
 
> [!NOTE]  说明  
> 默认会在${HOME}/Ascend目录下卸载，如果安装时通过```--install-path```指定了安装路径，则卸载时也需添加```--install-path```，例如```./Ascend-mindstudio-opprof-*.run  --install-path=./test --uninstall```。
 
如果run包已经删除，则可执行如下命令，卸载软件。
 
```shell
bash $HOME/Ascend/share/info/mindstudio-opprof/script/uninstall.sh  # 或bash ./xxx/share/info/mindstudio-opprof/script/uninstall.sh（指定路径安装场景）
```
