# msOpProf Installation Guide

<br>

## 1. Installation Instructions

This tool is integrated into CANN. If CANN is already installed and the tool does not need to be updated, you can use it directly without following the installation procedure described in this document.

If CANN is not installed in your environment, see [CANN Quick Installation](https://www.hiascend.com/en/cann/download) to install the Ascend NPU driver and CANN software (including the Toolkit and Ops packages), and configure the environment variables.

To upgrade this tool separately or use the latest version, use the following three installation methods: [Online Installation](#21-online-installation), [Offline Installation](#22-offline-installation), and [Source Code Installation](#23-source-code-installation).

## 2. Installation Methods

### 2.1 Online Installation

If your device has Internet access, you can use a single command to automatically download and install the tool. Visit the MindStudio [download](https://www.hiascend.com/en/developer/software/mindstudio/download) page on the Ascend community, select the corresponding CANN version, choose **Online Installation** as the installation method, and the system guides you through the remaining steps.

### 2.2 Offline Installation

For devices in an intranet or other environments without Internet access, first download the complete offline installation package on a machine with Internet access, then transfer it to the target device for installation. Visit the MindStudio [download](https://www.hiascend.com/en/developer/software/mindstudio/download) page on the Ascend community, select the corresponding CANN version, choose **Offline Installation** as the installation method, and obtain the installation package and instructions.

### 2.3 Source Code Installation

To use the latest code features or modify the source code to enhance functions, download the code from this repository, build and package the tool, and install it.

#### 2.3.1 Environment Setup

All source code compilation uses the standard MindStudio build environment.

- For daily development or when you use a released image, refer to the [MindStudio Tool Development Environment Installation Guide](https://atomgit.com/Ascend/msot/blob/26.2.0/docs/en/common/dev_env_setup.md).
- If you need to reproduce the environment from a base operating system, or to perform source code build verification or unit test verification, you must refer to the [MindStudio Unified Build Image Creation Guide](https://atomgit.com/Ascend/msot/blob/26.2.0/docs/en/common/docker_image_build_guide.md) to build the environment image on site from the openEuler base image.

The source code compilation and unit test commands later in this document all run in the container of the specified image or of the environment image built on site. The CANN software package version, the GCC version, and the Python version are subject to the *Unified Build Image Creation Guide*, and this repository does not maintain them separately.

After the image is built, you must use the `ctr_in.py` command provided in Chapter 7 of the *Unified Build Image Creation Guide* to start and enter the container in an interactive terminal. Do not use a plain `docker run` to create a container, and do not use `docker exec <container_name> bash -c '<command>'` to replace the interactive environment. Otherwise, the initialization of the Python, GCC, and CANN environments may be skipped.

After you enter the interactive container shell opened by `ctr_in.py`, run the following command to clone this repository:

```bash
cd ~
git clone https://atomgit.com/Ascend/msopprof.git -b 26.2.0
```

#### 2.3.2 Compilation

Stay in the same interactive container shell opened by `ctr_in.py`, and run the following command in the repository root directory to automatically download dependencies and build:

```bash
cd ~/msopprof
python3 build.py
```

After the build succeeds, the installation package is generated in the `artifacts/` directory.

#### 2.3.3 Unit Tests (Optional)

This step is not required for installation. If you need to verify the basic functions of the code, you can run the unit tests:

```bash
cd ~/msopprof
python3 build.py test
```

If the return code of the command is 0 and no test case fails, the unit tests pass.

#### 2.3.4 Installation

##### 2.3.4.1 Preparing the `.run` Package

Before installing the software package, add the execute permission to the `.run` package. Go to the directory where the `.run` package is stored and run the following command to add the execute permission:

```bash
chmod +x mindstudio-opprof_<version>_<arch>.run
```

##### 2.3.4.2 Installation

Copy the `.run` package to the operating environment and run the following command to install it:

```bash
./mindstudio-opprof_<version>_<arch>.run --install
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
> - If the `ASCEND_HOME_PATH` environment variable has been configured in the environment, the tool is installed in the `${ASCEND_HOME_PATH}` directory. Otherwise, the tool is installed in the `${HOME}/Ascend` directory by default.
>
> - To specify a custom installation path, add the `--install-path` option. For example, `./mindstudio-opprof_<version>_<arch>.run --install-path=./test --install` installs the `.run` package to the `test` directory in the current directory.

## 3. Verifying the Installation

After the installation is complete, run the following command to verify that the tool is successfully installed:

```bash
msopprof --help
```

If no error is reported and the help information is displayed, the installation is successful.

## 4. Uninstallation

You can uninstall the tool using the following steps:

1. Download the script.

   ```bash
   curl -O https://inst.obs.cn-north-4.myhuaweicloud.com/26.2.0/ms_install.py
   ```

   > [!NOTE]
   >
   > - An Internet connection is required for download. If your environment does not allow Internet access or is offline, download the script in an environment with Internet access first and then copy it to the target device.
   > - If the command does not respond or returns a connection failure, SSL certificate error, or other issues, see the [FAQ](https://www.hiascend.com/developer/blog/details/02176213671719317003).

2. Run the uninstallation.

   ```bash
   python3 ms_install.py uninstall {tools_name}
   ```

   In this command, `{tools_name}` specifies the name of the tool to be uninstalled. You can query the tool name using the `python3 ms_install.py help` command. The tool names are displayed under the `Available Tools` field in the output.

   When the following information is displayed, the software is successfully uninstalled:

   ```text
   Successfully uninstalled 1 tool ({tools_name})
   ```

## 5. Upgrade

The upgrade process follows the "uninstall before install" approach. Run the installation command directly, and the tool automatically uninstalls the old version and guides you through the overwrite installation.

You can run the `msopprof --version` command to check the version information of the current environment, and then select the version to upgrade to. Pay attention to the version compatibility when upgrading. See the [Release Notes](https://atomgit.com/Ascend/release-management/blob/master/MindStudio/26.2.0/release_notes.md) for details.
