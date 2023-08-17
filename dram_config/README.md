# DRAM Configuration
## Overview
As mNPUsim - multi-npu assumes shared DRAM among multiple NPUs, only one 'dram configuration file' (.cfg) is required as a third input.

## DRAM Configuration File Specification
* dramconfig_name: Configuration file (.ini) name for DRAMsim3.
* pagebits: Length of page offset (in binary).
* npu_num: Number of NPUs.
* dramoutdir_name: Name of directory for saving DRAM/TLB logs.
* dram_unit: request_size_bytes calculated in DRAMsim3/src/configuration.cc file.
* dram_log: Flag for recording DRAM/TLB logs.
* dram_capacity: DRAM Capacity in Byte.
* dynamic_partition: Dynamic partitioning flag for DRAM. 0 for static partitioning, 1 for dynamic.
* twolev_tran: Two level translation?
* tlb_shared: (single-level) TLB shared?
* l2tlb_shared: L2 TLB shared?
* ptw_shared: PTW Shared?
* channel_num: Number of channels per Module (Default: same as defined in .ini file)
* module_num: Number of HBM modules (DRAMsim objects)
* tiled_bw: Tile-level bandwidth interleaving?
* token_bw: Fair bandwidth sharing with token?
* return_ptw
