#ifndef __UTIL_H__
#define __UTIL_H__

#include "common.h"
#include "accelerator.h"

void read_config(npu_accelerator *ng, string arch_file, string network_file, vector<string> input_co_runners);
void read_arch_config(npu_accelerator *ng, string file_name);
void read_network_config(npu_accelerator *ng, string file_name);
uint64_t random_value(uint64_t max_input, uint64_t min_input);

#endif
