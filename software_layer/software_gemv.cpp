#include "../util.h"
#include "../accelerator.h"

//--------------------------------------------------
// name: software_request_generator::gemv_translation
// usage: SW request generator for MV mapped to mvunit
//--------------------------------------------------

void software_request_generator::gemv_translation()
{
	//assumption: systolic_width == 1 for the vector unit
	assert(systolic_width == 1);

	//tile size computation
	//height x width
	int sum_tile_size, mtx_tile_height, mtx_tile_width;
	sum_tile_size = (tile_ifmap_size + tile_filter_size + tile_ofmap_size)/element_unit;
    mtx_tile_width = ifmap_width;
    mtx_tile_height = ifmap_height;
	if ((mtx_tile_height * mtx_tile_width + mtx_tile_height + mtx_tile_width) > sum_tile_size){
		//calculate maximum possible size of matrix height
		mtx_tile_height = int(ceil((float)(sum_tile_size - mtx_tile_width)/(float)(mtx_tile_width + 1)));
	}

	//Mapping to mvunit
	//L2 tiling (Split original GEMV if necessary)
    //MxN mtx -> P x (MN/P) matrix
	int tile_idx, tile_num;
	tile_num = (int)ceil((float)ifmap_height/(float)mtx_tile_height);
	uint64_t mtx_tile_addr, vec_tile_addr, output_tile_addr;
	mtx_tile_addr = ifmap_base_addr;
	vec_tile_addr = filter_base_addr;
	output_tile_addr = ofmap_base_addr;
	for (tile_idx=0; tile_idx < tile_num; tile_idx++){
		int mtx_height_T, mtx_width_T, vec_orig_T; //number of elements
		if ((tile_idx != tile_num - 1) && ((mtx_tile_height * mtx_tile_width)%systolic_height == 0)){
            mtx_height_T = systolic_height;
            mtx_width_T = (mtx_tile_height * mtx_tile_width)/systolic_height;
            vec_orig_T = mtx_tile_height;
		}else if (tile_idx != tile_num - 1){
            if (mtx_tile_height > systolic_height){
                mtx_height_T = systolic_height;
                mtx_width_T = int(ceil((float)(mtx_tile_width*mtx_tile_height)/(float)mtx_height_T));
            }
            vec_orig_T = mtx_tile_height;
		}else{
            int tmpheight = ifmap_height - (mtx_tile_height * (tile_num - 1));
            if (tmpheight > systolic_height){
                mtx_height_T = systolic_height;
                mtx_width_T = int(ceil((float)(mtx_tile_width*mtx_tile_height)/(float)mtx_height_T));
            }else{
                mtx_height_T = tmpheight;
                mtx_width_T = mtx_tile_width;
            }
            vec_orig_T = tmpheight;
        }
		//setup tile addressing
		//third argument (FILTER_HEIGHT) saves the original vector length
		write_config_string += layer_name + string("_T") + to_string(tile_idx) + string(",") + \
							   to_string(mtx_height_T) + string(",") + to_string(mtx_width_T) + string(",") + to_string(vec_orig_T) + string(",") +\
							   string("1,1,1,1,Gemv,") + to_string(mtx_tile_addr) + string(",") + to_string(vec_tile_addr) + string(",") + to_string(output_tile_addr) + string(",\n");
		mtx_tile_addr += (mtx_height_T * mtx_width_T * element_unit); //size in bytes
		output_tile_addr += (vec_orig_T * element_unit); //size in bytes
	}
}



//--------------------------------------------------
// name: software_request_generator::gemv_computation
// usage: called in npu_accelerator::computation (real pre-run)
//--------------------------------------------------

void software_request_generator::gemv_computation()
{
	printf("gemv_computation for cacheline_size %dB\n", cacheline_size);
	//called after set_variable(layer_idx), init_compute_variables(), init_file_io()
	//assumes preprocessing of gemv_translation()

	//Step 1. Generate DRAM requests - next_***_set
	//matrix
	uint64_t addr = (ifmap_base_addr/cacheline_size) * cacheline_size;
	for (; addr<(ifmap_base_addr + ifmap_height*ifmap_width*element_unit); addr+=cacheline_size){
		next_ifmap_set.insert(addr);
	}
	//vector
	addr = (filter_base_addr/cacheline_size) * cacheline_size;
	for (; addr<(filter_base_addr + filter_height*element_unit); addr+=cacheline_size){
		next_filter_set.insert(addr);
	}
	//output
	addr = (ofmap_base_addr/cacheline_size) * cacheline_size;
	for (; addr<(ofmap_base_addr + (ifmap_height*ifmap_width/filter_height)*element_unit); addr+=cacheline_size){
		next_ofmap_set.insert(addr);
	}
	//Optional: Generate SRAM trace - sram_***_vector
	if (SRAM_TRACE){
		uint64_t sram_addr = ifmap_base_addr;
		for (; sram_addr<(ifmap_base_addr + ifmap_height*ifmap_width*element_unit); sram_addr+=element_unit){
			sram_ifmap_vector.push_back(sram_addr);
		}
		sram_addr = filter_base_addr;
		for (; sram_addr<(filter_base_addr + filter_height*element_unit); sram_addr+=element_unit){
			sram_filter_vector.push_back(sram_addr);
		}
		sram_addr = ofmap_base_addr;
		for (; sram_addr<(ofmap_base_addr + (ifmap_height*ifmap_width/filter_height)*element_unit); addr+=element_unit){
			sram_ofmap_vector.push_back(sram_addr);
		}
	}

	//Step 2. Calculate execution cycles for all consecutive L1 tiles with utilization
	int l1tile_idx, l1tile_num;
	l1tile_num = ifmap_width/filter_height;
	if (!SRAM_TRACE){
		pre_local_cycle = 0;
	}
	uint64_t active_pe = 0;
	uint64_t idle_pe = 0;
	//Reduction-tree based implementation
    if (!SRAM_TRACE){
		if (ifmap_width <= systolic_height){ //underutilization
			pre_local_cycle += ifmap_height*unit_compute;//mult
			pre_local_cycle += (uint64_t)(log2(systolic_height) + 1)*unit_compute;//add
			//PE activation: only for utilization calculation
			active_pe += ifmap_width;
			idle_pe += (systolic_height - ifmap_width);
		}else{
			pre_local_cycle += ifmap_height*((uint64_t)ceil((float)ifmap_width/(float)systolic_height))*unit_compute;//mult
			pre_local_cycle += (uint64_t)(log2(systolic_height) + 1)*unit_compute;//add
			active_pe += (ifmap_width);
			idle_pe += (ifmap_width%systolic_height);
		}
    }
	//Step 3. Write requests
	move_reset_dram();
	move_reset_sram();
	tile_output();
	printf("tile(gemv): %d\n", ++full_num);
	write_output(result_path, utilization_result, to_string(active_pe)+"/"+to_string(active_pe+idle_pe)+"="+to_string((double)active_pe/(active_pe+idle_pe)));
}


