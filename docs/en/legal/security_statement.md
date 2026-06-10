# Security Declaration

## System Security Hardening

Enable **address space layout randomization** (ASLR) level 2 in the system. Run the following command to enable it:

```sh
echo 2 > /proc/sys/kernel/randomize_va_space
```

## User Account Recommendations

1. All tools in this repository are designed to run with minimal permissions. For security reasons, do not use `root` or other privileged accounts. Always install and execute tools as a regular user.

2. If a tool depends on CANN, install the CANN package under the same non-privileged user. After running the `source` command, do not modify the environment variables in `set_env.sh`.

3. Before using any tools, set umask to `0027` or stricter to ensure generated files meet minimum security requirements.

4. This tool is for development. No restrictions are placed on the owner and permissions for installation, nor on the owner and permissions for the file objects that the tool processes. You need to assign appropriate owners and permissions based on the application scenario and ensure that the file content processed by the tool is secure and trustworthy.

5. When you provide input to the tool, ensure that the content is secure and trustworthy and avoid symbolic links. Convert any file to a real absolute path before you input it into the tool.

## File Permission Control

 1. When providing input files to the tool, it is recommended that the file owner matches the process owner of the tool and that file permissions restrict write access for `group` and `others`. By default, tool files written to the drive are not writable by group users and other users. You can manually control the permissions for the generated files as needed.

 2. Proper permission control is essential during installation and use. For details, see the following table.

## File Permission Reference 

| Type                              | Maximum Linux Permissions|
| ---------------------------------- | ------------------- |
| Home directory                        | 750 (rwxr-x---)   |
| Program files (including scripts and library files)    | 550 (r-xr-x---)   |
| Program file directory                      | 550 (r-xr-x---)   |
| Configuration files                          | 640 (rw-r-----)   |
| Configuration file directory                      | 750 (rwxr-x---)   |
| Log files (recorded or archived)    | 440 (r--r-----)   |
| Log files (being recorded)                | 640 (rw-r-----)   |
| Log file directory                      | 750 (rwxr-x---)   |
| Debug files                         | 640 (rw-r-----)   |
| Debug file directory                     | 750 (rwxr-x---)   |
| Temporary file directory                      | 750 (rwxr-x---)   |
| Maintenance and upgrade file directory                  | 770 (rwxrwx---)   |
| Service data files                        | 640 (rw-r-----)   |
| Service data file directory                  | 750 (rwxr-x---)   |
| Key components, private keys, certificates, and ciphertext file directory| 700 (rwx------)   |
| Key components, private keys, certificates, and ciphertext files    | 600 (rw-------)   |
| Interfaces and scripts for encryption and decryption            | 500 (r-x------)   |

## Vulnerability Security Statement 

For details, see [MindStudio Vulnerability Handling Mechanism Description](./vulnerability_handling_procedure.md).

## Data security

1. During tool use, some functions related to code lines may access customer operator code. If the operator code must remain confidential and cannot be leaked, delete the corresponding deliverables in a timely manner after use to prevent information leakage.

## Build Security

This tool supports building from source. During the build process, the system downloads third-party dependencies, executes shell build scripts, and generates temporary program files and build directories. To reduce security risks, you can perform permission control on files within the source code directory. During the build process, you can modify build scripts as needed to avoid security risks and ensure the security of the build results.

## Runtime Security

1. When loading a dataset that exceeds available memory, or when monitoring runs too long and generated data fills the available drive space, the tool may exit unexpectedly.

2. If an exception occurs during operation, the tool will exit the process and print error messages. This is expected behavior. You are advised to locate the specific cause of the error based on the error prompts, such as by viewing log files or result files generated during the collection and parsing process.

3. During tool use, no security validation is performed on the user-input programs. You need to ensure the security of the programs.

## Public Network Addresses

The tool does not involve the use of public IP addresses.

## Public APIs

This project is developed using C++. The provided external APIs are disclosed in the documentation. Dynamic libraries do not provide services directly. The exposed APIs are for internal use and user calls are not recommended.

The interfaces exposed by the dynamic library compiled from `msopcommon` are for internal use. You are advised not to modify or use them.

## Usage of Safe Functions

While unsafe functions are not forcibly disabled, you are advised to use their safe variants that explicitly take a buffer length parameter, for example, `memset_s` and `memcpy_s`.

## Communication security hardening

This tool does not involve remote communication. However, you are advised to run it in a secure network environment, for example, behind a firewall or within a local area network (LAN). Be mindful of potential communication security risks when using third-party software.

## Communication matrix

External port communication is not involved.
