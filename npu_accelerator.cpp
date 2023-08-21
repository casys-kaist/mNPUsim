#include "util.h"
#include "accelerator.h"

npu_accelerator::npu_accelerator(int idx, int iteration_init, uint64_t compute_cycle_init)
{
	npu_idx = idx;
	compute_cycle = compute_cycle_init;
	iteration = iteration_init;
	iter_cnt = 0;
}


//--------------------------------------------------
// name: npu_accelerator::computation
// usage: main function for software_request_generator (simulator pre-run)
//--------------------------------------------------
void npu_accelerator::computation()
{
	max_addr = 0;
	min_addr = -1;

	for(int i=0;i<network_info.size();i++)
	{
		full_num = 0;
		cout << "prerunning: " << i+1 << " / " << network_info.size() << endl;
		//set variables
		set_variable(i);

		init_compute_variables();
		init_file_io();

		if(DEBUG)
			cout << "(ifmap_base_addr, filter_base_addr, ofmap_base_addr): " << ifmap_base_addr << ", " << filter_base_addr << ", " << ofmap_base_addr << endl;

		if(layer_type==string("Conv"))
			conv_fold_parallel_computation();
		else if(layer_type==string("Ineff_Conv"))
			conv_non_fold_parallel_computation();
		else if(layer_type==string("Pool"))
			pool_computation();
	}
}


//--------------------------------------------------
// name: npu_accelerator::calcCycle
// usage: subroutine of npu_accelerator::run (calculates execution cycle)
//--------------------------------------------------
void npu_accelerator::calcCycle()
{
	if(DEBUG)
		printf("input_local_cycle (compute_cycle %ld): %ld\n", compute_cycle, input_local_cycle);
	cout << "(" << model_name << ", " << layer_name << ", " << layer_type << "): (compute_cycle " << compute_cycle << ")" << endl;
	compute_cycle += (input_local_cycle + 1) * unit_compute;
	compute_cycle += MAX(systolic_width, systolic_height) * unit_compute;
	if(DEBUG)
		printf("compute_cycle: %ld\n", compute_cycle);
}


//--------------------------------------------------
// name: npu_accelerator::run
// usage: main function for simulator run
//--------------------------------------------------
bool npu_accelerator::run(ifstream **data_file_ptr, int* op_type, bool* need_sync)
{
	bool is_memop = false;
	while (true){
		if (!file.eof()){
			if (tile_full){
				if(DEBUG)
					printf("tile_full - write\n");
				tile_full = false;
				(*data_file_ptr) = &file_dwrite;
				(*op_type) = 1;
				(*need_sync) = true;
				is_memop = true;

				input_local_cycle = stoull(str_buf);
				getline(file, str_buf, '\n');
				str_buf.erase(remove(str_buf.begin(), str_buf.end(), '\n'), str_buf.end());
				is_first = false;
				break;
			}

			getline(file, str_buf, ',');
			str_buf.erase(remove(str_buf.begin(), str_buf.end(), ' '), str_buf.end());
			str_buf.erase(remove(str_buf.begin(), str_buf.end(), '\n'), str_buf.end());

			if(file.eof())
				break;

			if (stoull(str_buf) == 0 && !is_first){
				if(DEBUG)
					printf("tile_full - read\n");
				calcCycle();
				(*data_file_ptr) = &file_dread;
				(*op_type) = 0;
				is_returned = false;
				(*need_sync) = false;
				tile_full = true;
				is_memop = true;
				break;
			}
			input_local_cycle = stoull(str_buf);
			getline(file, str_buf, '\n');
			str_buf.erase(remove(str_buf.begin(), str_buf.end(), '\n'), str_buf.end());
			is_first = false;
		}else{
			(*data_file_ptr) = NULL;
			return false;
		}
	}

	if (!is_memop){
		printf("End of layer\n");
		calcCycle();
		(*data_file_ptr) = &file_dwrite;
		(*need_sync) = true;
		(*op_type) = 1;
	}

	return !is_memop;
}
