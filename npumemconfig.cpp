#include "memctrl.h"

void readNPUMemConfig(NPUMemConfig* config, char* file_name)
{
	FILE* f = fopen(file_name, "r");
	char opbuf[100];
	char buf[100];

	while (fscanf(f, "%s %s", opbuf, buf) != EOF){
		if (!strncmp(opbuf, "tlb_hit_latency", 16)){
			config->tlb_hit_latency = (uint64_t)atol(buf);
			continue;
		}
		if (!strncmp(opbuf, "tlb_miss_latency", 17)){
			config->tlb_miss_latency = (uint64_t)atol(buf);
			continue;
		}
		if (!strncmp(opbuf, "tlb_assoc", 10)){
			config->tlb_assoc = (uint32_t)atoi(buf);
			continue;
		}
		if (!strncmp(opbuf, "tlb_entrynum", 13)){
			config->tlb_entrynum = (uint32_t)atoi(buf);
			continue;
		}
		if (!strncmp(opbuf, "npu_clockspeed", 15)){
			config->npu_clockspeed = (uint32_t)atoi(buf);
			continue;
		}
		if (!strncmp(opbuf, "dram_clockspeed", 16)){
			config->dram_clockspeed = (uint32_t)atoi(buf);
			continue;
		}
		if (!strncmp(opbuf, "spm_latency", 12)){
			config->spm_latency = (uint64_t)atol(buf);
			continue;
		}
		if (!strncmp(opbuf, "spm_size", 9)){
			config->spm_size = (uint32_t)atoi(buf);
			continue;
		}
		if (!strncmp(opbuf, "cacheline_size", 15)){
			config->spm_wordsize = (uint32_t)atoi(buf);
			continue;
		}
		if (!strncmp(opbuf, "tlb_pref_mode", 14)){
			config->tlb_pref_mode = (uint8_t)atoi(buf);
			continue;
		}
		if (!strncmp(opbuf, "double_buffer", 14)){
			config->is_double_buffer = !!(atoi(buf));
			continue;
		}
		if (!strncmp(opbuf, "tlb_portnum", 12)){
			config->tlb_portnum = (uint32_t)atoi(buf);
			continue;
		}
		if (!strncmp(opbuf, "ptw_num", 8)){
			config->ptw_num = (uint32_t)atoi(buf);
			continue;
		}
		if (!strncmp(opbuf, "pt_step_num", 13)){
			config->pt_step_num = (uint32_t)atoi(buf);
			continue;
		}
		if (!strncmp(opbuf, "is_tpreg", 9)){
			config->is_tpreg = !!(atoi(buf));
			continue;
		}
		if (!strncmp(opbuf, "token_bucket", 13)){
			config->token_bucket = atoi(buf);
			continue;
		}
		if (!strncmp(opbuf, "token_epoch", 12)){
			config->token_epoch = atoi(buf);
			continue;
		}
	}

	fclose(f);
}
