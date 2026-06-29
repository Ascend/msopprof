# msOpProf模式用户指南

## 简介

MindStudio Ops Profiler（算子调优工具，msOpProf）用于采集和分析运行在AI处理器上算子的关键性能指标，用户可根据输出的性能数据，快速定位算子的软、硬件性能瓶颈，提升算子性能的分析效率。

当前支持基于上板（msOpProf）或仿真（msOpProf simulator）运行模式和不同文件形式（可执行文件或算子二进制**.o**文件）进行性能数据的采集和自动解析。

本文档介绍msOpProf运行模式的使用方法。

**功能特性**

通过MindStudio Insight展示计算内存热力图、Roofline瓶颈分析图、Cache热力图、通算流水图（通算融合算子）、Pipe流水图、算子代码热点图以及性能数据文件等单算子调优能力，具体请参考[**表 1**  msOpProf模式功能特性](#msOpProf模式功能特性)。

**表 1**  msOpProf模式功能特性<a id="msOpProf模式功能特性"></a>

|功能|链接|
|---|---|
|计算内存热力图|[计算内存热力图](#计算内存热力图)|
|Roofline瓶颈分析图|[Roofline瓶颈分析图](#roofline瓶颈分析图)|
|Cache热力图|[Cache热力图](#cache热力图)|
|Pipe流水图|[Pipe流水图](#pipe流水图)|
|指令流水图|[指令流水图](#指令流水图)|
|通算流水图|[通算流水图](#通算流水图)|
|Warp流水图|[Warp流水图](#warp流水图)|
|算子代码热点图|[算子代码热点图](#算子代码热点图)|
 |Warp Stall热点图|[Warp Stall热点图](#warp-stall热点图)|
|性能数据文件|[性能数据文件](./msopprof_performance_data.md)|

**调用场景**

| 调用场景 | 参考章节 |
| --- | --- |
| Kernel直调场景 | [Kernel直调场景](https://gitcode.com/Ascend/msopprof/blob/master/docs/zh/user_guide/msopprof_usage.md#采集kernel直调方式ascend-c算子的性能数据) |
| 单算子API调用场景 | [单算子API调用场景](https://gitcode.com/Ascend/msopprof/blob/master/docs/zh/user_guide/msopprof_usage.md#采集API调用单算子的性能数据) |
| PyTorch 框架接入算子 | [PyTorch框架算子调用场景](https://gitcode.com/Ascend/msopprof/blob/master/docs/zh/user_guide/msopprof_usage.md#采集PyTorch框架算子的性能数据) |
| Triton-Ascend 算子 | [Triton算子调用场景](https://gitcode.com/Ascend/msopprof/blob/master/docs/zh/user_guide/msopprof_usage.md#采集triton算子的性能数据) |
| catlass算子 | [catlass算子调用场景](https://gitcode.com/Ascend/msopprof/blob/master/docs/zh/user_guide/msopprof_usage.md#采集catlass算子的性能数据) |
| MC2算子调用场景 | [MC2算子调用场景](https://gitcode.com/Ascend/msopprof/blob/master/docs/zh/user_guide/msopprof_usage.md#采集MC2算子的性能数据) |

## 使用前准备

**环境准备**

- 请参考[MindStudio Ops Profiler安装指南](../install_guide/msopprof_install_guide.md)，完成相关环境变量的配置。

- 若要使用MindStudio Insight进行查看时，需要单独安装MindStudio Insight软件包，具体下载链接请参见[MindStudio Insight安装指南](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/install_guide/mindstudio_insight_install_guide.md)。

**使用约束**

- 性能数据采集时间建议在5min以内，同时推荐用户设置的内存大小在20G以上（例如容器配置：docker run --memory=20g  _容器名_）。
- 请确保性能数据保存在不含软链接的当前用户目录下，否则可能引起安全问题。
- 工具运行过程中涉及从LD_LIBRARY_PATH加载so，用户在使用前需要确保LD_LIBRARY_PATH环境变量内容安全可信，指向路径不涉及软链接，且权限及属主符合安全预期，无法被第三方篡改，否则有任意代码注入风险。

## 注意事项

- msOpProf工具的使用依赖CANN包中的msOpProf可执行文件，该文件中的接口使用和msOpProf一致，该文件为CANN包自带，无需单独安装。
- 通过键盘输入“CTRL+C”后，算子执行将会被停止，工具会根据当前已有信息生成性能数据文件。若不需要生成该文件，可再次键盘输入“CTRL+C”指令。
- 若未指定--output参数，需确保其他用户不具备当前路径的上一级目录的写入权限。
- 使用msOpProf之前，用户需保证app功能正常。
- 不支持在同一个Device侧同时拉起多个性能采集任务。
- 用户需自行保证可执行文件或用户程序（_application_）执行的安全性。
    - 建议限制对可执行文件或用户程序（_application_）的操作权限，避免提权风险。
    - 不建议进行高危操作（删除文件、删除目录、修改密码及提权命令等），避免安全风险。

## 命令参考

登录运行环境，使用```msprof op 可选参数  app [arguments]```格式调用，可选参数的具体情况请参考[**表 2**  msOpProf可选参数表](#msOpProf可选参数表)。具体命令示例如下：

```shell
msprof op --output=$HOME/projects/output $HOME/projects/MyApp/out/main blockdim 1    # --output为可选参数,$HOME/projects/MyApp/out/main为使用的app,blockdim 1为用户app的可选参数 
```

**表 2**  msOpProf可选参数表<a id="msOpProf可选参数表"></a>

|可选参数|描述|是否必选|
|------|-------|-------|
|--application|建议使用**msprof op [msOpProf 参数] ./app**进行拉取，其中app为指定的可执行文件，如果app未指定路径，默认为使用当前路径。<br>使用./app时，需将msOpProf的相关参数添加到./app前，以确保相关功能生效。<br>当前与./app [arguments]兼容，后期将修改为./app [arguments]。|是，指定的可执行文件和--config二选一|
|--config|配置为输入算子编译得到的二进制文件*.o，可配置为绝对路径或者相对路径。具体可参考[json配置文件说明](./extended_functions.md#json配置文件说明)。<br>进行算子调优之前，可通过以下两种方式获取算子二进制*.o文件。<ul><li>参考《Ascend C算子开发指南》中的“Kernel直调算子开发 > [Kernel直调](https://www.hiascend.com/document/detail/zh/canncommercial/850/opdevg/Ascendcopdevg/atlas_ascendc_10_0056.html)”章节中的“修改并执行一键式编译运行脚本”，获取NPU侧可执行文件，并需要用户自行从可执行文件中提取*.o文件。</li><li>参考[算子编译部署](https://gitcode.com/Ascend/msopgen/blob/master/docs/zh/user_guide/msopgen_user_guide.md#%E7%AE%97%E5%AD%90%E7%BC%96%E8%AF%91%E9%83%A8%E7%BD%B2)，算子编译时会自动生成*.o文件。</li></ul>需确保群组和其他组的用户不具备--config指定的json文件及上一级目录的写入权限。同时，需要确保json文件的上一级目录属主为当前用户。|是，指定的可执行文件和--config二选一|
|--kernel-name|指定要采集的算子名称，支持使用算子名前缀进行模糊匹配。如果不指定，则只对程序运行过程中调度的第一个算子进行采集。<br>注意事项：<li>需与--application配合使用，限制长度为1024，仅支持A-Za-z0-9_中的一个或多个字符。</li><li>需要采集多个算子时，支持使用符号“\|”进行拼接。例如,--kernel-name="add\|abs"表示采集前缀名为add和abs的算子。</li><li>具体采集的算子数量由--launch-count参数值决定。</li><li>支持使用通配符（*）匹配任意长度字符。</li>|否|
|--launch-count|设置可以采集算子的最大数量，默认值为1，取值范围为1~5000之间的整数。|否|
|--launch-skip-before-match|用于设置不需要采集数据的算子数量，从第一个算子开始到指定数目的算子不进行采集，仅对指定数目之后的算子开始采集。<br>注意事项：<ul><li>无论--launch-skip-before-match参数是否命中kernel-name中指定的算子，该项的计数都会增加，且不采集该算子。</li><li>此参数的取值范围为0~1000之间的整数。</li></ul>|否|
|--aic-metrics|使能算子性能指标的采集能力和算子采集能力指标。<ul><li>使能算子性能指标的采集能力（ArithmeticUtilization、L2Cache、Memory、MemoryL0、MemoryUB、PipeUtilization、ResourceConflictRatio和Default），可选其中的一项或多项性能指标，选多项时用英文逗号隔开，例如：`--aic-metrics=Memory,MemoryL0`。</li><li>默认使能**Default**，采集以下性能指标（ArithmeticUtilization、L2Cache、Memory、MemoryL0、MemoryUB、PipeUtilization、ResourceConflictRatio）。例如：`--aic-metrics=Default`。</li><li>使能算子Kernel侧指定代码段范围内的性能指标采集（KernelScale）。<br>KernelScale可对算子Kernel侧指定代码段范围进行调优。需先配置--aic-metrics=KernelScale，然后选其中的一项或多项算子性能指标，选多项时用英文逗号隔开，例如：`--aic-metrics=KernelScale,Memory,MemoryL0`。<br>默认选择全部算子性能指标进行采集，例如：`--aic-metrics=KernelScale`。<br>指定代码段范围时，需要在算子Kernel侧对应的代码段前后进行设置，具体设置请参见《Ascend C算子开发接口》的“算子调测API”章节的[MetricsProfStart](https://www.hiascend.com/document/detail/zh/canncommercial/850/API/ascendcopapi/atlasascendc_api_07_1214.html)和[MetricsProfStop](https://www.hiascend.com/document/detail/zh/canncommercial/850/API/ascendcopapi/atlasascendc_api_07_1215.html)接口。<br>仅Atlas A3 训练系列产品/Atlas A3 推理系列产品和Atlas A2 训练系列产品/Atlas A2 推理系列产品以及Ascend 950 系列产品支持该功能。</li><li>Roofline：使能生成Roofline瓶颈分析图，并通过MindStudio Insight进行可视化呈现，例如：--aic-metrics=Roofline。具体请参见[Roofline瓶颈分析图](#roofline瓶颈分析图)。Roofline与Default已绑定，使能Roofline即同时启用了Roofline和Default模式。</li><li>TimelineDetail：使能生成仿真指令流水图和算子代码热点图，进行可视化呈现，例如：`--aic-metrics=TimelineDetail`。具体呈现内容请参见[仿真指令流水图](./msopprof_simulator_user_guide.md#指令流水图)和[算子代码热点图](#算子代码热点图)。<br>若要使能此功能，需要参考[使用前准备](#使用前准备)进行配置。<br>仅Atlas A2 训练系列产品/Atlas A2 推理系列产品和Atlas A3 训练系列产品/Atlas A3 推理系列产品支持该功能。<br>此功能仅支持第三方框架算子调用：PyTorch框架的场景且内部使用单算子API方式调起算子的场景。<br>此功能不支持采集二级指针类算子，Triton算子及通算融合类算子。且不支持与--replay-mode=application/range同时使能。<br>若要生成csv文件或展示计算内存热力图，拉起算子时，需使能Default，示例如下：`msprof op --aic-metrics=TimelineDetail,Default`</li><li>PipeTimeline：使能生成pipe流水图，能够直观看到算子各个Pipe的运行情况。例如：`--aic-metrics=PipeTimeline`。<br>具体呈现内容请参见[Pipe流水图](#pipe流水图)。<br>不支持PipeTimeline和instrTimeLine同时使能。<br>暂不支持通算融合类算子。<br>目前该功能只支持Ascend 950 系列产品。</li><li>instrTimeLine：使能生成上板指令流水图，直观展示每条指令的实际运行耗时，包含VECTOR/MTE1/MTE2/MTE3/CUBE/FIXP通路。例如：`--aic-metrics=instrTimeLine`。<br>配合参数`--instr-timeline-pipe`可以只采集指定pipe的流水，支持的pipe如下：cube、fixp、vector、mte1、mte2、mte3。例如`--aic-metrics=instrTimeLine --instr-timeline-pipe="mte1\|vector"`。<br>若需要每条指令展示指令PC及调用栈信息，需在编译算子时添加`-g`编译选项。<br>具体呈现内容请参见[指令流水图](#指令流水图)。<br>不支持PipeTimeline和instrTimeLine同时使能。<br>暂不支持通算融合类算子。<br>目前该功能只支持Ascend 950 系列产品。</li><li>Occupancy：使能生成核间负载分析图，并通过MindStudio Insight进行可视化呈现，例如：`--aic-metrics=Occupancy`。具体请参见[核间负载分析图](#计算内存热力图)。<br>各物理核之间，会针对耗时、数据吞吐量及Cache命中率分别进行对比，若最大值和最小值的差距大于10%，则说明负载不均衡，命令行界面会给出相应的调优建议。<br>仅Atlas A3 训练系列产品/Atlas A3 推理系列产品和Atlas A2 训练系列产品/Atlas A2 推理系列产品以及Ascend 950 系列产品支持该功能。</li><li>MemoryDetail：例如：`--aic-metrics=MemoryDetail`。</li><ul><li>使能该命令后，会开启L2 Cache相关功能（[计算负载分析图](#计算内存热力图)中的L2 Cache-L0A/L0B连线，[Cache热力图](#cache热力图)、[算子代码热点图](#算子代码热点图)中的L2Cache命中率以及与GM有关的数据搬运量）。</li></ul><ul><li>使能MemoryDetail时，会在内存负载分析图中展示aicore上Cube单元中MTE1和MTE2的活跃带宽。若MemoryDetail失败，则内存负载分析图中相应栏位会展示为NA，PipeUtilization（计算单元和搬运单元耗时占比）中不展示aic_mte1_active_bw(GB/s)和aic_mte2_active_bw(GB/s)。<br>不支持与`--replay-mode=range`同时使能。<br>MemoryDetail与Default已绑定，使能MemoryDetail即同时启用了MemoryDetail和Default模式。<br>仅Atlas A3 训练系列产品/Atlas A3 推理系列产品、Atlas A2 训练系列产品/Atlas A2 推理系列产品以及Ascend 950 系列产品支持该功能。</li></ul><li>BasicInfo：使能基础信息采集，仅落盘算子基础信息，例如：`--aic-metrics=BasicInfo`，具体落盘内容请参考[OpBasicInfo（算子基础信息）](./msopprof_performance_data.md#opbasicinfo算子基础信息)。</li><li>Source：使能算子代码热点图，例如：`--aic-metrics=Source`。具体请参见[算子代码热点图](#算子代码热点图)。<br>若需要查看代码调用栈，需在编译算子时添加`-g`编译选项，具体操作请参见[编译选项需添加-g](#使用前准备)。<br>不支持与`--replay-mode=range`同时使能。<br>仅<term>Atlas A3 训练系列产品/Atlas A3 推理系列产品</term>和<term>Atlas A2 训练系列产品/Atlas A2 推理系列产品</term>以及Ascend 950 系列产品支持该功能。</li><li>PcSampling：展示simt算子上板运行的 stall 信息。例如：`--aic-metrics=PcSampling`。具体请参见[算子代码热点图](#算子代码热点图)和[Warp Stall热点图](#warp-stall热点图)。<br>仅Ascend 950 系列产品支持该功能。</li></ul>|否|
|--instr-timeline-pipe|需配合`--aic-metrics=instrTimeLine`参数进行使用，详情请参见上方`--aic-metrics=instrTimeLine`参数。|否|
|--kill|选项包括开启（on）和关闭（off），默认情况下设置为关闭（off），关闭该功能。<br>若用户配置--kill=on使能该功能，用户程序将会在采集完--launch-count设置的算子数量后，自动停止程序。<br>注意事项：<br><ul><li>配置--kill=on后，可能会出现因用户程序提前结束而引发的错误日志，用户需自行评估是否使用该功能。</li><li>若用户程序为多进程，--kill参数的配置只对子进程生效。</li><li>使用该参数会造成最后一个被执行的通算融合算子无法正常获取接口调用流水，具体请参见[通算流水图](#通算流水图)。</li><li>不建议与`--replay-mode=range`同时使能，否则可能导致采集的算子数据缺失。</li></ul>|否|
|--mstx|该参数决定算子调优工具是否使能用户代码程序中使用的mstx API。<br>默认为off，表示关闭对mstx API的使能。<br>若用户配置--mstx=on，算子调优工具将会使能用户代码程序中使用的mstx API。具体举例如下：`msprof op --mstx=on ./add_custom`<br>注意事项：<ul><li>当前已支持mstx API中的mstxRangeStartA和mstxRangeEnd接口，功能为使能算子调优的指定区间，具体参数介绍请参见《MindStudio mstx API参考》中的[mstxRangeStartA](https://gitcode.com/Ascend/mstx/blob/master/docs/zh/api_reference/Common/mstxRangeStartA.md)和[mstxRangeEnd](https://gitcode.com/Ascend/mstx/blob/master/docs/zh/api_reference/Common/mstxRangeEnd.md)接口。</li><li>配合`--replay-mode=range`使用时，mstxRangeStartA和mstxRangeEnd接口需成对调用，不支持交叉调用。每一对mstx API中包含的算子为一个重放范围，该重放范围内算子的Stream不能改变。同时，能采集的算子数量受[OpBasicInfo（算子基础信息）](./msopprof_performance_data.md#opbasicinfo算子基础信息)中算子Block Dim数量限制（建议不超过50个）。</li></ul>|否|
|--mstx-include|该参数支持在算子调优工具使能mstx API的情况下，仅使能用户指定mstx API。<br>若不配置，则默认使能所有用户代码中使用的mstx API。<br>若配置，--mstx-include只使能用户指定的mstx API。--mstx-include的输入为用户调用mstx函数时传入的message字符串，使用"\|"拼接多个字符串。具体举例如下：`--mstx=on --mstx-include="hello\|hi" //仅使能用户传入mstx函数中message参数为hello和hi的mstx API`<br>注意事项：<ul><li>不可单独配置，需要与--mstx配合使用。</li><li>仅支持message为A-Z a-z 0-9 _这些字符，使用"\|"进行拼接。</li></ul>|否|
|--replay-mode|算子数据采集的重放模式，可配置为kernel/application/range，默认为kernel。<ul><li>若配置为application，代表是应用级重放，整个应用会进行多次重放。application模式下，单独使能部分aic-metrics可能会导致visualize_data.bin文件中部分数据丢失，若需要查看完整的visualize_data.bin数据，建议添加Default到--aic-metrics以采集完整的可视化数据。</li><li>若配置为kernel，代表是核函数级重放，指定采集范围的单个算子的核函数进行多次重放。</li><li>若配置为range，代表是范围级重放，指定范围内的多算子整体进行多次重放。可以指定多个范围，范围之间相互独立。</li>注意事项：<li>kernel模式和range模式会清除L2cache数据，而application模式不会清理L2cache数据。</li><li>多卡多算子的场景不支持配置为application。</li><li>范围级重放需配合`--mstx=on`一起使用，且仅适用于Atlas A3 训练系列产品/Atlas A3 推理系列产品和Atlas A2 训练系列产品/Atlas A2 推理系列产品。</li><li>范围级重放不支持采集MC2和LCCL类型的通算融合算子，且不支持与--kill=on、--aic-metrics=MemoryDetail、--aic-metrics=TimelineDetail及--aic-metrics=Source同时使能。</li></ul>|否|
|--warm-up|当部分算子使用msOpProf采集时，会达不到芯片提频的最小任务耗时产生降频，从而会对交付件的结果产生一定影响。在该情况下，可用`--warm-up`指定预热次数，提前提升**AI处理器**的运行频率，使上板数据更准确。<br>注意事项：<ul><li>默认值为5，取值范围为[0,500]。</li><li>此参数对MC2算子不生效。</li><li>当开启范围级别重放时，--warm-up次数至少为1，不得设置--warm-up=0。</li></ul>|否|
|--output|收集到的性能数据的存放路径，默认在当前目录下保存性能数据。<br>需确保群组和其他组的用户不具备--output指定输出路径的上一级目录的写入权限。同时，需要确保--output指定目录的上一级目录属主为当前用户。|否|
|--dump|控制仿真器dump文件是否生成。<br>选项包括开启（on）和关闭（off），默认情况下设置为关闭（off），即不生成仿真器dump文件。<br>注意事项：<ul><li>此参数仅在使用--aic-metrics=TimelineDetail选项时有效，且仅针对Atlas A2 训练系列产品/Atlas A2 推理系列产品及Atlas A3 训练系列产品/Atlas A3 推理系列产品生效，对Atlas 推理系列产品不生效。</li><li>此参数仅适用于单进程场景，不支持两个算子同时运行的场景。</li></ul>|否|
|--core-id|该参数适用于算子分布均匀的情况时，可使用--core-id参数指定部分逻辑核的id，解析部分核的仿真数据。<br>核id的取值范围为[0,49]。<br>注意事项：<ul><li>若要解析多个核的仿真数据时，需要使用符号"\|"进行拼接。例如，--core-id="0\|31"表示解析核id为0和31的仿真数据。</li><li>此参数仅在使用`--aic-metrics=TimelineDetail`选项时有效，仅作用于[指令流水图](./msopprof_simulator_user_guide.md#指令流水图)和[算子代码热点图](#算子代码热点图)，仅适用于Atlas A2 训练系列产品/Atlas A2 推理系列产品及Atlas A3 训练系列产品/Atlas A3 推理系列产品。</li></ul>|否|
|--custom-input|用于读取自定义输入文件，具体输入示例请参见[json示例文件](#json示例文件)，目前只支持[通算流水图](#通算流水图)使用。|否|
|-h，--help|输出帮助信息。|否|

## 工具使用

msOpProf工具协助用户定位算子内存、算子代码以及算子指令的异常，实现全方位的算子调优。使用方式的详细说明请参考[**表 3**  msOpProf模式功能说明表](#msOpProf模式功能说明表)。

**表 3**  msOpProf模式功能说明表<a id="msOpProf模式功能说明表"></a>

|适用场景|使用方式|展示的图形|
|---|---|---|
|适用于实际运行环境中的性能分析，可协助用户定位算子内存和性能瓶颈。|直接分析运行中的算子，无需额外配置，适合在板环境中快速定位算子性能问题。|[计算内存热力图](#计算内存热力图) <br> [Roofline瓶颈分析图](#roofline瓶颈分析图) <br> [Cache热力图](#cache热力图) <br> [流水图](#流水图) <br> [算子代码热点图](#算子代码热点图) <br> [Warp Stall热力图](#warp-stall热点图)|

**msOpProf分段调优原则**

1. 使用--launch-skip-before-match命令筛选算子调优范围，筛选原则如下：<a id="1"></a>

    - 若已配置--launch-skip-before-match，从第一个算子开始到指定数目的算子不进行采集，仅对指定数目之后的算子开始采集。
    - 若未配置，不进行筛选。

2. 在[1](#1)的基础上，使用--mstx命令筛选算子调优范围，筛选原则如下：<a id="2"></a>
    - 若已配置--mstx，只采集mstxRangeStartA和mstxRangeEnd接口使能范围内的算子。
    - 若未配置，不进行筛选。

3. 在[2](#2)的基础上，使用--kernel-name命令筛选算子调优范围，筛选原则如下：<a id="3"></a>
    - 若已配置--kernel-name，只采集--kernel-name范围内的算子。
    - 若未配置--kernel-name，则只对程序运行过程中调度的第一个算子进行采集。

4. 在[3](#3)的基础上，使用--aic-metrics命令筛选算子调优数据的采集项，筛选原则如下：<a id="4"></a>
    - 若已配置--aic-metrics，选择算子性能指标的采集项。
    - 若未配置--aic-metrics，默认采集Default部分的算子性能指标，KernelScale、TimelineDetail、Roofline、Occupancy部分的算子性能指标将无法采集。

5. 通过[1](#1)至[4](#4)逐层过滤，可获得实际的调优算子数量以及性能指标的采集范围。
6. 使能--kill=on功能的情况下，将实际调优的算子数量与--launch-count值进行对比，从而决定是否需要自动停止程序。<br>若实际已调优算子数量小于等于--launch-count值，则继续执行。否则，实际已调优算子数量达到--launch-count设置的算子数值时，会自动停止程序。

**msOpProf配置**

若要实现[Cache热力图跳转](#cache热力图)功能，需要执行以下操作：

1. 在编译算子时添加`-g`编译选项，具体操作请参见[编译选项需添加-g](./msopprof_simulator_user_guide.md#工具使用)。
2. --aic-metrics参数使能Source选项。

**启动工具**

> [!NOTE]    
> 当前msOpProf不支持-O0编译选项。

1. 登录运行环境，使用```msprof op 可选参数 app [arguments]```格式开启算子上板调优，可选参数的具体情况请参考[命令参考](#命令参考)。具体命令示例如下：

    ```shell
    msprof op --output=$HOME/projects/output $HOME/projects/MyApp/out/main    # --output为可选参数  $HOME/projects/MyApp/out/main为使用的app 
    ```

2. 通过以下两种方式执行算子调优：
    - 基于可执行文件
        - 单算子场景，以*test*为例。
            > [!NOTE]    
            > 示例中的可执行文件名称`test`仅作为示例展示，实际名称请以当前工程中编译生成的可执行文件为准。

            示例一：

            ```shell
            msprof op ./test
            ```

            示例二：

            ```shell
            msprof op --aic-metrics=<select_metrics> --output=./output_data ./test 
            ```

        - 多算子场景。

            若test中有Add，MatlMul，Sub算子，可配合--launch-count和--kernel-name使用，可以指定采集Add和Sub算子。

            ```shell
            msprof op --launch-count=10 --kernel-name="Add|Sub" --output=./output_data ./test    # ./test为用户二进制文件，需放置在命令末尾
            ```

    - 基于输入算子二进制文件*.o的配置文件.json，具体请参见[json配置文件说明](./extended_functions.md#json配置文件说明)。

        ```shell
        msprof op --config=./add_test.json --aic-metrics=<select_metrics> --output=./output_data
        ```

3. 命令完成后，会在默认路径或指定的“--output”目录下生成以“OPPROF\__\{timestamp\}_\__XXX_”命名的文件夹，在“--aic-metrics”全部开启时，结构示例如下：

    - 采集多卡多算子的场景。

        > [!NOTE]    
        > 对多卡并行的通算融合算子（MC2或LCCL算子）进行调优时，结果目录下会存在若干以Device ID为名的子目录，这取决于定义时指定的NPU数量，每个NPU的调优结果会分别存放在对应的Device ID目录下。

        ```tex
        └──OPPROF_{timestamp}_XXX
        ├── device0                  // 运行时使用AI处理器的ID
        └── device1                
          ├── OpName0                // OpName0为采集算子名称
          │ ├── 0                    // 表示算子调度顺序
          │ │ ├──dump                // 与单算子含义一致，存放过程件的文件夹
          │ │ └──xxx_yyy.csv        // xxx代表该算子生成的指标种类名,例如L2Cache,具体指标种类可参考[表2](#zh-cn_topic_0000002144027017_table056510168348)中的csv文件介绍,yyy为csv文件的时序后缀,例如L2Cache_20240603022812284.csv
          │ │ └──visualize_data.bin 
          ├── OpName1               
          │ ├── 0
          │ │ ├──dump 
          │ │ └──xxx_yyy.csv
          │ │ └──visualize_data.bin 
          ├── OpName2         
          │ ├── 0
          │ │ ├── dump  
          │ │ └── xxx_yyy.csv
          │ │ └──visualize_data.bin 
          │ │ └── trace.json         // 此文件仅适用于MC2和LCCL类型通算融合算子  
        ```

    - 采集单卡多算子场景。

        ```tex
        └──OPPROF_{timestamp}_XXX
        ├── OpName0                  // OpName0为采集算子名称
        │ ├── 0                      // 表示算子调度顺序
        │ │ ├── dump                 // 与单算子含义一致，存放过程件的文件夹
        │ │ └── xxx_yyy.csv          // xxx代表该算子生成的指标种类名,例如L2Cache,具体指标种类可参考[表2](#zh-cn_topic_0000002144027017_table056510168348)中的csv文件介绍,yyy为csv文件的时序后缀,例如L2Cache_20240603022812284.csv
        │ │ └──visualize_data.bin 
        │ ├── 1
        │ │ ├──dump 
        │ │ └──xxx_yyy.csv
        │ │ └──visualize_data.bin 
        ├── OpName1         
        │ ├── 0
        │ │ ├── dump  
        │ │ └── xxx_yyy.csv
        │ │ └── visualize_data.bin 
        ```

    - 采集单卡单算子场景。

        ```tex
        OPPROF_{timestamp}_XXX
        ├── dump
        ├── ArithmeticUtilization.csv
        ├── L2Cache.csv
        ├── Memory.csv
        ├── MemoryL0.csv
        ├── MemoryUB.csv
        ├── OpBasicInfo.csv
        ├── PipeUtilization.csv
        ├── ResourceConflictRatio.csv
        ├── visualize_data.bin 
        ```

    **表 4**  msOpProf模式文件介绍

    |名称|说明|
    |---|---|
    |dump文件夹|原始的性能数据，用户无需关注。|
    |ArithmeticUtilization.csv|Cube和Vector类型的指令耗时和占比，可参考[ArithmeticUtilization（Cube及Vector类型指令耗时和占比）](./msopprof_performance_data.md#arithmeticutilizationcube及vector类型指令耗时和占比)。|
    |L2Cache.csv|L2 Cache命中率，可参考[L2Cache（L2 Cache命中率）](./msopprof_performance_data.md#l2cachel2-cache命中率).|
    |Memory.csv|UB/L1/L2/主存储器采集内存读写带宽速率，可参考[Memory（内存读写带宽速率）](./msopprof_performance_data.md#memory内存读写带宽速率)。|
    |MemoryL0.csv|L0A/L0B/L0C采集内存读写带宽速率，可参考[MemoryL0（L0读写带宽速率）](./msopprof_performance_data.md#memoryl0l0读写带宽速率)。|
    |MemoryUB.csv|mte/vector/scalar采集ub读写带宽速率，可参考[MemoryUB（UB读写带宽速率）](./msopprof_performance_data.md#memoryubub读写带宽速率)。|
    |PipeUtilization.csv|采集计算单元和搬运单元耗时和占比，可参考[PipeUtilization（计算单元和搬运单元耗时占比）](./msopprof_performance_data.md#pipeutilization计算单元和搬运单元耗时占比)。|
    |ResourceConflictRatio.csv|UB上的bank group、bank conflict和资源冲突在所有指令中的占比，可参考[ResourceConflictRatio（资源冲突占比）](./msopprof_performance_data.md#resourceconflictratio资源冲突占比)。|
    |OpBasicInfo.csv|算子基础信息，包含算子名称、block dim和耗时等信息，可参考[OpBasicInfo（算子基础信息）](./msopprof_performance_data.md#opbasicinfo算子基础信息)。|
    |visualize_data.bin|算子基础信息、计算单元负载、热点函数和Roofline瓶颈分析等信息的可视化呈现文件。|
    |trace.json|通算流水可视化呈现文件。|

    > [!NOTE] 
    > 
    > - visualize\_data.bin可通过MindStudio Insight工具进行可视化展示，具体使用方法请参考[MindStudio Insight算子调优](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/operator_tuning.md)。
    > - msOpProf的热点函数功能仅支持<term>Atlas A2 训练系列产品/Atlas A2 推理系列产品</term>。
    > - 当前，仅支持生成MC2和LCCL类型通算融合算子的[通算流水图](#通算流水图)。
    > - MC2和LCCL类型通算融合算子不支持生成[Cache热力图](#cache热力图)和[算子代码热点图](#算子代码热点图)，且不支持<term>Atlas 推理系列产品</term>。
    > - 单位GB/s表示每秒传输1GB的数据量。

4. 将visualize_data.bin文件导入MindStudio Insight后，将会展示[计算内存热力图](#计算内存热力图)、[Roofline瓶颈分析图](#roofline瓶颈分析图)、[Cache热力图](#cache热力图)、[流水图](#流水图)和[算子代码热点图](#算子代码热点图)。
5. 将trace.json文件导入Chrome浏览器或MindStudio Insight后，将会展示[通算流水图](#通算流水图)。

## 计算内存热力图

### 功能说明

通过msOpProf运行模式生成的visualize\_data.bin文件可通过MindStudio Insight进行可视化呈现，界面将会以资源维度展示算子基础信息、计算负载分析和内存负载分析的数据，协助开发者以全局视角识别资源瓶颈。

MindStudio Insight具体操作请参考《MindStudio Insight算子调优》的“[详情（Details）](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/operator_tuning.md#%E8%AF%A6%E6%83%85%EF%BC%88details%EF%BC%89)”章节。

### 使用说明

visualize\_data.bin文件通过MindStudio Insight工具展示的界面如下。

**图 1**  详情界面1  
![](../figures/详情界面1.png "详情界面1")

- 提供核间负载分析图（Core Occupancy），以数据窗格的方式呈现各物理单核的耗时、总数据吞吐量及Cache命中率，帮助开发人员提升物理核的使用效率。

    > [!NOTE] 
    > 
    > - 仅Atlas A3 训练系列产品/Atlas A3 推理系列产品和Atlas A2 训练系列产品/Atlas A2 推理系列产品以及Ascend 950 系列产品支持该功能。
    > - 具体展示的核数与实际使用的硬件有关。

- Roofline瓶颈分析图（Roofline），具体介绍请参见[Roofline瓶颈分析图](#roofline瓶颈分析图)。
- 提供计算负载分析（Compute Workload Analysis），以柱状图和数据表格的方式呈现计算负载数据，帮助开发人员分析Cube/Vector计算资源是否得到了充分利用。
- 提供内存负载分析（Memory Workload Analysis），支持展示MTE各通路的活跃带宽值（未开启MemoryDetail不显示Cube上的MTE1和MTE2通路的活跃带宽）。通过内存热力图和数据窗格，直观呈现各通路的请求数、搬运带宽与利用率。帮助开发人员分析可能存在瓶颈的通路。

    > [!NOTE] 
    > 
    > - 数据窗格呈现的内容会随算子类型而变化。
    > - 活跃带宽值的功能不适用于Atlas 推理系列产品。
    > - Atlas A3 训练系列产品/Atlas A3 推理系列产品暂不支持峰值（最大带宽占比）展示。
    > - Ascend 950 系列产品包含SIMD架构算子暂不支持UB读写VEC单元的数据展示。

## Roofline瓶颈分析图

### 功能说明

通过msOpProf运行模式生成的visualize\_data.bin文件可通过MindStudio Insight进行可视化呈现，Roofline瓶颈分析图可构建出处理器的性能模型，然后利用该性能模型快速评估出算子的理论性能极限，协助开发者快速识别瓶颈类型。

MindStudio Insight具体操作请参考《MindStudio Insight算子调优》的“[详情（Details）](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/operator_tuning.md#%E8%AF%A6%E6%83%85%EF%BC%88details%EF%BC%89)”章节。

### 使用说明

**界面介绍**

生成的visualize\_data.bin文件可导入MindStudio Insight进行可视化呈现，并针对不同的硬件以及算子类型会生成不同的Roofline分析视图。

- Atlas 推理系列产品的Roofline瓶颈分析图中仅有内存单元视图。

    **图 2** Atlas 推理系列产品 Roofline瓶颈分析图  
    ![](../figures/Atlas-推理系列产品-Roofline瓶颈分析图.png "Atlas-推理系列产品-Roofline瓶颈分析图")

- Atlas A3 训练系列产品/Atlas A3 推理系列产品和Atlas A2 训练系列产品/Atlas A2 推理系列产品根据算子类型不同而产生不同的视图，具体请参见[**表 5** Atlas A3 训练系列产品/Atlas A3 推理系列产品和Atlas A2 训练系列产品/Atlas A2 推理系列产品支持Roofline视图情况列表](#A2-A3支持Roofline视图情况)。

    **图 3** Atlas A3 训练系列产品/Atlas A3 推理系列产品和Atlas A2 训练系列产品/Atlas A2 推理系列产品 Roofline瓶颈分析图  
    
    ![](../figures/1.png)

    **表 5** Atlas A3 训练系列产品/Atlas A3 推理系列产品和Atlas A2 训练系列产品/Atlas A2 推理系列产品支持Roofline视图情况列表<a id="A2-A3支持Roofline视图情况"></a>

    <table><thead align="left"><tr id="zh-cn_topic_0000002037945009_row1347917355615"><th class="cellrowborder" valign="top" width="24.97%" id="mcps1.2.5.1.1"><p id="zh-cn_topic_0000002037945009_p1447915351616">Roofline视图类型</p>
    </th>
    <th class="cellrowborder" valign="top" width="23.54%" id="mcps1.2.5.1.2"><p id="zh-cn_topic_0000002037945009_p647910358615">Vector算子</p>
    </th>
    <th class="cellrowborder" valign="top" width="26.490000000000002%" id="mcps1.2.5.1.3"><p id="zh-cn_topic_0000002037945009_p18491135615919">Cube算子</p>
    </th>
    <th class="cellrowborder" valign="top" width="25%" id="mcps1.2.5.1.4"><p id="zh-cn_topic_0000002037945009_p24801335163">Mix算子</p>
    </th>
    </tr>
    </thead>
    <tbody><tr id="zh-cn_topic_0000002037945009_row248016358610"><td class="cellrowborder" valign="top" width="24.97%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000002037945009_p126391453776">GM/L2视图</p>
    </td>
    <td class="cellrowborder" valign="top" width="23.54%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000002037945009_p848012358611">√</p>
    </td>
    <td class="cellrowborder" valign="top" width="26.490000000000002%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000002037945009_p13480035663">√</p>
    </td>
    <td class="cellrowborder" valign="top" width="25%" headers="mcps1.2.5.1.4 "><p id="zh-cn_topic_0000002037945009_p1913416816108">√</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002037945009_row6480735162"><td class="cellrowborder" valign="top" width="24.97%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000002037945009_p449045611592">Vector内存单元视图</p>
    </td>
    <td class="cellrowborder" valign="top" width="23.54%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000002037945009_p44801335968">√</p>
    </td>
    <td class="cellrowborder" valign="top" width="26.490000000000002%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000002037945009_p448016351617">-</p>
    </td>
    <td class="cellrowborder" valign="top" width="25%" headers="mcps1.2.5.1.4 "><p id="zh-cn_topic_0000002037945009_p2346141312108">√</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002037945009_row14480203520614"><td class="cellrowborder" valign="top" width="24.97%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000002037945009_p1135801418814">Vector内存通路视图</p>
    </td>
    <td class="cellrowborder" valign="top" width="23.54%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000002037945009_p1248018351269">√</p>
    </td>
    <td class="cellrowborder" valign="top" width="26.490000000000002%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000002037945009_p6480163515617">-</p>
    </td>
    <td class="cellrowborder" valign="top" width="25%" headers="mcps1.2.5.1.4 "><p id="zh-cn_topic_0000002037945009_p17481144100">√</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002037945009_row145071720980"><td class="cellrowborder" valign="top" width="24.97%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000002037945009_p23042355814">Vector Pipeline视图</p>
    </td>
    <td class="cellrowborder" valign="top" width="23.54%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000002037945009_p205081820887">√</p>
    </td>
    <td class="cellrowborder" valign="top" width="26.490000000000002%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000002037945009_p350811201483">-</p>
    </td>
    <td class="cellrowborder" valign="top" width="25%" headers="mcps1.2.5.1.4 "><p id="zh-cn_topic_0000002037945009_p08312151104">√</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002037945009_row18508112018810"><td class="cellrowborder" valign="top" width="24.97%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000002037945009_p636013402810">Cube内存单元视图</p>
    </td>
    <td class="cellrowborder" valign="top" width="23.54%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000002037945009_p155086202820">-</p>
    </td>
    <td class="cellrowborder" valign="top" width="26.490000000000002%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000002037945009_p6508132014812">√</p>
    </td>
    <td class="cellrowborder" valign="top" width="25%" headers="mcps1.2.5.1.4 "><p id="zh-cn_topic_0000002037945009_p1913418871011">√</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002037945009_row175087201382"><td class="cellrowborder" valign="top" width="24.97%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000002037945009_p1244016452081">Cube内存通路视图</p>
    </td>
    <td class="cellrowborder" valign="top" width="23.54%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000002037945009_p1550820201483">-</p>
    </td>
    <td class="cellrowborder" valign="top" width="26.490000000000002%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000002037945009_p17508112010812">√</p>
    </td>
    <td class="cellrowborder" valign="top" width="25%" headers="mcps1.2.5.1.4 "><p id="zh-cn_topic_0000002037945009_p171340813107">√</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002037945009_row122665261188"><td class="cellrowborder" valign="top" width="24.97%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000002037945009_p8147115019820">Cube Pipeline视图</p>
    </td>
    <td class="cellrowborder" valign="top" width="23.54%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000002037945009_p102667261987">-</p>
    </td>
    <td class="cellrowborder" valign="top" width="26.490000000000002%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000002037945009_p3266142618810">√</p>
    </td>
    <td class="cellrowborder" valign="top" width="25%" headers="mcps1.2.5.1.4 "><p id="zh-cn_topic_0000002037945009_p12135138201011">√</p>
    </td>
    </tr>
    </tbody>
    </table>

- Ascend 950 系列产品根据算子类型不同而产生不同的视图，具体请参见[**表 6** Ascend 950 系列产品支持Roofline视图情况列表](#A5支持Roofline视图情况)。

    **图 4** Ascend 950 系列产品Roofline瓶颈分析图  
    
    ![](../figures/1-0.png)

    **表 6** Ascend 950 系列产品支持Roofline视图情况列表<a id="A5支持Roofline视图情况"></a>

    <table><thead align="left"><tr id="zh-cn_topic_0000002037945009_row1660731475010"><th class="cellrowborder" valign="top" width="24.97%" id="mcps1.2.5.1.1"><p id="zh-cn_topic_0000002037945009_p1760741418509">Roofline视图类型</p>
    </th>
    <th class="cellrowborder" valign="top" width="23.54%" id="mcps1.2.5.1.2"><p id="zh-cn_topic_0000002037945009_p186071414155020">Vector算子</p>
    </th>
    <th class="cellrowborder" valign="top" width="26.490000000000002%" id="mcps1.2.5.1.3"><p id="zh-cn_topic_0000002037945009_p18607181418505">Cube算子</p>
    </th>
    <th class="cellrowborder" valign="top" width="25%" id="mcps1.2.5.1.4"><p id="zh-cn_topic_0000002037945009_p16071014125010">Mix算子</p>
    </th>
    </tr>
    </thead>
    <tbody><tr id="zh-cn_topic_0000002037945009_row0607151413505"><td class="cellrowborder" valign="top" width="24.97%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000002037945009_p1860716146506">GM/L2视图</p>
    </td>
    <td class="cellrowborder" valign="top" width="23.54%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000002037945009_p196071514125012">√</p>
    </td>
    <td class="cellrowborder" valign="top" width="26.490000000000002%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000002037945009_p16607171413503">√</p>
    </td>
    <td class="cellrowborder" valign="top" width="25%" headers="mcps1.2.5.1.4 "><p id="zh-cn_topic_0000002037945009_p14607141435013">√</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002037945009_row126076142500"><td class="cellrowborder" valign="top" width="24.97%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000002037945009_p3607111475010">Vector内存单元视图</p>
    </td>
    <td class="cellrowborder" valign="top" width="23.54%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000002037945009_p960791412508">√</p>
    </td>
    <td class="cellrowborder" valign="top" width="26.490000000000002%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000002037945009_p960741418503">-</p>
    </td>
    <td class="cellrowborder" valign="top" width="25%" headers="mcps1.2.5.1.4 "><p id="zh-cn_topic_0000002037945009_p1960721419501">√</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002037945009_row3608414155013"><td class="cellrowborder" valign="top" width="24.97%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000002037945009_p260821411506">Cube内存单元视图</p>
    </td>
    <td class="cellrowborder" valign="top" width="23.54%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000002037945009_p1608201418505">-</p>
    </td>
    <td class="cellrowborder" valign="top" width="26.490000000000002%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000002037945009_p26081014145020">√</p>
    </td>
    <td class="cellrowborder" valign="top" width="25%" headers="mcps1.2.5.1.4 "><p id="zh-cn_topic_0000002037945009_p156081414125019">√</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002037945009_row66082144505"><td class="cellrowborder" valign="top" width="24.97%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000002037945009_p4608201415015">Cube内存通路视图</p>
    </td>
    <td class="cellrowborder" valign="top" width="23.54%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000002037945009_p19608141410506">-</p>
    </td>
    <td class="cellrowborder" valign="top" width="26.490000000000002%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000002037945009_p156081142508">√</p>
    </td>
    <td class="cellrowborder" valign="top" width="25%" headers="mcps1.2.5.1.4 "><p id="zh-cn_topic_0000002037945009_p0608121425015">√</p>
    </td>
    </tr>
    <tr id="zh-cn_topic_0000002037945009_row960821445015"><td class="cellrowborder" valign="top" width="24.97%" headers="mcps1.2.5.1.1 "><p id="zh-cn_topic_0000002037945009_p4608191420508">Cube Pipeline视图</p>
    </td>
    <td class="cellrowborder" valign="top" width="23.54%" headers="mcps1.2.5.1.2 "><p id="zh-cn_topic_0000002037945009_p18608111416506">-</p>
    </td>
    <td class="cellrowborder" valign="top" width="26.490000000000002%" headers="mcps1.2.5.1.3 "><p id="zh-cn_topic_0000002037945009_p86083143506">√</p>
    </td>
    <td class="cellrowborder" valign="top" width="25%" headers="mcps1.2.5.1.4 "><p id="zh-cn_topic_0000002037945009_p1560831413509">√</p>
    </td>
    </tr>
    </tbody>
    </table>

    > [!NOTE]    
    > Ascend 950 系列产品的Vector内存单元视图仅支持SIMT视图，暂不支持SIMD视图。

**使用说明**

每个单元/通路的Roofline性能分析结果由横轴、纵轴、屋顶线、带宽斜线和实际运行坐标点组成，具体请参见[**图 5**  Roofline示意图](#Roofline示意图)。

**图 5**  Roofline示意图<a id="Roofline示意图"> </a> 
![](../figures/Roofline示意图.png "Roofline示意图")

- 横轴：代表算术强度（Arithmetic Intensity），即某一单元或通路中总的浮点运算次数与总的访存数据量之比，单位为Ops/Byte。
- 纵轴：表示计算性能（Performance），即每秒可执行的浮点操作数，单位为TOps/s。
- 屋顶线：指图中顶部的水平线，代表NPU的理论最大计算性能。无论算术强度如何提高，应用的实际性能都不可能超过硬件上限。
- 带宽斜线：指图中与屋顶线相交的斜线，其与纵轴的交点取决于理论最大带宽。当理论最大带宽乘以算术强度小于NPU理论最大计算性能时，能达到的最大算力随算术强度的增加而线性增长。

    > [!NOTE]    
    > 屋顶线和带宽斜线组合成算子能达到的理论最大算力，可以概括为min（NPU理论最大计算性能，理论最大带宽\*实际算术强度）。

- 实际运行坐标点的参数构成请参见[**表 7**  实际运行坐标点说明](#实际运行坐标点说明)。

    **表 7**  实际运行坐标点说明<a id="实际运行坐标点说明"></a>

    |坐标参数|说明|
    |---|---|
    |带宽（Bandwidth）|该单元/通路的理论最大带宽。|
    |算术强度（Arithmetic Intensity）|算子实际运行时的算术强度，即横轴坐标值。|
    |性能（Performance）|算子实际运行时的计算性能，即纵轴坐标值。|
    |性能百分比（Performance Ratio）|算子实际运行时的计算性能与当前数据量下的理论最大计算性能比值，即图中a/b的百分比。|

Roofline分析视图分析算子的性能百分比，并提供以下客观分析结果：

- 算子性能百分比大于80%时，按照所在区域进行提示，有以下两种情况。
    - Compute Bound：计算瓶颈。
    - Memory Bound：内存瓶颈。

- 算子性能百分比小于80%，Bound类型为Latency Bound，有以下三种情况：
    - 若最大的pipeline ratio小于80%，提示latency bound:pipeline caused。
    - 若最大的pipeline ratio大于80%，需识别最大pipeline ratio的类型。
        - 若最大pipeline ratio的类型是compute pipeline (cube ratio、vector ratio、scalar ratio)，提示latency bound:compute caused。
        - 若最大pipeline ratio的类型是memory pipeline(MTE1 ratio、MTE2 ratio、MTE3 ratio)，提示latency bound:memory caused。

## Cache热力图

### 功能说明

针对用户程序Kernel函数内的L2 Cache访问情况，msOpProf工具可以记录并通过MindStudio Insight工具进行可视化呈现Cache热力图，该热力图可显示对应指令信息，以便用户优化L2Cache命中率，从而优化算子程序。

**注意事项**

- MindStudio Insight具体操作和详细字段解释请参考《MindStudio Insight算子调优》的“[缓存（Cache）](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/operator_tuning.md#%E7%BC%93%E5%AD%98%EF%BC%88cache%EF%BC%89)”章节。
- 添加`-g`编译选项会在生成的二进制文件中附带调试信息，建议限制带有调试信息的用户程序的访问权限，确保只有授权人员可以访问该二进制文件。
- 若不使用`llvm-symbolizer`组件提供的相关功能，输入msOpProf的程序编译时不包含-g即可，msOpProf工具则不会调用`llvm-symbolizer`组件的相关功能。
- Cache热力图功能不适用于<term>Atlas 推理系列产品</term>。
- MC2算子和LCCL算子均不支持生成Cache热力图。

### 使用说明

Cache热力图界面显示如下。

**图 6**  Cache热力图  
![](../figures/Cache热力图.png "Cache热力图")

- Hit展示Cacheline的命中情况，Miss展示Cacheline未命中情况，以便用户分析L2Cache的使用情况，
- 在缓存（Cache）界面，选择命中和未命中事件图，单击放大，在放大的事件图中右键单击所选内存单元格，选择“显示指令”，可跳转至源码（Source）界面，并高亮显示相关指令行。

    **图 7**  Cacheline对应的算子代码热点图  
    ![](../figures/Cacheline对应的算子代码热点图.png "Cacheline对应的算子代码热点图")

    > [!NOTE]    
    > 若要使用Cache热力图跳转至算子代码热点图功能，需参考[msOpProf配置](#工具使用)，提前进行配置。

## 流水图

流水图分为通算流水图、pipe流水图、warp流水图、指令流水图。其中通算流水图、pipe流水图、指令流水图三种流水图不能同时存在，warp流水图可与三者其一同时存在。详细分类及说明请参见下表。

|名称|功能|注意事项|
|---|---|---|
|[pipe流水图](#pipe流水图)|展示算子各个Pipe的运行情况|需通过参数`--aic-metrics=pipeTimeLine`开启|
|[指令流水图](#指令流水图)|展示指令耗时以及指令PC、调用栈等信息|需通过参数`--aic-metrics=instrTimeLine`开启|
|[通算流水图](#通算流水图)|展示通算运行情况、指令耗时等信息|当前仅支持MC2和LCCL类型的通算融合算子以及使能了PrintTimeStamp的通算类算子|
|[warp流水图](#warp流水图)|展示各warp的耗时情况|当前仅支持SIMT类型|

### Pipe流水图

#### 功能说明

对算子进行调优后，生成的trace.json和visualize\_data.bin文件可通过MindStudio Insight进行可视化呈现，能够直观看到算子各个Pipe的运行情况，协助开发者识别算子瓶颈。

MindStudio Insight具体操作和详细字段解释请参考《MindStudio Insight算子调优》的“[时间线（Timeline）](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/operator_tuning.md#%E6%97%B6%E9%97%B4%E7%BA%BF%EF%BC%88timeline%EF%BC%89)”章节。

#### 使用说明

- 生成的visualize\_data.bin文件可导入MindStudio Insight进行可视化呈现，展示算子在各个AI CORE各单元的活跃情况，界面如下。
- Pipe流水图特性是基于采样实现的，和用户开启的核数没有直接关系。若用户开启全部的核，最终也只会展示6个核的数据。
- 若使用MarkStamp打点时出现打点数据丢失情况，建议降低打点数目和密集程度。

**图 8**  Pipe流水图  

![](../figures/1-2.png)

支持用户通过[AscendC::MarkStamp接口](https://www.hiascend.com/document/detail/zh/CANNCommunityEdition/900beta2/API/ascendcopapi/atlasascendc_api_07_00264.html)在算子kernel侧任意代码处进行流水图打点，用以标识流水范围。使用接口在vector打上一个id为13的点，会在图上的scalar单元和vector单元展示MarkStamp13，具体请参见[**图 9**  自定义打点图](#自定义打点图)。

**图 9**  自定义打点图<a id="自定义打点图"></a>  
![](../figures/自定义打点图.png "自定义打点图")

> [!NOTE]    
> 
>- 在scalar上进行打点，只会产生一条打点数据，既表示下发也表示执行。在其他单元上进行打点，会产生两条数据，一条是scalar上的点，表示打点指令下发，一条是对应单元上的点，表示打点指令执行。
>- SIMT函数里不支持打点。

### 指令流水图

#### 功能说明

对算子进行调优后，生成的trace.json和visualize_data.bin文件可通过MindStudio Insight进行可视化呈现，借助算子上板指令级流水图，可精准采集每条指令的实际运行耗时，掌握全局指令调度时序与运行状态，协助开发者开展指令调度优化、指令序列重排等深度调优工作。

MindStudio Insight具体操作和详细字段解释请参考《MindStudio Insight算子调优》的“[时间线（Timeline）](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/operator_tuning.md#%E6%97%B6%E9%97%B4%E7%BA%BF%EF%BC%88timeline%EF%BC%89)”章节。

#### 使用说明

- 通过参数`--aic-metrics=instrTimeLine`开启上板指令级别流水图功能，--instr-timeline-pipe用于指定流水图支持的pipe，例如`--aic-metrics=instrTimeLine --instr-timeline-pipe="mte1|vector"`。支持的pipe如下：cube、fixp、vector、mte1、mte2、mte3。每个pipe指令限制条数为1024条。
- 开启-g算子编译选项可开始指令级流水图PC和调用栈信息。
- 生成的visualize_data.bin文件可导入MindStudio Insight进行可视化呈现，展示上板指令流水图情况，界面如下。

**图 10**  指令流水图<a id="指令流水图"></a>  
![](../figures/指令流水图.png "指令流水图")

### 通算流水图

#### 功能说明

通过msOpProf运行模式对通算融合算子进行调优后，生成的trace.json和visualize\_data.bin文件可通过MindStudio Insight进行可视化呈现，能够直观看到通算运行情况、指令耗时等信息，协助开发者识别通算瓶颈。支持通过AscendC API进行性能打点，采集代码在算子block上的实际耗时情况，用于通算算子性能的分析和优化。当前仅支持MC2和LCCL类型的通算融合算子以及使能了PrintTimeStamp的通算类算子。

**注意事项**

- MindStudio Insight具体操作和详细字段解释请参考《MindStudio Insight算子调优》的“[时间线（Timeline）](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/operator_tuning.md#%E6%97%B6%E9%97%B4%E7%BA%BF%EF%BC%88timeline%EF%BC%89)”章节。
- 添加`-g`编译选项会在生成的二进制文件中附带调试信息，建议限制带有调试信息的用户程序的访问权限，确保只有授权人员可以访问该二进制文件。
- 上板流水图功能在KFC场景中使用时，需要注意遵循相关接口使用规则，部分场景下Cube侧的数据无法呈现，需要结合KFC相关接口原理查看算子打点数据。

#### 使用说明

trace.json文件可分别通过Chrome浏览器和MindStudio Insight展示，visualize_data.bin文件仅可通过MindStudio Insight展示。

通过AscendC API进行性能打点具体用法请参照《Ascend C算子开发接口》中的“调试接口>上板打印>[PrintTimeStamp](https://www.hiascend.com/document/detail/zh/canncommercial/850/API/ascendcopapi/atlasascendc_api_07_00002.html)”章节。该功能可结合`--custom-input`参数，将PrintTimeStamp的descid在通算流水图上映射成自定义字符串进行展示。json示例文件参考如下<a id="json示例文件"></a>，字段详情请参见下表：

```json
{
    "mc2_trace_config":{
        "enable_block_time":false,
        "descid_display":{
            "65541":"all_gather",
            "0x10006":"end",
            "0x10007":"start"
        }
    }
}
```

|字段|说明|
|---|---|
|mc2_trace_config|MC2算子流水配置。|
|enable_block_time|是否展示各block运行时间详情，默认true。|
|descid_display|descid展示名称映射列表，PrintTimeStamp接口传入的descid将展示为映射的字符串，descid支持10进制和16进制两种字符串，映射名称支持a-zA-Z0-9-_和空格，最长64个字符，支持最多1024个映射。|

- Chrome浏览器

    在Chrome浏览器中输入“chrome://tracing”地址，并将生成的通算流水图文件（trace.json）拖到空白处打开，键盘上输入快捷键（W：放大，S：缩小，A：左移，D：右移）可进行查看。关键字段说明如[**表 8**  关键字段说明](#关键字段说明)。

    **表 8**  关键字段说明<a id="关键字段说明"></a>

    |字段名|字段功能|MC2算子|LCCL算子|ASC通算算子|
    |---|---|---|---|--------|
    |AI CORE|算子在AI CORE上的整体运行情况。|支持|支持|不支持|
    |AI CPU|算子在AI CPU上的整体运行情况。|支持|不支持|不支持|
    |TURN|算子在AI CPU上不同通信轮次的流水。|支持|不支持|不支持|
    |AIC BLOCK|算子在AI CORE各Cube核上的整体运行情况和关键接口调用情况。|支持|支持|支持|
    |AIV BLOCK|算子在AI CORE各Vector核上的整体运行情况和关键接口调用情况。|支持|支持|支持|
    |HCCL|通过HCCL通信的算子在多卡间的集合通信流水。|支持|不支持|不支持|
    |HCCL TASK|通过HCCL通信的算子在多卡间的集合通信任务执行流水。|支持|不支持|不支持|
    |AscendC API|展示用户通过AscendC API在每个block上的运行耗时情况|支持|不支持|支持|

- MindStudio Insight

    将生成的trace.json文件或visualize\_data.bin文件可导入MindStudio Insight进行可视化呈现。

    **图 11**  通算流水图  
    
    ![](../figures/1-1.png)

    - 展示算子在AI CPU和AI CORE的耗时掩盖情况，用于评估通算融合算子的性能。
    - 展示算子在AI CPU上的不同通信轮次的流水。
    - 展示算子在各BLOCK上的运行时间及关键接口调用流水。
    - 展示通过HCCL通信的算子在多卡间运行时的集合通信流水及集合通信任务流水。

        > [!NOTE] 
        > 
        > - MC2算子支持对<term>Atlas A2 训练系列产品/Atlas A2 推理系列产品</term>的AllReduce、AllGather、ReduceScatter、AlltoAll等接口及<term>Atlas A3 训练系列产品/Atlas A3 推理系列产品</term>的AllGather、ReduceScatter、AlltoAllV等接口进行调用，具体介绍请参见《Ascend C算子开发接口》中的“高阶API \> Hccl \>  [Hccl Kernel侧接口](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/ascendcopapi/atlasascendc_api_07_0869.html)”章节，添加`-g`编译选项后，单击具体接口将会关联代码行调用栈。
        > - MC2算子和LCCL算子及普通通算算子的支持情况请参考[**表 8**  关键字段说明](#关键字段说明)。
        > - Ascend 950 系列产品支持打点流水图功能。

    **图 12**  算子运行耗时情况图

    ![](../figures/msopprof1.png)

    通过使用同一个descid的打点API，在算子kernel侧进行两次标识，工具会绘制出这两个标识之间，每个block上该算子的运行耗时情况。

### Warp流水图

#### 功能说明

对算子进行调优后，生成的trace.json和visualize_data.bin文件可通过MindStudio Insight进行可视化呈现，能够直观展示各warp耗时等相应信息，协助开发者识别算子瓶颈。

MindStudio Insight具体操作和详细字段解释请参考《MindStudio Insight算子调优》的“[时间线（Timeline）](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/operator_tuning.md#%E6%97%B6%E9%97%B4%E7%BA%BF%EF%BC%88timeline%EF%BC%89)”章节。

#### 使用说明

- 生成的visualize_data.bin文件可导入MindStudio Insight进行可视化呈现，展示warp的耗时情况，界面如下。
- warp流水图当前仅支持Ascend 950 系列产品。

**图 13**  warp流水图<a id="warp流水图"></a>  
![](../figures/warp流水图.png "warp流水图")

> [!NOTE]
> 
> - 在simt entry开始和结尾插入了一部分代码，对算子耗时会产生一定影响。
> - warp耗时数据的采集在独立的重放过程中完成，不影响其他类型数据，也不建议与其他各类数据做对比。

## 算子代码热点图

### 功能说明

通过msOpProf运行模式生成的visualize\_data.bin文件可通过MindStudio Insight进行可视化呈现。界面支持查看算子源码与指令集的映射关系、耗时情况等功能，可协助开发者识别热点代码分布，并分析热点函数优化的可行性。

**注意事项**

- MindStudio Insight具体操作和详细字段解释请参考《MindStudio Insight算子调优》的“[源码（Source）](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/operator_tuning.md#%E6%BA%90%E7%A0%81%EF%BC%88source%EF%BC%89)”章节。
- 添加`-g`编译选项会在生成的二进制文件中附带调试信息，建议限制带有调试信息的用户程序的访问权限，确保只有授权人员可以访问该二进制文件。
- 算子程序编译时需要包含-g，否则msOpProf不会展示热点图，也不调用llvm-symbolizer组件的相关功能实现代码与PC映射。
- msOpProf算子代码热点图功能不适用于<term>Atlas 推理系列产品</term>。
- MC2算子和LCCL算子均不支持生成算子代码热点图。

### 使用说明

算子代码热点图界面如下。

**图 14**  msOpProf源码界面  
![](../figures/msopprof源码界面2.png "msopprof源码界面2")

- 在界面顶部，可切换计算单元和核函数文件。
- 在左侧界面，提供算子核函数各行代码模拟L2Cache命中率、与GM有关的数据搬运量及对应的指令数，帮助开发者快速定位瓶颈代码行。
- 在右侧界面，提供具体的指令维度模拟L2Cache命中率、与GM有关的数据搬运量、执行次数及与代码相关联，帮助开发者进一步分析代码耗时长的原因。
- MindStudio Insight时间线和详情页面中L2Cache命中率的差异请参见[**表 9** MindStudio Insight L2Cache命中率对比表](#L2Cache命中率对比表)。

    **表 9** MindStudio Insight L2Cache命中率对比表<a id="L2Cache命中率对比表"></a>

    |页面位置|数据来源|维度|
    |---|---|---|
    |时间线|工具模拟|代码行和指令维度。|
    |详情|真实存在|核维度。|

    > [!NOTE]   
    > 查看与GM有关的数据搬运量（Process Bytes）时，不涉及GM单元的情况都显示为NA。

- msOpProf具体特性支持情况请参见[**表 10**  msOpProf热点图的功能介绍](#msOpProf热点图的功能介绍)及[**表 11**  Stall参数说明](#Stall参数说明)。

    **表 10**  msOpProf热点图的功能介绍<a id="msOpProf热点图的功能介绍"></a>

    |列名|Atlas A2 训练系列产品/Atlas A2 推理系列产品|Atlas A3 训练系列产品/Atlas A3 推理系列产品|Atlas 推理系列产品|Ascend 950 系列产品|说明|
    |-------|-------|------|-------|-------|--------|
    |源码|支持|支持|不支持|支持|-|
    |指令PC地址|支持|支持|不支持|支持|-|
    |PIPE|支持|支持|不支持|支持|-|
    |执行次数|支持|支持|不支持|支持|查看算子源码与指令的执行次数。|
    |GPR Count|不支持|不支持|不支持|支持|查看寄存器使用情况。<br>不支持使用TRACE_START和TRACE_STOP接口查看部分算子的寄存器使用情况。|
    |GPR Status|不支持|不支持|不支持|支持|查看寄存器使用状态。<br>当前寄存器共有五种状态：Space=0（不显示在可视化工具中）、Read=1（右空心箭头）、Write=2（左实心箭头）、ReadAndWrite=3（左实心箭头与右空心箭头）、InUsed=4（竖线）。|
    |L2Cache命中率|支持|支持|不支持|不支持|模拟代码行和指令维度。|
    |Process Bytes|支持|支持|不支持|支持|查看与GM有关的数据搬运量。|
    |Stall Sampling(All Samples)|不支持|不支持|不支持|支持|指令在执行过程中，由于资源冲突、数据依赖等原因而产生的等待次数，包含active。|
    |Stall Sampling(Not Issue)|不支持|不支持|不支持|支持|指令在执行过程中，由于资源冲突、数据依赖等原因而产生的等待次数，不包含active。|

    **表 11**  Stall参数说明<a id="Stall参数说明"></a>

    |名称|说明|
    |-------|-------|
    |Nop_Cycles（nop阻塞）|空操作周期。|
    |Divergence_Stack_Spill（Divergence栈溢出阻塞）|分支发散栈溢出。在执行分支指令时，由于栈空间不足或分支路径导致的预测与实际执行路径不一致，从而引发的流水线阻塞周期。|
    |IBuf_Empty（IBuf空阻塞）|指令缓冲为空。指IBuf为空时，因无法获取下一条指令而导致的流水线阻塞周期。|
    |Register_bank_conflict（寄存器冲突阻塞）|寄存器Bank冲突。由于指令访问同一组寄存器时发生冲突，导致流水线暂停以避免数据错误的阻塞周期。|
    |Resource_conflict（资源冲突阻塞）|资源冲突。由于多个指令同时竞争同一硬件资源而无法并行执行，导致流水线暂停的阻塞周期。|
    |Scoreboard_Not_Ready（ScoreBoard未准备阻塞）|记分牌未就绪。由于ScoreBoard状态尚未就绪（如前序指令仍在占用相关资源或寄存器），当前指令无法继续执行而导致的流水线阻塞。|
    |Warp_Level_Sync（Warp Level同步阻塞）|Warp级同步等待。在多线程执行中，为确保同Warp内线程的同步与一致性，在未满足同步条件时对指令执行进行的阻塞。|
    |active|无阻塞成功发射指令的次数。|
    |Others（其他阻塞）|指除已明确分类的阻塞原因外，由其他异常或控制逻辑触发的流水线暂停周期。|

## Warp Stall热点图

### 功能说明

通过msOpProf运行模式生成的visualize_data.bin文件可通过MindStudio Insight进行可视化呈现，主要展示指令在执行过程中，由于资源冲突、数据依赖等原因而产生的等待次数，包含active。

MindStudio Insight具体操作请参考《MindStudio Insight算子调优》的“[详情（Details）](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/operator_tuning.md#%E8%AF%A6%E6%83%85%EF%BC%88details%EF%BC%89)”章节。

### 使用说明

visualize\_data.bin文件通过MindStudio Insight工具展示的界面如下图所示。参数解释见[表 11 Stall参数说明](#Stall参数说明)。

**图 15**  Warp Stall热点图
![](../figures/warp_stall热点图.png "Warp Stall热点图")

> [!NOTE]
> 
> - 仅Ascend 950 系列产品支持该功能。
> - 该功能在开启参数`--aic-metrics=PcSampling`时生效。
