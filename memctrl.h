#include "DRAMsim3/src/dramsim3.h"
#include "spm.h"
#include "tlb.h"

#ifndef __MEMCTRL_H__
#define __MEMCTRL_H__

using namespace dramsim3;

struct DRAMPacket
{
    uint32_t npu_idx;
	uint32_t spm_idx;
    uint32_t size;
	bool is_write;
    uint64_t paddr;
    uint64_t page_addr;
	uint64_t vaddr;
};


struct NPUMemConfig //per-NPU
{
	uint64_t tlb_hit_latency = 5;//cycle
	uint64_t tlb_miss_latency = 400;//cycle
	uint32_t tlb_assoc = 1;//Size of set (i.e. way)
	uint32_t tlb_entrynum = 2048;//setnum = entrynum/assoc
    uint32_t tlb_portnum = 4;
	uint32_t npu_clockspeed = 1;
	uint32_t dram_clockspeed = 1;
    uint32_t ptw_num = 8;
    uint32_t pt_step_num = 4;
	uint64_t spm_latency = 1;//cycle
	uint32_t spm_size = 1048576;//B
	uint32_t spm_wordsize = 64;//B
    uint32_t token_bucket = 256;
    uint32_t token_epoch = 2;
	uint8_t tlb_pref_mode = 0;//0 for no pref, 1 for SP, 2 for ASP, 3 for ...
	bool is_double_buffer = true;
    bool is_tpreg = false;
};


struct MemoryConfig //shared
{
    string dramconfig = "dram_config/HBM2_8Gb_x128.ini";
    string dramoutdir = "dramsim_output";
	uint64_t dram_capacity = 2l<<33;//8GB -> 2^33B. Not 21, 2L!)
    uint32_t npu_num = 1;
    uint32_t dram_unit = 64;//B
	uint32_t pagebits = 12;
    uint32_t channel_num = 8;
    uint32_t module_num = 1;
    bool is_dram_log = true;
	bool is_dynamic_partition = true;
	bool is_twolev_tran = false;
    bool is_shared_tlb = false;
	bool is_shared_l2 = false;
    bool is_shared_ptw = false;
    bool is_tiled_bw = false;
    bool is_token_bw = false;
    bool is_conserv_token = false;
    bool is_return_ptw = false;
};


class NPUMemory
{//synchronized with DRAM tick
};

class MemoryController
{
};


void readMemConfig(MemoryConfig* memconfig, char* file_name);
void readNPUMemConfig(NPUMemConfig* config, char* file_name);

#endif
