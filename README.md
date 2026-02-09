# **MindStudio Ops Profiler**

## 最新消息

- [2025.12.30]：MindStudio Ops Profiler项目首次上线。

## 简介

MindStudio Ops Profiler（算子调优工具，msOpProf）用于采集和分析运行在昇腾AI处理器上算子的关键性能指标，用户可根据输出的性能数据，快速定位算子的软、硬件性能瓶颈，提升算子性能的分析效率。

当前支持基于不同运行模式（上板或仿真）和不同文件形式（可执行文件或算子二进制.o文件）进行性能数据的采集和自动解析。

## 目录结构

关键目录如下。

```tex
├── cmake                              # 项目工程编译目录 
├── csrc                               # 源码
├── docs                               # 项目文档介绍 
├── example                            # 项目示例代码
├── package                            # run包安装、卸载及升级相关脚本 
├── test                               # UT测试 
├── thirdparty                         # 三方依赖
```

## 环境部署

### 环境依赖

- 硬件环境请参见《[昇腾产品形态说明](https://www.hiascend.com/document/detail/zh/AscendFAQ/ProduTech/productform/hardwaredesc_0001.html)》。

- 工具的使用运行需要提前获取并安装CANN开源版本，当前CANN开源版本正在发布中，敬请期待。

### 工具安装

msOpProf工具安装操作请参见[MindStudio Ops Profiler安装指南](./docs/zh/msopprof_install_guide.md)。

## 快速入门

详细操作步骤请参见[快速入门](./docs/zh/quick_start/msopprof_quick_start.md)。

## 工具限制与注意事项

- msOpProf工具的使用依赖CANN包中的msopprof可执行文件，该文件中的接口使用和msopprof一致，该文件为CANN包自带，无需单独安装。

- 通过键盘输入“CTRL+C”后，算子执行将会被停止，工具会根据当前已有信息生成性能数据文件。若不需要生成该文件，可再次键盘输入“CTRL+C”指令。

- 若未指定--output参数，需确保群组和其他组的用户不具备当前路径的上一级目录的写入权限。

- 用户需自行保证可执行文件或用户程序（*application*）执行的安全性。
  - 建议限制对可执行文件或用户程序（*application*）的操作权限，避免提权风险。
  - 不建议进行高危操作（删除文件、删除目录、修改密码及提权命令等），避免安全风险。

- 如果要使用MindStudio Insight工具查看msOpProf工具输出的文件时，需要单独安装MindStudio Insight软件包，安装步骤可参见[MindStudio Insight安装指南](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/mindstudio_insight_install_guide.md)。

- 性能数据采集时间建议在5min以内，同时推荐用户设置的内存大小在20G以上（例如容器配置：docker run --memory=20g 容器名）。

- 请确保性能数据保存在不含软链接的当前用户目录下，否则可能引起安全问题。

- 不支持在同一个Device侧同时拉起多个性能采集任务。

- 使用msopprof和msopprof simulator之前，用户需保证app功能正常。

## 功能介绍

msOpProf工具包含msopprof和msopprof simulator两种使用方式，协助用户定位算子内存、算子代码以及算子指令的异常，实现全方位的算子调优。

- [msopprof模式](./docs/zh/msopprof_user_guide.md)：适用于实际运行环境中的性能分析，可协助用户定位算子内存和性能瓶颈。直接分析运行中的算子，无需额外配置，适合在板环境中快速定位算子性能问题。

- [msopprof simulator模式](./docs/zh/msopprof_simulator_user_guide.md)：需配置环境变量和编译选项，适合在仿真环境中详细分析算子行为。

## 典型案例

msOpProf通过一些典型案例帮助您理解并使用工具，具体案例请参见[MindStudio Ops Profiler典型案例](./docs/zh/typical_cases.md)。

## 免责声明

### 致msOpProf使用者

- 本工具仅供调试和开发之用，使用者需自行承担使用风险，并理解以下内容：

  - [x] 数据处理及删除：用户在使用本工具过程中产生的数据属于用户责任范畴。建议用户在使用完毕后及时删除相关数据，以防信息泄露。

  - [x] 数据保密与传播：使用者了解并同意不得将通过本工具产生的数据随意外发或传播。对于由此产生的信息泄露、数据泄露或其他不良后果，本工具及其开发者概不负责。

  - [x] 用户输入安全性：用户需自行保证输入的命令行的安全性，并承担因输入不当而导致的任何安全风险或损失。对于由于输入命令行不当所导致的问题，本工具及其开发者概不负责。

- 免责声明范围：本免责声明适用于所有使用本工具的个人或实体。使用本工具即表示您同意并接受本声明的内容，并愿意承担因使用该功能而产生的风险和责任，如有异议请停止使用本工具。

- 在使用本工具之前，请**谨慎阅读并理解以上免责声明的内容**。对于使用本工具所产生的任何问题或疑问，请及时联系开发者。

### 致数据所有者

如果您不希望您的模型或数据集等信息在msOpProf中被提及，或希望更新msOpProf中有关的描述，请在GitCode提交Issues，我们将根据您的Issues要求删除或更新您相关描述。衷心感谢您对msOpProf的理解和贡献。

## License

msOpProf工具的使用许可证，具体请参见[LICENSE](./LICENSE)。

msOpProf工具docs目录下的文档适用CC-BY 4.0许可证，具体请参见[LICENSE](./docs/LICENSE)。

## 贡献声明

1. **提交错误报告**：如果您在msOpProf中发现了一个不存在安全问题的漏洞，请在msOpProf仓库中的Issues中搜索，以防该漏洞被重复提交，如果找不到漏洞可以创建一个新的Issues。如果发现了一个安全问题请不要将其公开，请参阅安全问题处理方式。提交错误报告时应该包含完整信息。
2. **安全问题处理**：本项目中对安全问题处理的形式，请通过邮箱通知项目核心人员确认并编辑。
3. **解决现有问题**：通过查看仓库的Issues列表可以发现需要处理的问题信息, 可以尝试解决其中的某个问题。
4. **如何提出新功能**：请使用Issues的Feature标签进行标记，我们会定期处理和确认开发。
5. **开始贡献**：
   1. Fork本项目的仓库。
   2. Clone到本地。
   3. 创建开发分支。
   4. 本地测试：提交前请通过所有单元测试，包括新增的测试用例。
   5. 提交代码。
   6. 新建Pull Request。
   7. 代码检视：您需要根据评审意见修改代码，并再次推送更新。此流程可能涉及多轮迭代。
   8. 当您的PR获得足够数量的检视者批准后，Committer会进行最终审核。
   9. 审核和测试通过后，CI会将您的PR合并到项目的主干分支。

## 建议与交流

欢迎大家为社区做贡献。如果有任何疑问或建议，请提交[Issues](https://gitcode.com/Ascend/msopprof/issues)，我们会尽快回复。感谢您的支持。

## 致谢

msOpProf工具由华为公司的下列部门联合贡献：

- 计算产品线

感谢来自社区的每一个PR，欢迎贡献msOpProf。
