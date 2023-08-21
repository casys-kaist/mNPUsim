#include "util.h"
#include "accelerator.h"

//--------------------------------------------------
// name: software_request_generator::config_update
// usage: config_update
//--------------------------------------------------
void software_request_generator::config_update(string intermediate_config_name)
{
	write_config_string="Layer name, IFMAP Height, IFMAP Width, Filter Height, Filter Width, Channels, Num Filter, Strides, Type, IFMAP Base Addr(Absolute), Filter Base Addr(Absolute), Ofmap Base Addr(Absolute),\n";
	
	for(int i=0;i<network_info.size();i++)
	{
		cout << "processing: " << i+1 << " / " << network_info.size() << endl;
		layer_type = get<9>(network_info[i]);
		if(DEBUG)
			cout << "layer_type: " << layer_type << endl;

		//set variables
		if(layer_type==string("Im2col_conv"))
			set_variable_config_im2col_conv(i);
		else
			set_variable_config(i);

		if(layer_type==string("Gemm") || layer_type==string("Im2col_conv"))
			gemm_translation();
		else
		{
			init_compute_variables();

			write_config_string += layer_name+string(",")+to_string(ifmap_height)+string(",")+to_string(ifmap_width)+string(",")+to_string(filter_height)+string(",")+to_string(filter_width)+string(",")+to_string(filter_depth)+string(",")+to_string(filter_num)+string(",")+to_string(stride)+string(",")+layer_type+string(",")+to_string(ifmap_base_addr)+string(",")+to_string(filter_base_addr)+string(",")+to_string(ofmap_base_addr)+string(",\n");

			address_update();
		}
	}
	cout << "intermediate_config_name: " << intermediate_config_name << endl;
	write_output(result_path, intermediate_config_name, write_config_string);
}

//--------------------------------------------------
// name: software_request_generator::tile_output
// usage: write tile_output
//--------------------------------------------------
void software_request_generator::tile_output()
{
	string dram_string = "";
	for(set<uint64_t>::iterator itr = dram_filter_set.begin(); itr != dram_filter_set.end(); ++itr)
	{
		dram_string += to_string(*itr) + ",";
		if((*itr) > max_addr)
			max_addr = (*itr);
		if((*itr) < min_addr)
			min_addr = (*itr);
	}
	write_output(result_path, dram_read_intermediate,dram_string);

	dram_string = "";
	for(set<uint64_t>::iterator itr = dram_ifmap_set.begin(); itr != dram_ifmap_set.end(); ++itr)
	{
		dram_string += to_string(*itr) + ",";
		if((*itr) > max_addr)
			max_addr = (*itr);
		if((*itr) < min_addr)
			min_addr = (*itr);
	}
	write_output(result_path, dram_read_intermediate,dram_string);

	dram_string = "";
	for(set<uint64_t>::iterator itr = dram_ofmap_set.begin(); itr != dram_ofmap_set.end(); ++itr)
	{
		dram_string += to_string(*itr) + ",";
		if((*itr) > max_addr)
			max_addr = (*itr);
		if((*itr) < min_addr)
			min_addr = (*itr);
	}
	write_output(result_path, dram_write_intermediate,dram_string);

	set<uint64_t>().swap(dram_filter_set);
	set<uint64_t>().swap(dram_ifmap_set);
	set<uint64_t>().swap(dram_ofmap_set);

	if(SRAM_TRACE)
	{
		sram_file_write(&sram_filter_vector, sram_read_filter_intermediate, systolic_width);
		sram_file_write(&sram_ifmap_vector, sram_read_ifmap_intermediate, systolic_height);
		sram_file_write(&sram_ofmap_vector, sram_write_ofmap_intermediate, systolic_width*systolic_height);

		vector<uint64_t>().swap(sram_filter_vector);
		vector<uint64_t>().swap(sram_ifmap_vector);
		vector<uint64_t>().swap(sram_ofmap_vector);
	}
	else
	{
		write_output(result_path, sram_read_filter_intermediate, to_string(0)+",");
		write_output(result_path, sram_read_ifmap_intermediate, to_string(0)+",");
		write_output(result_path, sram_write_ofmap_intermediate, to_string(0)+",");

		if(pre_local_cycle!=1)
		{
			write_output(result_path, sram_read_filter_intermediate, to_string(pre_local_cycle-1)+",");
			write_output(result_path, sram_read_ifmap_intermediate, to_string(pre_local_cycle-1)+",");
			write_output(result_path, sram_write_ofmap_intermediate, to_string(pre_local_cycle-1)+",");
		}

		pre_local_cycle=0;
	}
}
