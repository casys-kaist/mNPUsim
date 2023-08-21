#ifndef __TP_SIM_ACCELERATOR__
#define __TP_SIM_ACCELERATOR__

#include "common.h"
#include "memctrl.h"

struct NPU_packet
{
	ifstream* data_file_ptr;
	uint32_t npu_idx;
	int op_type;
	bool eof;
	bool need_sync; //computation barrier (set when true)
};

class software_request_generator {

public:
	//arch config
	string arch_name;
	int systolic_height, systolic_width;
	uint64_t sram_ifmap_size, sram_filter_size, sram_ofmap_size;
	uint64_t tile_ifmap_size, tile_filter_size, tile_ofmap_size;
	string dataflow_type;
	int cacheline_size, element_unit;
	string result_path;

	//network config
	vector<tuple<string,string, int, int, int, int, int, int, int, string, uint64_t, uint64_t, uint64_t>> network_info;
	//co-runners info
	vector<string> co_runners;
	int my_pos; // NPU current position

	//current info
	string model_name, layer_name;
	int ifmap_height, ifmap_width;
	int filter_height, filter_width, filter_depth, filter_num;
	int stride;
	string layer_type;
	uint64_t ifmap_base_addr, filter_base_addr, ofmap_base_addr;
	int full_num;
	uint64_t max_addr, min_addr;

	void set_variable(int index);
	void set_variable_config(int index);
	void set_variable_config_im2col_conv(int index);

	//buffer
	set<uint64_t> dram_filter_set, dram_ifmap_set, dram_ofmap_set; //unit: cacheline_size
	set<uint64_t> next_filter_set, next_ifmap_set, next_ofmap_set; //unit: cacheline_size
	vector<uint64_t> sram_filter_vector, sram_ifmap_vector, sram_ofmap_vector; //unit: element
	vector<uint64_t> next_filter_vector, next_ifmap_vector, next_ofmap_vector; //unit: element

	// compute operation for os
	void init_compute_variables();
	int filter_size, filter_size_one_channel;
	int ifmap_size_one_channel;
	int ifmap_iter_width, ifmap_iter_height;
	int filter_fold, ifmap_fold;

	// core functions
	void conv_fold_parallel_computation();
	void conv_non_fold_parallel_computation();
	void pool_computation();

	// software functions (config update functions)
	void config_update(string intermediate_config_name);
	void gemm_translation();
	string write_config_string;
	uint64_t pre_local_cycle;

	bool is_full();

	// dram util functions
	void move_reset_dram();

	//sram util functions
	void move_reset_sram();

	//file name variables
	void init_file_io();
	string dram_read_intermediate;
	string dram_write_intermediate;
	string sram_read_ifmap_intermediate;
	string sram_read_filter_intermediate;
	string sram_write_ofmap_intermediate;
	string dram_read_result;
	string dram_write_result;
	string sram_read_ifmap_result;
	string sram_read_filter_result;
	string sram_write_ofmap_result;
	string execution_cycle_result;
	string avg_cycle_result;
	string utilization_result;
	string memory_footprint_result;

	//address update
	uint64_t new_space_addr=0;
	vector<uint64_t> ifmap_base_addr_array;
	vector<uint64_t> ofmap_base_addr_array;
	void address_update();

	//intermediate result
	void tile_output();

	//sram trace
	void sram_file_write(vector<uint64_t> *data, string file_name, int line_len); //print unit: element

	//MemoryController* memctrl;
	int npu_idx;

	uint64_t unit_compute = 1;
};


class npu_accelerator : public software_request_generator{
public:
	bool is_begin;
	bool is_first;
	bool tile_full;
	bool is_setup;
	bool is_returned; // memory barrier (set when false)
	ifstream file;
	ifstream file_dread;
	ifstream file_dwrite;
	string str_buf;
	uint64_t input_local_cycle;
	npu_accelerator(int idx, int iteration_init, uint64_t compute_cycle_init);
	void computation();
	void calcCycle();
	bool run(ifstream **data_file_ptr, int* op_type, bool* need_sync);
	uint64_t compute_cycle;
    int iteration;
    int iter_cnt;
};

class npu_group {
public:
	MemoryController* memctrl;
	npu_accelerator** npus;
	void pre_run();
	void run();
	void dram_load_write(ifstream *file_stream, map<uint64_t, int> *mem_buffer, int npu_idx);
	string dram_buffer_string(map<uint64_t, int> *mem_buffer, uint64_t mem_cycle);
	void mem_op(ifstream *data_file_ptr, int type, int npu_idx); 
	int mem_stall();
	void mem_finished(int npu_idx);
	void mem_sync(int npu_idx);
	void mem_flush(int npu_idx);
	int min_finish_iter();
	uint64_t npu2dramTick(uint64_t tick, int npu_idx){return memctrl->npu2dramTick(tick, npu_idx);};

	string result_path;
	uint64_t memory_cycle;
	int num_npus;
    list<int> keep_queue;
};

#endif
