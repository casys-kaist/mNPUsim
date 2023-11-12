# Architecture Configuration
## Overview
mNPUsim receives 'architecture configuration list file' (.txt) name as a first input. \
'Architecture configuration list file' contains list of 'architecture configuration file' (.csv) name, one by one in each line.

## Architecture Configuration File Specification
* arch_name: Name (or nickname) of the NPU architecture.
* systolic_height, systolic_width: Height and width of systolic array.
* sram_ifmap_size, sram_filter_size, sram_ofmap_size: Size of SRAM assigned for ifmap, filter, ofmap. Must be a multiple of cacheline_size.
* dataflow_type: Systolic array dataflow. Currently, only output-stationary (denoted as 'os') dataflow is supported.
* element_unit: Size of tensor element in Byte.
* cacheline_size: Size of SPM block (word).
* tile_ifmap_size, tile_filter_size, tile_ofmap_size: Tile size. Should be set to half of sram size for double buffering. Must be a multiple of cacheline_size.
* unit_compute: cycle time of MAC operation. 1 for default.
