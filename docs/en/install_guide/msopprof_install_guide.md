# **MindStudio Ops Profiler Installation Guide**

<br>

## 1. Binary Installation

The MindStudio toolchain is integrated into the CANN package for release. You can install it in either of the following ways.

### Method 1: Install the software according to the CANN official document

For details, see <a href="https://www.hiascend.com/document/detail/zh/canncommercial/850/softwareinst" target="_blank"> CANN Installation Guide </a>.
Perform the installation and configuration step by step according to the document.

### Method 2: Use the official CANN container image

Visit <a href="https://www.hiascend.com/developer/ascendhub/detail/17da20d1c2b6493cb38765adeba85884" target="_blank">CANN official image repository</a>.
Pull the image and start the container according to the instructions in the repository.

<br>

## 2. Source Code Installation

To use the functions of the latest code or modify the source code to enhance functions, you can download the code from this repository, build and package the tool, and install it.

### 2.1 Environment Setup

Set up the environment by referring to the [Operator Tool Development Environment Setup Guide](https://gitcode.com/Ascend/msot/blob/master/docs/en/common/dev_env_setup.md).

### 2.2 Building and Packaging

Run the one-click script to automatically download and build the dependency repository:

```shell
python build.py
```

### 2.3 Installation and Uninstallation

#### 2.3.1 Preparing the .run Package

The .whl package is generated in the `output` directory. Run the following commands to ensure that the .run package has the execute permission:

```shell
cd output
chmod +x mindstudio-opprof_<version>_<arch>.run
```

#### 2.3.2 Installation

Copy the .run package to the operating environment (not required for local installation) and perform the following operation:

```shell
./mindstudio-opprof_<version>_<arch>.run --run  
```

When the following information is displayed, the software package is successfully installed:

```text
mindstudio-opprof package install success!
```

If an earlier version of the tool has been installed in the system, a message will be displayed during the installation asking you whether to replace it. Enter "y" to perform an overwrite installation.
>[!NOTE]NOTE  
> If the `ASCEND_HOME_PATH` environment variable has been configured in the environment, the tool will be installed in the `$ASCEND_HOME_PATH` directory.
> Otherwise, the tool will be installed in the `$HOME/Ascend` directory by default. 
> To specify a custom installation path, use the `--install-path` option. For example:
> `./mindstudio-opprof_<version>_<arch>.run --install-path=./xxx --run`  installs the runfile to the `xxx` directory.

### 2.3.3 Post-installation Configuration
 
After the software package is installed, configure the environment variables to ensure that the operators function correctly.
 
```shell
export ASCEND_HOME_PATH=$HOME/Ascend # For custom installation paths, run "export ASCEND_HOME_PATH=$PWD/xxx".
export PATH=$ASCEND_HOME_PATH/bin:$PATH
export LD_LIBRARY_PATH=$ASCEND_HOME_PATH/lib64:$LD_LIBRARY_PATH
```

#### 2.3.4 Uninstallation

You can run the following command to uninstall the tool:

```shell
./mindstudio-opprof_<version>_<arch>.run --uninstall
```

When the following information is displayed, the software package is successfully uninstalled:

```text
mindstudio-opprof uninstall success!
```

>[!NOTE]NOTE  
> By default, the tool is uninstalled from the `$HOME/Ascend` directory. If a custom path is specified using `--install-path` during the installation,
> explicitly add the `--install-path` option during uninstallation. For example:
> `./mindstudio-opprof_<version>_<arch>.run --install-path=./xxx --uninstall`.

If the .run package has been deleted, run the following command to uninstall the software:
 
```shell
bash $HOME/Ascend/share/info/mindstudio-opprof/script/uninstall.sh # For custom installation paths, run "bash ./xxx/share/info/mindstudio-opprof/script/uninstall.sh".
```

#### 2.3.4 Upgrade

The upgrade process essentially involves uninstalling the old version and installing the new version, which is the same as the overwrite installation method described in Section [2.3.2 Installation](#232-installation).

> [!NOTE]NOTE 
> By default, the upgrade targets the `mindstudio-opprof` directory in `$HOME/Ascend`. If the previous version is installed to a custom path, use the `--install-path` option, for example, `./mindstudio-opprof_<version>_<arch>.run  --install-path=./xxx --run`, where `xxx` is the previous installation directory.

## Upgrade
 
To replace an existing mindstudio-opprof package with the newly built .run package, run the following command:
 
```shell
./mindstudio-opprof_<version>_<arch>.run --run 
```  

During the process, you will be prompted "do you want to overwrite current installation? [y/n]." Enter `y` to proceed with the automatic upgrade.
