<h1 align="center">MindStudio Ops Profiler</h1>

<div align="center">
<h2>Ascend AI Operator Tuning Tool</h2>

 [![Ascend](https://img.shields.io/badge/Community-MindStudio-blue.svg)](https://www.hiascend.com/developer/software/mindstudio) 
 [![License](https://badgen.net/badge/License/MulanPSL-2.0/blue)](./LICENSE)

</div>

## ✨ Latest Updates

<span style="font-size:14px;">

🔹  **[2025.12.31]**: MindStudio Ops Profiler is fully open-source.

</span>

## ️ ℹ️ Overview

MindStudio Ops Profiler (msOpProf, an operator tuning tool) is used to collect and analyze the key performance metrics of operators running on Ascend AI Processors. Based on the output profile data, you can quickly locate the hardware and software performance bottlenecks of operators, significantly improving the efficiency of operator performance analysis. Currently, profile data can be collected and automatically parsed in various running modes (real-device deployment or simulation) and input formats (executable files or operator binary .o files).

<div align="center">
  <h4>▶️ Quick demo</h4>
  <img src="./docs/en/figures/demo-msopprof.gif" alt="Quick demo" width="600">
  <p><sup>Figure: Deploying operators on the board and collecting profile data for simulation tuning</sup></p>
</div>

## ⚙️ Features

The tool provides two usage modes: msOpProf and msOpProf simulator.

| Feature| Description|
|---------|--------|
| **msOpProf mode**| It is suitable for performance analysis in the actual operating environment. You can directly analyze running operators without additional configuration, helping you quickly locate memory and performance bottlenecks of operators. This mode is especially suitable for the board environment.|
| **msOpProf simulator mode**| You need to configure environment variables and compilation options. This mode is suitable for detailed and in-depth performance analysis of operator behavior in the simulation environment.|

## 🚀 Quick Start

Quickly experience core functions. For details, see [msOpProf Quick Start](./docs/en/quick_start/msopprof_quick_start.md).

## 📦 Installation Guide

For msOpProf installation details, see the [msOpProf Installation Guide](./docs/en/install_guide/msopprof_install_guide.md).

## 📘 User Guide

For details about how to use the tool, see [msOpProf User Guide](./docs/en/user_guide/msopprof_user_guide.md) or [msOpProf Simulator User Guide](./docs/en/user_guide/msopprof_simulator_user_guide.md).

## 💡 Typical Cases

msOpProf helps you understand and use the tool through some typical cases. For specific cases, see [msOpProf Typical Cases](./docs/en/best_practices/typical_cases.md).

## 🛠️ Contribution Guide

You are welcome to contribute to the project. For details, see [Contribution Guide](./docs/en/contributing/contributing_guide.md). 

## ⚖️ Related Information

🔹 [Release Notes](./docs/en/release_notes/release_notes.md) 
🔹 [License Notice](./docs/en/legal/license_notice.md) 
🔹 [Security Statement](./docs/en/legal/security_statement.md) 
🔹 [Disclaimer](./docs/en/legal/disclaimer.md) 

## 🤝 Suggestions and Communication

You are welcome to contribute to the community. If you have any questions or suggestions, please submit [issues](https://gitcode.com/Ascend/msopprof/issues). We will reply as soon as possible. Thank you for your support.

|                                      📱 Follow the MindStudio Wechat Account                                      | 💬 More Communication and Support                                                                                                                                                                                                                                                                                                                                                                                                                    |
|:-----------------------------------------------------------------------------------------------:|:-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| <img src="https://gitcode.com/Ascend/msot/blob/master/docs/zh/figures/readme/officialAccount.png" width="120"><br><sub>*Scan the QR code to follow us and get the latest updates.*</sub>| 💡  **Join the WeChat group**:<br>Follow the Wechat account and reply "communication group" to obtain the QR code for joining the group.<br><br>🛠️ ️ **Other channels**:<br><br>|

## 🙏 Acknowledgements

This tool is jointly developed by the following Huawei departments:   
🔹 Ascend Computing MindStudio Development Department 
🔹 Ascend Computing Ecosystem Enablement Department 
🔹 Huawei Cloud AI Compute Service 
🔹 Compiler Technologies Lab, 2012 Labs 
🔹 Markov Lab, 2012 Labs 
Thank you to everyone in the community for your PRs. We warmly welcome your contributions.
