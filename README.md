<h1 align="center">MindStudio Ops Profiler</h1>

<div align="center">
<h2>昇腾 AI 算子调优工具</h2>

 [![Ascend](https://img.shields.io/badge/Community-MindStudio-blue.svg)](https://www.hiascend.com/developer/software/mindstudio) 
 [![License](https://badgen.net/badge/License/MulanPSL-2.0/blue)](./docs/LICENSE)

</div>

## ✨ 最新消息

- [2025.12.30]：MindStudio Ops Profiler项目首次上线。

<br>

## ️ ℹ️ 简介

MindStudio Ops Profiler（算子调优工具，msOpProf）用于采集和分析运行在昇腾AI处理器上算子的关键性能指标，用户可根据输出的性能数据，快速定位算子的软、硬件性能瓶颈，提升算子性能的分析效率。

当前支持基于不同运行模式（上板或仿真）和不同文件形式（可执行文件或算子二进制.o文件）进行性能数据的采集和自动解析。

## ⚙️ 功能介绍

msOpProf工具包含msOpProf和msOpProf simulator两种使用方式，协助用户定位算子内存、算子代码以及算子指令的异常，实现全方位的算子调优。

- [msOpProf模式](./docs/zh/user_guide/msopprof_user_guide.md)：适用于实际运行环境中的性能分析，可协助用户定位算子内存和性能瓶颈。直接分析运行中的算子，无需额外配置，适合在板环境中快速定位算子性能问题。

- [msOpProf simulator模式](./docs/zh/user_guide/msopprof_simulator_user_guide.md)：需配置环境变量和编译选项，适合在仿真环境中详细分析算子行为。

## 🚀 快速入门

快速体验核心功能，请参见[《msOpProf 快速入门》](./docs/zh/quick_start/msopprof_quick_start.md)。

## 📦 安装指南

msOpProf工具安装操作请参见[《msOpProf 安装指南》](./docs/zh/install_guide/msopprof_install_guide.md)。

## 📘 使用指南

工具的详细使用方法，请参见 [《msOpProf 使用指南》](./docs/zh/user_guide/msopprof_user_guide.md)

## 💡 典型案例

msOpProf通过一些典型案例帮助您理解并使用工具，具体案例请参见[《msOpProf 典型案例》](./docs/zh/best_practices/typical_cases.md)。

## 🛠️ 贡献指南

若您有意参与项目贡献，请参见 [《贡献指南》](./docs/zh/contributing/contributing_guide.md)。  

## ⚖️ 相关说明

* [《License声明》](./docs/zh/legal/license_notice.md) 
* [《安全声明》](./docs/zh/legal/security_statement.md) 
* [《免责声明》](./docs/zh/legal/disclaimer.md)

## 🤝 建议与交流

欢迎大家为社区做贡献。如果有任何疑问或建议，请提交[Issues](https://gitcode.com/Ascend/msopprof/issues)，我们会尽快回复。感谢您的支持。

## 🙏 致谢

本工具由华为公司 **计算产品线** 贡献。    
感谢来自社区的每一个PR，欢迎贡献。
