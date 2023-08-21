#include "memctrl.h"


void readMemConfig(MemoryConfig* memconfig, char* file_name)
{
	FILE* f = fopen(file_name, "r");
	char opbuf[100];
	char buf[100];

	while (fscanf(f, "%s %s", opbuf, buf) != EOF){
		if (!strncmp(opbuf, "dramconfig_name", 16)){
			memconfig->dramconfig = string(buf);
			continue;
		}
		if (!strncmp(opbuf, "dramoutdir_name", 16)){
			memconfig->dramoutdir = string(buf);
			continue;
		}
		if (!strncmp(opbuf, "pagebits", 9)){
			memconfig->pagebits = (uint32_t)atoi(buf);
			continue;
		}
		if (!strncmp(opbuf, "npu_num", 8)){
			memconfig->npu_num = (uint32_t)atoi(buf);
			continue;
		}
		if (!strncmp(opbuf, "dram_log", 9)){
			memconfig->is_dram_log = !!(atoi(buf));
			continue;
		}
		if (!strncmp(opbuf, "dram_unit", 10)){
			memconfig->dram_unit = (uint32_t)atoi(buf);
			continue;
		}
		if (!strncmp(opbuf, "dram_capacity", 14)){
			memconfig->dram_capacity = (uint64_t)atol(buf);
			continue;
		}
		if (!strncmp(opbuf, "dynamic_partition", 18)){
			memconfig->is_dynamic_partition = !!(atoi(buf));
			continue;
		}
		if (!strncmp(opbuf, "ptw_shared", 11)){
			memconfig->is_shared_ptw = !!(atoi(buf));
			continue;
		}
		if (!strncmp(opbuf, "tlb_shared", 11)){
			memconfig->is_shared_tlb = !!(atoi(buf));
			continue;
		}
		if (!strncmp(opbuf, "channel_num", 12)){
			memconfig->channel_num = (uint32_t)atoi(buf);
			continue;
		}
		if (!strncmp(opbuf, "module_num", 11)){
			memconfig->module_num = (uint32_t)atoi(buf);
			continue;
		}
		if (!strncmp(opbuf, "tiled_bw", 9)){
			memconfig->is_tiled_bw = !!(atoi(buf));
			continue;
		}
		if (!strncmp(opbuf, "token_bw", 9)){
			memconfig->is_token_bw = !!(atoi(buf));
			continue;
		}
		if (!strncmp(opbuf, "conserv_token", 14)){
			memconfig->is_conserv_token = !!(atoi(buf));
			continue;
		}
		if (!strncmp(opbuf, "return_ptw", 11)){
			memconfig->is_return_ptw = !!(atoi(buf));
			continue;
		}
	}
	fclose(f);
}
