# Constraints and Precautions for MindStudio Ops Profiler

## Environment Constraints

- msOpProf depends on the `msopprof` executable file in the CANN package. The API usage in this file is the same as that in `msprof op`. This file is provided by the CANN package and does not need to be installed separately.
- To use the MindStudio Insight tool to view files output by msOpProf, you need to install the MindStudio Insight software package separately. For installation steps, see the [MindStudio Insight Installation Guide](https://gitcode.com/Ascend/msinsight/blob/26.1.0/docs/en/install_guide/mindstudio_insight_install_guide.md).

## Running Constraints

- After you press `Ctrl+C`, the operator execution stops, and the tool generates a profile data file based on existing information. If you do not need to generate the file, press `Ctrl+C` again.
- Do not initiate more than one profile data collection task on the same device.
- You are advised to collect profile data within 5 minutes and ensure that the set memory size is greater than 20 GB (for example, `docker run --memory=20g container_name`).
- Ensure that the profile data is stored in the current user directory that does not contain soft links. Otherwise, security issues may occur.

## File Permissions

- If you do not specify the `--output` parameter, the tool writes the results to a default file in the current working directory. To avoid permission issues, ensure that users in the group and other groups do not have the write permission on the parent directory of the current path.

## Security Precautions

- You need to ensure the execution security of executable files or applications.
  - You are advised to restrict the operation permission on executable files or applications to avoid privilege escalation risks.
  - Avoid high-risk operations (such as deleting files, deleting directories, changing passwords, and running privilege escalation commands) to prevent security risks.
- The tool loads shared libraries (`.so` files) from `LD_LIBRARY_PATH` while running. Before using the tool, ensure that the `LD_LIBRARY_PATH` environment variable is safe and trustworthy, the paths it points to do not involve soft links, and the permissions and owners of the paths meet security expectations and cannot be tampered with by third parties. Otherwise, arbitrary code injection may occur.

## General

- Before using msOpProf or msOpProf simulator, ensure that the application functions properly.
