# **msopprof Simulator Mode Performance Data**

## Code Line Time Consumption Data Files

The code line time consumption data files are `core*_code_exe.csv`.

The code line time consumption files for each compute unit are stored under the `core*.vecore*` or `core*.cubecore*` directories. For example, the `core0.veccore1_code_exe.csv` file in the `core0.veccore1` directory, where `core0` represents the core number and `veccore1` represents the sub-core number.

**Figure 1** core*_code_exe.csv file 
![](../figures/core-_code_exe-csv-file.png "core-_code_exe.csv file")

The key fields are as follows.

**Table 1** Field description

|Field|Description|
|--|--|
|code|Code line. The format is *code file path:line number*.|
|call_count|Number of calls to instructions involved in a code line.|
|cycles|Total number of cycles in which instructions involved in a code line are executed on the AI Vector Core/AI Cube Core.|
|running_time(us)|Valid execution time of a code line, in μs.|

## Code Instruction Information Files

Detailed code instruction information files are `core*_instr_exe.csv`.

The detailed code instruction information files for each compute unit are stored under the `core*.veccore*` or `core*.cubecore*` directories. For example, the `core0.veccore0_instr_exe.csv` file in the `core0.veccore0` directory, where `core0` represents the core number and `veccore0` represents the sub-core number.

**Figure 2** core*_instr_exe.csv file 
![](../figures/core-_instr_exe-csv-file.png "core-_instr_exe.csv file")

The key fields are as follows.

**Table 2** Field description

|Field|Description|
|--|--|
|instr|Name of a code instruction.|
|addr|PC address corresponding to a code instruction.|
|pipe|PIPE type, including the instruction queue and compute unit.|
|call_count|Number of times that an instruction is called.|
|cycles|Total number of cycles in which an instruction is executed on the AI Vector Core/AI Cube Core.|
|running_time(us)|Valid execution time of an instruction, in µs.|
|detail|Detailed parameters for executing an instruction.|
