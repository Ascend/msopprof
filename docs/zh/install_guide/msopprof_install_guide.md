# MindStudio Ops Profiler 安装指南

<br>

## 1. 安装说明

本工具已集成于CANN中，若已安装CANN且无需更新此工具，可直接使用，无需按本文档安装。

若您的环境尚未安装CANN，请参见《[CANN 快速安装](https://www.hiascend.com/cann/download)》安装昇腾NPU驱动和CANN软件（包含Toolkit和ops包），并配置环境变量。

如需单独升级本工具或使用最新版本，您可通过以下三种方式进行安装：[在线安装](#21-在线安装)、[离线安装](#22-离线安装)、[源码安装](#23-源码安装)。

## 2. 安装方式

### 2.1 在线安装

若您的设备具备互联网访问能力，可通过一条命令自动完成工具的下载与安装。请参见昇腾社区MindStudio[下载](https://www.hiascend.com/developer/software/mindstudio/download)页面，选择对应的CANN版本，并在安装方式中选择“在线安装”，系统将引导您完成后续操作。

### 2.2 离线安装

对处于企业内网等无外网环境的设备，请先在可联网的机器上下载完整的离线安装包，再将其传输至目标设备进行安装。请参见昇腾社区MindStudio[下载](https://www.hiascend.com/developer/software/mindstudio/download)页面，选择对应的CANN版本，并在安装方式中选择“离线安装”，获取对应的安装包及操作指引。

### 2.3 源码安装

如需使用最新代码的功能，或对源码进行修改以增强功能，可下载本仓库代码，自行编译、打包工具并完成安装。

#### 2.3.1 环境准备

请按照以下文档进行环境配置：《[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)》。

#### 2.3.2 执行编译打包

- 克隆本仓库

    ```sh
    git clone https://gitcode.com/Ascend/msopprof.git
    ```

- 构建打包

    通过一键式脚本自动完成依赖仓库的下载与构建流程：

    ```shell
    cd msopprof
    python build.py
    ```

#### 2.3.3 安装

##### 2.3.3.1 准备 run 包

run 包将生成于 output 目录下，执行以下命令确保其具备可执行权限：

```shell
cd output
chmod +x mindstudio-opprof_<version>_<arch>.run
```

##### 2.3.3.2 安装

将 run 包拷贝到运行环境中（本机安装无需拷贝），执行如下安装操作：

```shell
./mindstudio-opprof_<version>_<arch>.run --run  
```

当回显包含以下信息时，表示软件包安装成功。

```text
mindstudio-opprof package install success!
```

若系统中已安装该工具的旧版本，安装过程中会提示是否替换；输入 "y" 可执行覆盖安装。

> [!NOTE]
> 
> 安装路径说明   
> 
> 若环境中已配置 `ASCEND_HOME_PATH` 环境变量，工具将安装至 `$ASCEND_HOME_PATH` 目录；
> 否则，默认安装至 `$HOME/Ascend` 目录；  
> 如需指定自定义安装路径，请使用 `--install-path` 选项，例如：
> `./mindstudio-opprof_<version>_<arch>.run --install-path=./xxx --run`，即可将该运行包安装至 `xxx` 目录。

##### 2.3.3.3 安装后配置
 
软件包安装成功后，需设置环境变量，确保算子功能可以正常运行。
 
```shell
export ASCEND_HOME_PATH=$HOME/Ascend  # 或export ASCEND_HOME_PATH=$PWD/xxx（指定路径安装场景）
export PATH=$ASCEND_HOME_PATH/bin:$PATH
export LD_LIBRARY_PATH=$ASCEND_HOME_PATH/lib64:$LD_LIBRARY_PATH
```

## 3. 卸载

可通过如下步骤卸载：

1. 下载脚本。

   ```bash
   curl -O https://inst.obs.cn-north-4.myhuaweicloud.com/26.0.0/ms_install.py
   ```

   > [!NOTE]
   >
   > - 需要联网环境才能下载，若环境不允许联网或处于离线状态，请先在可联网的环境下载该脚本后拷贝到目标设备。
   > - 若执行命令无响应或出现连接失败、SSL证书错误等问题，请参见[FAQ](https://www.hiascend.com/developer/blog/details/02176213671719317003)。

2. 执行卸载。

   ```bash
   python ms_install.py uninstall {tools_name}
   ```

   其中{tools_name}配置为需卸载的工具名称，可通过`python ms_install.py help`命令查询，在打印信息中的Available Tools字段下显示工具名称。

   卸载成功打印如下信息：

   ```ColdFusion
   Successfully uninstalled 1 tool ({tools_name})
   ```

## 4. 升级

升级即“先卸后装”。直接执行安装命令，工具将自动卸载旧版本，并引导您完成覆盖安装。
