# NPU-side Memory Configuration
## Overview
While DRAM is shared among multiple NPUs, TLB and SPM are still private.
To handle these efficiently, mNPUsim - multi-npu receives 'NPU-side memory configuration list file' (.txt) name as a fourth input.\
'NPU-side memory configuration list file' contains list of 'NPU-side memory configuration file' (.cfg) name, one by one in each line.

## NPU-side Memory Configuration File Specification
* template: Architecture configuration file name (to avoid confusion).
* spm_size: Size of efficient scratchpad memory (for double buffering, half of true size).
* cacheline_size: Scratchpad block (word) size.
* tlb_hit_latency, tlb_miss_latency: TLB hit/miss latency in DRAM clock cycle.
* tlb_assoc: TLB associativity (= Number of entries per set).
* tlb_entrynum: Total number of TLB entries.
* tlb_portnum: TLB bandwidth (access/cycle).
* spm_latency: Scratchpad access latency in DRAM clock cycle.
* double_buffer: Flag for double buffering.
* npu_clockspeed, dram_clockspeed: Relative value of clock frequency (NPU frequency : DRAM frequency = npu_clockspeed : dram_clockspeed). Must be coprime.
* tlb_pref_mode: TLB prefetching mode. 0 for no prefetching, 1 for sequential prefetching, ....
* ptw_num: Number of page table walkers. (ptw_num%pt_step_num must be 0)
* pt_step_num: Number of page table walk steps.
* is_tpreg: NeuMMU-like TPReg activated?
* token_bucket
* token_epoch
