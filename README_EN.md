<h1 align="center">MindStudio Ops Profiler</h1>

<div align="center">
<p><b><span style="font-size:24px;">Ascend AI Operator Tuning Tool</span></b></p>

 [![License](https://badgen.net/badge/快速入门/QuickStart/blue)](./docs/en/quick_start/msopprof_quick_start.md)
 [![License](https://badgen.net/badge/精确搜索/ReadTheDocs/blue)](https://mindstudio-operator-tools-docs.readthedocs.io/zh-cn/latest/)
 [![License](https://badgen.net/badge/AI问答/DeepWiki/blue)](https://deepwiki.com/mindstudio-docs/master)
 [![License](https://badgen.net/badge/AI问答/ZRead/blue)](https://zread.ai/mindstudio-docs/master)
 [![License](https://badgen.net/badge/昇腾社区/Community/blue)](https://www.hiascend.com/cn/developer/software/mindstudio)
 [![License](https://badgen.net/badge/报告问题/Issues/blue)](https://gitcode.com/Ascend/msopprof/issues)

</div>

English | [简体中文](README.md)

## ✨ Latest Updates

<span style="font-size:14px;">

🔹 **[Dec 31, 2025]**: MindStudio Ops Profiler is fully open-source.

</span>

## ️ ℹ️ Overview

MindStudio Ops Profiler (msOpProf, an operator tuning tool) is used to collect and analyze the key performance metrics of operators running on Ascend AI Processors. Based on the output profile data, you can quickly locate the hardware and software performance bottlenecks of operators, significantly improving the efficiency of operator performance analysis. Currently, profile data can be collected and automatically parsed in various running modes (real-device deployment or simulation) and input formats (executable files or operator binary .o files).

<div align="center">
  <h4>▶️ Quick Demo</h4>
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

For details about how to use the tool, see the [msOpProf User Guide](./docs/en/user_guide/msopprof_user_guide.md) or the [msOpProf Simulator User Guide](./docs/en/user_guide/msopprof_simulator_user_guide.md).

## 💡 Typical Cases

msOpProf helps you understand and use the tool through some typical cases. For specific cases, see [msOpProf Typical Cases](./docs/en/best_practices/typical_cases.md).

## 🌌 Intelligent Search

To improve documentation search efficiency, we provide multiple efficient search methods:  
🔹 [AI Q&A (DeepWiki)](https://deepwiki.com/mindstudio-docs/master): Natural language Q&A that helps you quickly grasp the project architecture and module relationships.   
🔹 [AI Q&A (ZRead)](https://zread.ai/mindstudio-docs/master): A better Chinese Q&A experience that precisely locates feature usage and details.   
🔹 [Precise Search (ReadTheDocs)](https://mindstudio-operator-tools-docs.readthedocs.io/zh-cn/latest/): Keyword-based full-text search that takes you directly to interfaces, parameters, and error messages.  

## 🛠️ Contribution Guide

You are welcome to contribute to the project. For details, see the [Contribution Guide](./docs/en/contributing/contributing_guide.md). 

## ⚖️ Related Information

🔹 [Release Notes](./docs/en/release_notes/release_notes.md)   
🔹 [License Notice](./docs/en/legal/license_notice.md)   
🔹 [Security Statement](./docs/en/legal/security_statement.md)   
🔹 [Disclaimer](./docs/en/legal/disclaimer.md) 

## 🤝 Suggestions and Communication

You are welcome to contribute to the community. If you have any questions or suggestions, please submit [issues](https://gitcode.com/Ascend/msopprof/issues). We will reply as soon as possible. Thank you for your support.

|Instant Interaction (WeChat Group)|Official Information (WeChat Official Account)|In-Depth Support (Assistant/Forum)|
|:---:|:---:|:---:|
| <img src="https://raw.gitcode.com/Ascend/docs/files/master/common/Writing_Template/figures/qr_code_wechat_work.png" width="120"><br><sub>*Scan the QR code to join the technical communication group.*</sub> | <img src="https://raw.gitcode.com/Ascend/docs/files/master/common/Writing_Template/figures/qr_code_wechat_official_account.png" width="120"><br><sub>*Scan the QR code to follow the official WeChat account.*</sub> | Scan the QR code to join the group and follow the official account to access the fastest communication platform for MindStudio users and developers:<br> **Quick questions:** Discuss technical issues with community members in real time.<br>**Stay informed:** Receive version release and feature update notifications as soon as they are published.<br> **Share experience:** Exchange best practices and hands-on insights with a wide range of developers.  <br> <br> **More support channels:** 👉 Ascend Assistant: [![WeChat](https://img.shields.io/badge/WeChat-07C160?style=flat-square&logo=wechat&logoColor=white)](https://gitcode.com/Ascend/msit/blob/master/docs/zh/figures/readme/xiaozhushou.png) 👉 Ascend Forum: [![Website](https://img.shields.io/badge/Website-%231e37ff?style=flat-square&logo=RSS&logoColor=white)](https://www.hiascend.com/forum/) |

## 🙏 Acknowledgements

This tool is jointly developed by the following Huawei departments:   
🔹 Ascend Computing MindStudio Development Department   
🔹 Ascend Computing Ecosystem Enablement Department   
🔹 Huawei Cloud AI Compute Service  
🔹 Compiler Technologies Lab, 2012 Labs  
🔹 Markov Lab, 2012 Labs   
Thank you to everyone in the community for your PRs. We warmly welcome your contributions.
