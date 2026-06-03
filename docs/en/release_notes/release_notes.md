# MindStudio Ops Profiler Release Notes

## Version Mapping

### Product Versions

| Product Name| Version | Version Type|
|------|-------|------|
| msOpProf | 26.0.0 | Internal test version|
| msOpProf | 8.3.0 | Official version|

### Related Product Versions

| msOpProf Version| CANN Version| Python Version| JSON Version|SecureC Version| Makeself Version| llvm-project Version
|----------|-----------------|----------|----------|----------|----------|----------|
| 26.0.0 | 9.0.0 or later (recommended)| 3.11 or later (recommended)| v3.12.0 or later| v1.1.16 or later| release-2.5.0 or later| 22.1.2 or later|
| 8.3.0 | 8.2.RC1 or later| 3.11 or later (recommended)| v3.12.0 or later| v1.1.16 or later| release-2.5.0 or later| 22.1.2 or later|

## Version Compatibility

### 26.0.0

1. Adapted to changes in BiSheng Compiler options.
2. Added compatibility with multiple new chip specifications and model variants and updated to accommodate changes in chip identification within CANN.

## Feature Updates

### 26.0.0

#### 1. New Features

Functional changes:

- New features:

1. Added support for performance tuning of the `shmem` and `asc` operator libraries.
2. Added performance analysis for custom communication and computing integrated frameworks. You can use AscendC API instrumentation to generate communication and computing pipeline charts.
3. Added fine-grained Scalar performance data analysis to help identify specific bottleneck locations in the Scalar unit. Expanded related performance metrics in performance data files and computing memory heatmap - memory workload analysis.
4. Added support for SIMT VF instruction stall analysis and register utilization display. Expanded related performance metrics in on-board code hot spot maps.
5. Added support for SIMT VF instruction issue efficiency statistics and load balancing analysis. Expanded related performance metrics in computing memory heatmap - core occupancy view.

- Optimizations:

1. Optimized help information display based on the features supported by different chips.
2. Optimized theoretical bandwidth values and performance metric formulas for certain chip models.
3. Optimized lane sorting and instruction color coding in the simulation pipeline charts.
4. Optimized instruction information sorting in the code hot spot map.
5. Enhanced software security by updating permissions and groups for build artifacts and output files.

Build and release:

1. Optimized dependency repository download functionality, increasing download speeds by 10 times.
2. Added a debug build option to support breakpoint debugging of build artifacts.
3. Renamed the installation package to `mindstudio-opprof_linux.run`.

Documentation:

1. Updated README links and added documents like the user guide and images to the `docs` repository.
2. Renamed the installation guide to `msopprof_install_guide.md`.
3. Modified and optimized the installation guide and quick start content. Added a prompt for the `pigz` dependency in the installation documentation.
4. Added a quick start document.

#### 2. Deleted Features

None

#### 3. Bug Fixes

None

### 8.3.0

#### 1. New Features

This issue is the first official release. The following features are added:

msOpProf mode:

1. Computing memory heatmap: Displays basic operator information, comput workload analysis data, and memory workload analysis data by resource, allowing developers to identify resource bottlenecks from a comprehensive perspective.
2. Roofline bottleneck analysis chart: Builds a processor performance model, which can be used to quickly evaluate the theoretical performance limit of an operator, allowing developers to quickly identify bottlenecks.
3. Communication and computing pipeline chart (for MC2 operators): Allows developers to intuitively see operator running status and instruction time consumption, which helps identify operator bottlenecks. Performance annotation is supported through Ascend C APIs to collect actual execution time of code on operator blocks, used for analysis and optimization of communication and compute operators performance.
4. Pipeline chart: Allows developers to intuitively see the running status of each pipeline.
5. Operator code hot spot map: Displays the mapping between operator source code and instructions, as well as the time consumption. This helps developers identify hot spot code distribution and analyze the feasibility of hot spot function optimization.
6. Cache heatmap: Visualizes the cache heatmap and displays corresponding instruction information to help optimize L2 cache hit rates.
7. Profile data files: Provides detailed profile data for operators across multiple performance metric dimensions.

msOpProf simulator mode:

1. Instruction pipeline chart: Displays timing relationship by instruction and associates with the call stack to quickly locate bottlenecks.
2. Operator code hot spot map: Displays the mapping between operator source code and instructions, as well as the time consumption. This helps developers identify hot spot code distribution and analyze the feasibility of hot spot function optimization.
3. Memory channel throughput waveform chart: Provides the statistical analysis of the memory bandwidth of the operator MTE log channel over time, helping you identify the bandwidth usage of the operator during different operator stages and evaluate the feasibility of bandwidth optimization.

#### 2. Deleted Features

None

#### 3. Bug Fixes

None
