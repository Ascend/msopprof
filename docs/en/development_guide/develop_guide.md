# msOpProf Development Guide

<br>

## 1. Prerequisites

### 1.1 Prerequisite Knowledge

For details about the code framework and core process, see the [msOpProf Architecture Design Specifications](./architecture.md).

### 1.2 Environment Preparation

Follow the [msOpProf Installation Guide — Source Installation](../install_guide/msopprof_install_guide.md#231-environment-setup) to set up the build and test environment.

> **Note:** The environment image build method and the supporting software versions are maintained by the *MindStudio Unified Build Image Creation Guide* and are not redefined in this repository.

## 2. Build

Choose one of the following build methods as needed:

| Method | Application Scenario | Features |
|------|---------|------|
| One-click script | Initial build and CI/CD pipeline | Zero configuration, completes the entire process in one step |
| Step-by-step build | Routine development and incremental build | Executes individual steps as needed, which is more efficient. |

### 2.1 One-Click Script

Run the following command in the project root directory:

```bash
python3 build.py
```

The script automatically completes the entire process of downloading dependencies, building, and packaging.

### 2.2 Step-by-Step Build

#### Downloading Dependencies

```bash
python3 download_dependencies.py
```

#### Building

```bash
python3 build.py local
```

#### Build Artifacts

After a successful build, the output is generated in the `artifacts` directory.

#### Build Cleanup

To perform a full rebuild, delete the build directory and run the preceding steps again:

```bash
rm -rf build
```

## 3. Unit Tests

### 3.1 Running Tests

```bash
python3 build.py test
```

A command return code of 0 with no failed test cases indicates that the unit tests pass. If any `FAILED` case exists, locate the cause of the failure from the logs.

### 3.2 Cleaning the Test Environment

Delete the UT build directory and run the tests again:

```bash
rm -rf build_ut
```
