#include "util.h"
#include "accelerator.h"

//--------------------------------------------------
// name: software_request_generator::address_update
// usage: update empty address space
//--------------------------------------------------
void software_request_generator::address_update()
{
	uint64_t addr_tmp;
	addr_tmp = filter_base_addr + filter_size * filter_num * element_unit;
	new_space_addr = MAX(new_space_addr, addr_tmp);
	if(DEBUG)
		cout << "new_space_addr: " << new_space_addr << endl;
	
	addr_tmp = ifmap_base_addr + ifmap_size_one_channel*filter_depth*element_unit;
	new_space_addr = MAX(new_space_addr, addr_tmp);
	if(DEBUG)
		cout << "new_space_addr: " << new_space_addr << endl;
	
	addr_tmp = ofmap_base_addr + (ceil((ifmap_height-filter_height)/(double)stride)+1)*(ceil((ifmap_width-filter_width)/(double)stride)+1)*filter_num*element_unit;
	new_space_addr = MAX(new_space_addr, addr_tmp);
	if(DEBUG)
		cout << "new_space_addr: " << new_space_addr << endl;
}

//--------------------------------------------------
// name: software_request_generator::set_variable_config
// usage: set network and security variables from (index)th layer's info. (base_addr=layer_num)
//--------------------------------------------------
void software_request_generator::set_variable_config(int index)
{
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

	//ifmap addressing
	if(index==0)
	{
		ifmap_base_addr = 0;
		new_space_addr += ifmap_height*ifmap_width*filter_depth*element_unit;
	}
	else if(get<10>(network_info[index])==uint64_minus_1)
		ifmap_base_addr = ofmap_base_addr;

	else if(get<10>(network_info[index])==uint64_minus_2)
	{
		ifmap_base_addr = new_space_addr;
		new_space_addr += ifmap_height*ifmap_width*filter_depth*element_unit;
	}
	else if(get<10>(network_info[index])<=BOUNDARY_IFMAP_OFMAP_ARRAY)
		ifmap_base_addr = ofmap_base_addr_array[get<10>(network_info[index])-1];
	else
		ifmap_base_addr = ifmap_base_addr_array[get<10>(network_info[index])-1-BOUNDARY_IFMAP_OFMAP_ARRAY];

	ifmap_base_addr_array.push_back(ifmap_base_addr);

	//filter addressing
	filter_base_addr = new_space_addr;
	new_space_addr += filter_height*filter_width*filter_depth*filter_num*element_unit;

	//ofmap addressing
	if(get<12>(network_info[index])==uint64_minus_1)
	{
		ofmap_base_addr = new_space_addr;
		if(layer_type==string("Conv") || layer_type==string("Pool"))
		{
			int ofmap_width = (int)ceil((ifmap_width - filter_width)/(double)stride)+1;
			int ofmap_height = (int)ceil((ifmap_height - filter_height)/(double)stride)+1;
			int ofmap_channel = filter_num;

			new_space_addr += ofmap_width*ofmap_height*ofmap_channel*element_unit;
		}
		else if(layer_type==string("Gemm"))
			new_space_addr += ifmap_height*filter_width*element_unit;
	}
	else if(get<12>(network_info[index])<=BOUNDARY_IFMAP_OFMAP_ARRAY)
		ofmap_base_addr = ofmap_base_addr_array[get<12>(network_info[index])-1];
	else
		ofmap_base_addr = ifmap_base_addr_array[get<12>(network_info[index])-1-BOUNDARY_IFMAP_OFMAP_ARRAY];

	ofmap_base_addr_array.push_back(ofmap_base_addr);
}