//--------------------------------------------------
// name: software_request_generator::address_update_gemv
// usage: gemv version of address_update() - ifmap/filter => matrix/vector
// note: input 'vector' and weight 'matrix'!
//--------------------------------------------------

void software_request_generator::address_update_gemv()
{
	uint64_t addr_tmp;
	addr_tmp = ifmap_base_addr + ifmap_width*ifmap_height*element_unit;
	new_space_addr = MAX(new_space_addr, addr_tmp);
	if(DEBUG)
		cout << "new_space_addr: " << new_space_addr << endl;

	addr_tmp = filter_base_addr + ifmap_width*element_unit;
	new_space_addr = MAX(new_space_addr, addr_tmp);
	if(DEBUG)
		cout << "new_space_addr: " << new_space_addr << endl;

	addr_tmp = ofmap_base_addr + ifmap_height*element_unit;
	new_space_addr = MAX(new_space_addr, addr_tmp);
	if(DEBUG)
		cout << "new_space_addr: " << new_space_addr << endl;
}


//--------------------------------------------------
// name: software_request_generator::set_variable_config_gemv
// usage: gemv/mvunit version of set_variable_config
//--------------------------------------------------

void software_request_generator::set_variable_config_gemv(int index)
{
	//ifmap -> matrix
	//filter -> vector
	model_name = get<0>(network_info[index]);
	layer_name = get<1>(network_info[index]);
	ifmap_height = get<2>(network_info[index]);
	ifmap_width = get<3>(network_info[index]);
	filter_height = get<4>(network_info[index]);
	filter_width = get<5>(network_info[index]);
	filter_depth = get<6>(network_info[index]);
	filter_num = get<7>(network_info[index]);
	stride = get<8>(network_info[index]);
	uint64_t uint64_minus_1 = -1;
	uint64_t uint64_minus_2 = -2;

	//matrix addressing: ignore reuse signal
	if (index == 0){
		ifmap_base_addr = 0;
	}else{
		ifmap_base_addr = new_space_addr;
	}
	new_space_addr += ifmap_height*ifmap_width*element_unit;
	ifmap_base_addr_array.push_back(ifmap_base_addr);
	//vector addressing: must consider reuse signal
	uint64_t filter_addressing = get<11>(network_info[index]);
	if (filter_addressing == uint64_minus_1){
		//decendent
		filter_base_addr = ofmap_base_addr;
	}else if (filter_addressing == uint64_minus_2){
		//separated
		filter_base_addr = new_space_addr;
		new_space_addr += filter_height*element_unit;
	}else if (filter_addressing <= BOUNDARY_IFMAP_OFMAP_ARRAY){
		//from one of ancestor outputs
		filter_base_addr = ofmap_base_addr_array[filter_addressing - 1];
	}else{
		//from one of ancestor inputs
		filter_base_addr = filter_base_addr_array[filter_addressing - 1 - BOUNDARY_IFMAP_OFMAP_ARRAY];
	}
	filter_base_addr_array.push_back(filter_base_addr);
	//output addressing: less choices than vector addressing
	uint64_t ofmap_addressing = get<12>(network_info[index]);
	if (ofmap_addressing == uint64_minus_1){
		ofmap_base_addr = new_space_addr;
		new_space_addr += ifmap_height*element_unit;
	}else if (ofmap_addressing <= BOUNDARY_IFMAP_OFMAP_ARRAY){
		ofmap_base_addr = ofmap_base_addr_array[ofmap_addressing - 1];
	}else{
		ofmap_base_addr = filter_base_addr_array[ofmap_addressing - 1 - BOUNDARY_IFMAP_OFMAP_ARRAY];
	}
	ofmap_base_addr_array.push_back(ofmap_base_addr);
}
