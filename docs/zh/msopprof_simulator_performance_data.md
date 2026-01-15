# **msopprof simulator模式性能数据**

## 代码行耗时数据文件

代码行耗时数据文件core\*\_code\_exe.csv。

core\*.veccore\* 或core\*.cubecore\*目录下存放各计算单元的代码行耗时文件，例如core0.veccore1目录下的core0.veccore1\_code\_exe.csv文件，“core0”代表核编号，“veccore1”代表子核编号。

**图 1**  core\*\_code\_exe.csv文件  
![](./figures/core-_code_exe-csv文件.png "core-_code_exe-csv文件")

关键字段说明如下。

**表 1**  字段说明

|字段名|字段解释|
|--|--|
|code|代码行，格式为代码文件路径:行号。|
|call_count|对应代码行所涉及指令的调用次数。|
|cycles|该代码行所涉及的指令在AI Vector Core/AI Cube Core上执行的cycle总数。|
|running_time(us)|代码行的有效执行时间，单位us。|

## 代码指令信息文件

代码指令详细信息文件core\*\_instr\_exe.csv。

core\*.veccore\* 或core\*.cubecore\*目录下存放各计算单元的代码指令详细信息文件，例如core0.veccore0目录下core0.veccore0\_instr\_exe.csv，“core0”代表核编号，“veccore0”代表子核编号。

**图 1**  core\*\_instr\_exe.csv文件  
![](./figures/core-_instr_exe-csv文件.png "core-_instr_exe-csv文件")

关键字段说明如下。

**表 1**  字段说明

|字段名|字段解释|
|--|--|
|instr|代码指令名称。|
|addr|代码指令对应的PC地址。|
|pipe|PIPE类型，包括指令队列和计算单元。|
|call_count|该指令的调用次数。|
|cycles|该指令在AI Vector Core/AI Cube Core上执行的cycle总数。|
|running_time(us)|指令的有效执行时间，单位us。|
|detail|指令执行的详细参数。|
