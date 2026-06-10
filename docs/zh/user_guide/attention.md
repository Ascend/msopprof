# MindStudio Ops Profiler 工具限制与注意事项

## 环境约束

- msOpProf工具的使用依赖CANN包中的msopprof可执行文件，该文件中的接口使用和msopprof一致，该文件为CANN包自带，无需单独安装。
- 如果要使用MindStudio Insight工具查看msOpProf工具输出的文件时，需要单独安装MindStudio Insight软件包，安装步骤可参见[MindStudio Insight安装指南](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/mindstudio_insight_install_guide.md)。

## 运行约束

- 通过键盘输入"CTRL+C"后，算子执行将会被停止，工具会根据当前已有信息生成性能数据文件。若不需要生成该文件，可再次键盘输入"CTRL+C"指令。
- 不支持在同一个Device侧同时拉起多个性能采集任务。
- 性能数据采集时间建议在5min以内，同时推荐用户设置的内存大小在20G以上（例如容器配置：docker run --memory=20g 容器名）。
- 请确保性能数据保存在不含软链接的当前用户目录下，否则可能引起安全问题。

## 文件权限

- 若未指定--output参数，工具默认将结果写入当前工作目录下的默认文件名。为避免权限问题，需确保群组和其他组的用户不具备当前路径的上一级目录的写入权限。

## 安全注意事项

- 用户需自行保证可执行文件或用户程序（*application*）执行的安全性。
  - 建议限制对可执行文件或用户程序（*application*）的操作权限，避免提权风险。
  - 不建议进行高危操作（删除文件、删除目录、修改密码及提权命令等），避免安全风险。
- 工具运行过程中涉及从LD_LIBRARY_PATH加载so，用户在使用前需要需要确保LD_LIBRARY_PATH环境变量内容安全可信，指向路径不涉及软链接，且权限及属主符合安全预期，无法被第三方篡改，否则有任意代码注入风险。

## 通用

- 使用msopprof和msopprof simulator之前，用户需保证app功能正常。
