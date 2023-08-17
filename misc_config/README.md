# Other misc Configuration
## Overview
The sixth input of mNPUsim - multi-npu is optional: a single 'misc configuration file' (.cfg) for several optional setups.\
This configuration file is only required for back-to-back execution, asynchronously-started execution, or non-greedy partitioning of shared resources (PTW, especially).

# Misc Configuration File Specification
Each line of this configuration file denotes the required setup for each NPU.\
Every field should be separated by " ".\
[start cycle] [iteration] [shared PTW partition (0 for full-dynamic)] [upper bound of shared PTW partition] [lower bound of shared PTW partition]\
Without misc configuration file, default values are always set to "0 1 0 0 [Total number of shared PTW]".

Note that, [iteration] = -1 means that repeating specific task until finishing every other workloads. -2 means that repeating all workloads until all co-runners runs at least twice.
