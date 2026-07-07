# MindStudio Ops Profiler Installation Guide

<br>

## 1. Installation Instructions

This tool is integrated into CANN. If CANN is already installed and the tool does not need to be updated, you can use it directly without following the installation procedure described in this document.

If CANN is not installed in your environment, see [CANN Quick Installation](https://www.hiascend.com/cann/download) to install the Ascend NPU driver and CANN software (including the Toolkit and Ops packages), and configure the environment variables.

To upgrade this tool separately or use the latest version, use the following three installation methods: [Online Installation](#21-online-installation), [Offline Installation](#22-offline-installation), and [Source Code Installation](#23-source-code-installation).

## 2. Installation Methods

### 2.1 Online Installation

If your device has Internet access, you can use a single command to automatically download and install the tool. Visit the MindStudio [download](https://www.hiascend.com/developer/software/mindstudio/download) page on the Ascend community, select the corresponding CANN version, choose **Online Installation** as the installation method, and the system guides you through the remaining steps.

### 2.2 Offline Installation

For devices in an intranet or other environments without Internet access, first download the complete offline installation package on a machine with Internet access, then transfer it to the target device for installation. Visit the MindStudio [download](https://www.hiascend.com/developer/software/mindstudio/download) page on the Ascend community, select the corresponding CANN version, choose **Offline Installation** as the installation method, and obtain the installation package and instructions.

### 2.3 Source Code Installation

To use the latest code features or modify the source code to enhance functions, download the code from this repository, build and package the tool, and install it.

#### 2.3.1 Environment Setup

Set up the environment by referring to the [Operator Tool Development Environment Setup Guide](https://gitcode.com/Ascend/msot/blob/26.0.0/docs/en/common/dev_env_setup.md).

#### 2.3.2 Building and Packaging

- Clone this repository.

    ```sh
    git clone https://gitcode.com/Ascend/msopprof.git -b 26.0.0
    ```

- Build and package.

    Run the one-click script to automatically download and build the dependency repository:

    ```shell
    cd msopprof
    python build.py
    ```

#### 2.3.3 Installation

##### 2.3.3.1 Preparing the .run Package

The .run package is generated in the `output` directory. Run the following commands to ensure that the .run package has the execute permission:

```shell
cd output
chmod +x mindstudio-opprof_<version>_<arch>.run
```

##### 2.3.3.2 Installation

Copy the .run package to the operating environment (not required for local installation) and perform the following operation:

```shell
./mindstudio-opprof_<version>_<arch>.run --run  
```

When the following information is displayed, the software package is successfully installed:

```text
mindstudio-opprof package install success!
```

If an earlier version of the tool has been installed in the system, a message will be displayed during the installation asking you whether to replace it. Enter `y` to perform an overwrite installation.

> [!NOTE]
>
> Installation path note:
>
> If the `ASCEND_HOME_PATH` environment variable has been configured in the environment, the tool is installed in the `$ASCEND_HOME_PATH` directory.
> Otherwise, the tool is installed in the `$HOME/Ascend` directory by default.  
> To specify a custom installation path, use the `--install-path` option. For example:
> `./mindstudio-opprof_<version>_<arch>.run --install-path=./xxx --run` installs the .run package to the `xxx` directory.

##### 2.3.3.3 Post-Installation Configuration

After the software package is installed, configure the environment variables to ensure that the operators function correctly.

```shell
export ASCEND_HOME_PATH=$HOME/Ascend  # For custom installation paths, run "export ASCEND_HOME_PATH=$PWD/xxx"
export PATH=$ASCEND_HOME_PATH/bin:$PATH
export LD_LIBRARY_PATH=$ASCEND_HOME_PATH/lib64:$LD_LIBRARY_PATH
```

## 3. Verifying the Installation

After the installation is complete, run the following command to verify that the tool is successfully installed:

```shell
msopprof --help
```

If no error is reported and the help information is displayed, the installation is successful.

## 4. Uninstallation

You can uninstall the tool using the following steps:

1. Download the script.

   ```bash
   curl -O https://inst.obs.cn-north-4.myhuaweicloud.com/26.0.0/ms_install.py
   ```

   > [!NOTE]
   >
   > - An Internet connection is required for download. If your environment does not allow Internet access or is offline, download the script in an environment with Internet access first and then copy it to the target device.
   > - If the command does not respond or returns a connection failure, SSL certificate error, or other issues, see the [FAQ](https://www.hiascend.com/developer/blog/details/02176213671719317003).

2. Run the uninstallation.

   ```bash
   python ms_install.py uninstall {tools_name}
   ```

   In this command, `{tools_name}` specifies the name of the tool to be uninstalled. You can query the tool name using the `python ms_install.py help` command. The tool names are displayed under the `Available Tools` field in the output.

   When the following information is displayed, the software is successfully uninstalled:

   ```ColdFusion
   Successfully uninstalled 1 tool ({tools_name})
   ```

## 5. Upgrade

The upgrade process follows the "uninstall before install" approach. Run the installation command directly, and the tool automatically uninstalls the old version and guides you through the overwrite installation.

You can run the `msopprof --version` command to check the version information of the current environment, and then select the version to upgrade to. Pay attention to the version compatibility when upgrading. See the [Release Notes](https://gitcode.com/Ascend/release-management/blob/master/MindStudio/26.0.0/release_notes.md) for details.