//--------------------------------------------------
// name: software_request_generator::set_variable_config_im2col_conv
// usage: set network and security variables from (index)th layer's info. (base_addr=layer_num) for im2col_conv
//--------------------------------------------------
void software_request_generator::set_variable_config_im2col_conv(int index)
{
	model_name = get<0>(network_info[index]);
	layer_name = get<1>(network_info[index]);
	ifmap_height = get<2>(network_info[index]);
	ifmap_width = get<3>(network_info[index]);
	filter_height = get<4>(network_info[index]);
	filter_width = get<5>(network_info[index]);
	filter_depth = get<6>(network_info[index]);
	filter_num = get<7>(network_info[index]);
	stride = get<8>(network_info[index]);

	uint64_t ifmap_iter_height = (int)ceil((ifmap_height - filter_height)/(double)stride)+1; //same as ofmap_height
	uint64_t ifmap_iter_width = (int)ceil((ifmap_width - filter_width)/(double)stride)+1; //same as ofmap_width
	uint64_t gemm_ifmap_height = filter_num;
	uint64_t gemm_ifmap_width = filter_width * filter_height * filter_depth;
	uint64_t gemm_filter_height = gemm_ifmap_width;
	uint64_t gemm_filter_width = ifmap_iter_width * ifmap_iter_height;

	ifmap_height = gemm_ifmap_height;
	ifmap_width = gemm_ifmap_width;
	filter_height = gemm_filter_height;
	filter_width = gemm_filter_width;
	filter_depth = 1;
	filter_num = 1;
	stride = 1;
	uint64_t uint64_minus_1 = -1;
	uint64_t uint64_minus_2 = -2;

	//ifmap addressing
	if(index==0)
	{
		ifmap_base_addr = 0;
		new_space_addr += ifmap_height*ifmap_width*filter_depth*element_unit;
	}
	else if(get<10>(network_info[index])==uint64_minus_1)
		ifmap_base_addr = ofmap_base_addr;

	else if(get<10>(network_info[index])==uint64_minus_2)
	{
		ifmap_base_addr = new_space_addr;
		new_space_addr += ifmap_height*ifmap_width*filter_depth*element_unit;
	}
	else if(get<10>(network_info[index])<=BOUNDARY_IFMAP_OFMAP_ARRAY)
		ifmap_base_addr = ofmap_base_addr_array[get<10>(network_info[index])-1];
	else
		ifmap_base_addr = ifmap_base_addr_array[get<10>(network_info[index])-1-BOUNDARY_IFMAP_OFMAP_ARRAY];

	ifmap_base_addr_array.push_back(ifmap_base_addr);

	//filter addressing
	filter_base_addr = new_space_addr;
	new_space_addr += filter_height*filter_width*filter_depth*filter_num*element_unit;
	
	if(get<12>(network_info[index])==uint64_minus_1)
	{
		ofmap_base_addr = new_space_addr;
		new_space_addr += ifmap_height*filter_width*element_unit;
	}
	else if(get<12>(network_info[index])<=BOUNDARY_IFMAP_OFMAP_ARRAY)
		ofmap_base_addr = ofmap_base_addr_array[get<12>(network_info[index])-1];
	else
		ofmap_base_addr = ifmap_base_addr_array[get<12>(network_info[index])-1-BOUNDARY_IFMAP_OFMAP_ARRAY];

	ofmap_base_addr_array.push_back(ofmap_base_addr);
}

//--------------------------------------------------
// name: software_request_generator::set_variable
// usage: set network and security variables from (index)th layer's info. (base_addr=layer_num)
//--------------------------------------------------
void software_request_generator::set_variable(int index)
{
	model_name = get<0>(network_info[index]);
	layer_name = get<1>(network_info[index]);
	ifmap_height = get<2>(network_info[index]);
	ifmap_width = get<3>(network_info[index]);
	filter_height = get<4>(network_info[index]);
	filter_width = get<5>(network_info[index]);
	filter_depth = get<6>(network_info[index]);
	filter_num = get<7>(network_info[index]);
	stride = get<8>(network_info[index]);
	layer_type = get<9>(network_info[index]);

	ifmap_base_addr = get<10>(network_info[index]);
	filter_base_addr = get<11>(network_info[index]);
	ofmap_base_addr = get<12>(network_info[index]);
}

//--------------------------------------------------
// name: software_request_generator::init_file_io
// usage: initialize input, output file name.
//--------------------------------------------------
void software_request_generator::init_file_io()
{
	string suffix_layer;
	string suffix_layerless;
	suffix_layer=arch_name+to_string(npu_idx)+"_"+model_name+"_"+layer_name;
	suffix_layerless=arch_name+to_string(npu_idx)+"_"+model_name;

	suffix_layer += ".txt";
	suffix_layerless += ".txt";

	dram_read_intermediate = result_path + INTERMEDIATE_DIR+"dram_read_"+suffix_layer;
	dram_write_intermediate = result_path + INTERMEDIATE_DIR+"dram_write_"+suffix_layer;
	sram_read_ifmap_intermediate = result_path + INTERMEDIATE_DIR+"sram_ifmap_"+suffix_layer;
	sram_read_filter_intermediate = result_path + INTERMEDIATE_DIR+"sram_filter_"+suffix_layer;
	sram_write_ofmap_intermediate = result_path + INTERMEDIATE_DIR+"sram_ofmap_"+suffix_layer;

	dram_read_result = result_path + RESULT_DIR+"dram_read_"+suffix_layer;
	dram_write_result = result_path + RESULT_DIR+"dram_write_"+suffix_layer;
	sram_read_ifmap_result = result_path + RESULT_DIR+"sram_ifmap_"+suffix_layer;
	sram_read_filter_result = result_path + RESULT_DIR+"sram_filter_"+suffix_layer;
	sram_write_ofmap_result = result_path + RESULT_DIR+"sram_ofmap_"+suffix_layer;

	execution_cycle_result = result_path + RESULT_DIR+"execution_cycle_"+suffix_layerless;
	utilization_result = result_path + RESULT_DIR+"utilization_"+suffix_layerless;
	memory_footprint_result = result_path + RESULT_DIR+"memory_footprint_"+suffix_layerless;
	avg_cycle_result = result_path + RESULT_DIR+"avg_cycle_"+suffix_layerless;
}

