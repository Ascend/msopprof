<h1 align="center">MindStudio Ops Profiler</h1>

<div align="center">
<h2>昇腾 AI 算子调优工具</h2>

 [![Ascend](https://img.shields.io/badge/Community-MindStudio-blue.svg)](https://www.hiascend.com/developer/software/mindstudio) 
 [![License](https://badgen.net/badge/License/MulanPSL-2.0/blue)](./LICENSE)

</div>

## ✨ 最新消息

<span style="font-size:14px;">

🔹 **[2025.12.31]**：MindStudio Ops Profiler 项目全面开源

</span>

## ️ ℹ️ 简介

MindStudio Ops Profiler（算子调优工具，msOpProf）用于采集与分析运行在昇腾AI处理器上的算子关键性能指标，用户可基于输出的性能数据快速定位算子在软件或硬件层面的性能瓶颈，显著提升性能分析效率。当前支持多种运行模式（真机部署或仿真）及多种输入形式（可执行文件或算子二进制 .o 文件）下的性能数据采集与自动解析。

<div align="center">
  <h4>▶️ 核心能力快速演示</h4>
  <img src="./docs/zh/figures/demo-msopprof.gif" alt="快速演示" width="600">
  <p><sup>图示：算子上板、仿真调优性能采集过程演示</sup></p>
</div>

## ⚙️ 功能介绍

包含msOpProf和msOpProf simulator两种使用模式：

| 功能名称 | 功能描述 |
|---------|--------|
| **msOpProf 模式** | 适用于实际运行环境中的性能分析，可直接分析运行中的算子，无需额外配置，帮助用户快速定位算子的内存与性能瓶颈，尤其适合板端环境。 |
| **msOpProf simulator 模式** | 需配置环境变量和编译选项，适用于仿真环境中对算子行为进行详细、深入的性能分析。 |

## 🚀 快速入门

快速体验核心功能，请参见《[msOpProf 快速入门](./docs/zh/quick_start/msopprof_quick_start.md)》。

## 📦 安装指南

msOpProf工具安装操作请参见《[msOpProf 安装指南](./docs/zh/install_guide/msopprof_install_guide.md)》。

## 📘 使用指南

工具的详细使用方法，请参见《[msOpProf 使用指南](./docs/zh/user_guide/msopprof_user_guide.md)》或《[msOpProf simulator模式使用指南](./docs/zh/user_guide/msopprof_simulator_user_guide.md)》。

## 💡 典型案例

msOpProf通过一些典型案例帮助您理解并使用工具，具体案例请参见《[msOpProf 典型案例](./docs/zh/best_practices/typical_cases.md)》。

## 🛠️ 贡献指南

欢迎参与项目贡献，请参见《[贡献指南](./docs/zh/contributing/contributing_guide.md)》。

## ⚖️ 相关说明

🔹《[版本说明](./docs/zh/release_notes/release_notes.md)》  
🔹《[许可证声明](./docs/zh/legal/license_notice.md)》  
🔹《[安全声明](./docs/zh/legal/security_statement.md)》  
🔹《[免责声明](./docs/zh/legal/disclaimer.md)》   

## 🤝 建议与交流

欢迎大家为社区做贡献。如果有任何疑问或建议，请提交[Issues](https://gitcode.com/Ascend/msopprof/issues)，我们会尽快回复。感谢您的支持。

|                                      📱 关注 MindStudio 公众号                                       | 💬 更多交流与支持                                                                                                                                                                                                                                                                                                                                                                                                                     |
|:-----------------------------------------------------------------------------------------------:|:-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| <img src="https://gitcode.com/Ascend/msot/blob/master/docs/zh/figures/readme/officialAccount.png" width="120"><br><sub>*扫码关注获取最新动态*</sub> | 💡 **加入微信交流群**：<br>关注公众号，回复“交流群”即可获取入群二维码。<br><br>🛠️ **其他渠道**：<br>👉 昇腾助手：[![WeChat](https://img.shields.io/badge/WeChat-07C160?style=flat-square&logo=wechat&logoColor=white)](https://gitcode.com/Ascend/msot/blob/master/docs/zh/figures/readme/xiaozhushou.png)<br>👉 昇腾论坛：[![Website](https://img.shields.io/badge/Website-%231e37ff?style=flat-square&logo=RSS&logoColor=white)](https://www.hiascend.com/forum/) |

## 🙏 致谢

本工具由华为公司的下列部门联合贡献：    
🔹 昇腾计算MindStudio开发部  
🔹 昇腾计算生态使能部  
🔹 华为云昇腾云服务  
🔹 2012编译器实验室  
🔹 2012马尔科夫实验室  
感谢来自社区的每一个 PR，欢迎贡献！