//--------------------------------------------------
// name: software_request_generator::init_compute_variables
// usage: initialize frequently used intermediate variables.
//--------------------------------------------------
void software_request_generator::init_compute_variables()
{
	filter_size = filter_width * filter_height * filter_depth;
	filter_size_one_channel = filter_width * filter_height;

	ifmap_size_one_channel = ifmap_width * ifmap_height;
	ifmap_iter_width = (int)ceil((ifmap_width - filter_width)/(double)stride)+1;//same as ofmap_width
	ifmap_iter_height = (int)ceil((ifmap_height - filter_height)/(double)stride)+1; //same as ofmap_height

	filter_fold = int(ceil((float)filter_num/systolic_width));
	ifmap_fold = int(ceil((float)ifmap_iter_width*ifmap_iter_height/systolic_height));
}

//--------------------------------------------------
// name: software_request_generator::is_full
// usage: return true if dram->sram filters are full. (Any of three: filter, ifmap, ofmap)
//--------------------------------------------------
bool software_request_generator::is_full()
{
	int add_num=0;
	for(set<uint64_t>::iterator itr = next_filter_set.begin(); itr != next_filter_set.end(); ++itr)
	{
		if(dram_filter_set.find(*itr)==dram_filter_set.end())
			add_num+=1;
	}
	if((dram_filter_set.size()+add_num)*cacheline_size > tile_filter_size)
		return true;

	add_num=0;
	for(set<uint64_t>::iterator itr = next_ifmap_set.begin(); itr != next_ifmap_set.end(); ++itr)
	{
		if(dram_ifmap_set.find(*itr)==dram_ifmap_set.end())
			add_num+=1;
	}
	if((dram_ifmap_set.size()+add_num)*cacheline_size > tile_ifmap_size)
		return true;

	add_num=0;
	for(set<uint64_t>::iterator itr = next_ofmap_set.begin(); itr != next_ofmap_set.end(); ++itr)
	{
		if(dram_ofmap_set.find(*itr)==dram_ofmap_set.end())
			add_num+=1;
	}
	if((dram_ofmap_set.size()+add_num)*cacheline_size > tile_ofmap_size)
		return true;

	return false;
}


//--------------------------------------------------
// name: software_request_generator::move_reset_dram
// usage: move (next_[filter|ifmap|ofmap]_set) to dram buffer and reset
//--------------------------------------------------
void software_request_generator::move_reset_dram()
{
	// move
	for(set<uint64_t>::iterator itr = next_filter_set.begin(); itr != next_filter_set.end(); ++itr)
		dram_filter_set.insert(*itr);
	for(set<uint64_t>::iterator itr = next_ifmap_set.begin(); itr != next_ifmap_set.end(); ++itr)
		dram_ifmap_set.insert(*itr);
	for(set<uint64_t>::iterator itr = next_ofmap_set.begin(); itr != next_ofmap_set.end(); ++itr)
		dram_ofmap_set.insert(*itr);
	//init: for memory deallocate swap to empty set is necessary
	set<uint64_t>().swap(next_filter_set);
	set<uint64_t>().swap(next_ifmap_set);
	set<uint64_t>().swap(next_ofmap_set);
}

//--------------------------------------------------
// name: software_request_generator::move_reset_sram
// usage: move (next_[filter|ifmap|ofmap]_vector) to sram buffer and reset
//--------------------------------------------------
void software_request_generator::move_reset_sram()
{
	// move
	for(vector<uint64_t>::iterator itr = next_filter_vector.begin(); itr != next_filter_vector.end(); ++itr){
		sram_filter_vector.push_back(*itr);
    }
	for(vector<uint64_t>::iterator itr = next_ifmap_vector.begin(); itr != next_ifmap_vector.end(); ++itr){
		sram_ifmap_vector.push_back(*itr);
    }
	for(vector<uint64_t>::iterator itr = next_ofmap_vector.begin(); itr != next_ofmap_vector.end(); ++itr){
		sram_ofmap_vector.push_back(*itr);
    }

	//init: for memory deallocate swap to empty vector is necessary
	vector<uint64_t>().swap(next_filter_vector);
	vector<uint64_t>().swap(next_ifmap_vector);
	vector<uint64_t>().swap(next_ofmap_vector);
}

//--------------------------------------------------
// name: software_request_generator::sram_file_write
// usage: for sram trace
//--------------------------------------------------
void software_request_generator::sram_file_write(vector<uint64_t> *data, string file_name, int line_len)
{
	if(data->size()==0)
		return ;
	
	string write_text("");

	vector<uint64_t>::iterator itr=data->begin();
	uint64_t sram_base_cycle=0;

	while(true)
	{
		if(SRAM_TRACE)
			write_text += to_string(sram_base_cycle) + ",";
		for(int i=0;i<line_len;i++)
		{
			if(*itr!=(uint64_t)-1 && SRAM_TRACE)
				write_text += to_string(*itr) + ",";
			else if(SRAM_TRACE)
				write_text += "x,";
			++itr;
		}
		sram_base_cycle++;
		if(itr==data->end())
			break;
		if(SRAM_TRACE)
			write_text += "\n";
	}

	if(SRAM_TRACE)
		write_output(result_path, file_name, write_text);
}
