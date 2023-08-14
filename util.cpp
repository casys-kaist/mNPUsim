#include "util.h"

//--------------------------------------------------
// name: extract_name
// usage: extract string from file name structure
// example
// input: ./network_config/conv_nets/alexnet.csv
// output: alexnet
//--------------------------------------------------
string extract_name(string file_name)
{
	int start_idx = 0;
	int end_idx = file_name.size();

	for(int i=0;i<file_name.size();i++)
	{
		if(file_name.at(i)=='/')
			start_idx = i+1;
		else if(file_name.at(i)=='.')
			end_idx = i;
	}

	return file_name.substr(start_idx, end_idx-start_idx);
}

//--------------------------------------------------
// name: generate_intermediate_path
// usage: generate intermediate path from file name structure
// example
// input: ./network_config/conv_nets/alexnet.csv
// output: ./intermediate_config/alexnet.csv
//--------------------------------------------------
string generate_intermediate_path(string result_path, string file_name, int npu_idx)
{
	int start_idx = 0;
	int end_idx = file_name.size();

	for(int i=0;i<file_name.size();i++)
	{
		if(file_name.at(i)=='/')
			start_idx = i+1;
		else if(file_name.at(i)=='.')
			end_idx = i;
	}

	return result_path + INTERMEDIATE_CONFIG_DIR +file_name.substr(start_idx, end_idx-start_idx)+to_string(npu_idx)+string(".csv");
}

//--------------------------------------------------
// name: clear_output
// usage: erase all output files (trace files + summary files + time files)
// example
// input: none
// output: erase trace files + summary files + time files
//--------------------------------------------------
void clear_output(string result_path)
{
	string OUTPUT_DIR = result_path + INTERMEDIATE_CONFIG_DIR + " "
		+ result_path + INTERMEDIATE_DIR + " "
		+ result_path + TIME_DIR + " "
		+ result_path + CLOCK_DIR + " "
		+ result_path + RESULT_DIR;
	string cmd = "rm -rf " + OUTPUT_DIR;
	system(cmd.c_str());
}

//--------------------------------------------------
// name: write_output
// usage: write string to file (with append option)
// example
// input: ./trace/sram_read_ifmap_alexnet_Conv1_config_tiny_128.txt, hi
// output: 
//--------------------------------------------------
void write_output(string result_path, string file_name, string write_str)
{
	//make dir for output
    string OUTPUT_DIR = result_path + INTERMEDIATE_CONFIG_DIR + " "
        + result_path + INTERMEDIATE_DIR + " "
        + result_path + TIME_DIR + " "
        + result_path + CLOCK_DIR + " "
        + result_path + RESULT_DIR;
	string cmd = "mkdir -p " + OUTPUT_DIR;
	system(cmd.c_str());

	ofstream file;
	file.open(file_name.c_str(), ios::app);
	file << write_str << endl;
	file.close();
}

//--------------------------------------------------
// name: read_config
// usage: setup software request generator configuration from config files
//--------------------------------------------------
void read_config(npu_accelerator *ng, string arch_file, string network_file, vector<string> input_co_runners)
{
	ng->co_runners=input_co_runners;
	for(int i=0;i<ng->co_runners.size();i++)
	{
		if(ng->co_runners[i]==string("ME"))
		{
			ng->my_pos=i;
			ng->co_runners[i]=extract_name(network_file);
			break;
		}
	}
	
	read_arch_config(ng, arch_file);
	read_network_config(ng, network_file);

	//print parsing result
	cout << "parsing result" << endl;
	cout << "1. arch_parsing" << endl;
	cout << "arch_name: " << ng->arch_name << endl;
	cout << "systolic_array(width x height): " << ng->systolic_width << " x " << ng->systolic_height << endl;
	cout << "sram_size(ifmap, filter, ofmap): " << ng->sram_ifmap_size << ", " << ng->sram_filter_size << ", " << ng->sram_ofmap_size << " Bytes" << endl;
	cout << "tile_size(ifmap, filter, ofmap): " << ng->tile_ifmap_size << ", " << ng->tile_filter_size << ", " << ng->tile_ofmap_size << " Bytes" << endl;
	cout << "dataflow: " << ng->dataflow_type << endl;
	cout << "element_unit: " << ng->element_unit << " Bytes" << endl;
	cout << "is_special_function_unit: " << boolalpha << ng->is_special_function_unit << boolalpha << endl;
	cout << "2. network_parsing" << endl;
	cout << "model_name, layer_name, ifmap(width x height x channel), filter(width x height x channel x # of filter), stride, layer_type, ifmap_base_addr, filter_base_addr, ofmap_base_addr" << endl;
	for(int i=0;i<ng->network_info.size();i++)
	{
		cout << get<0>(ng->network_info[i]) << ", " << get<1>(ng->network_info[i]) << ", " << get<3>(ng->network_info[i]) << " x " << get<2>(ng->network_info[i]) << " x " << get<6>(ng->network_info[i]) << ", " << get<5>(ng->network_info[i]) << " x " << get<4>(ng->network_info[i]) << " x " << get<6>(ng->network_info[i]) << " x " << get<7>(ng->network_info[i]) << ", " << get<8>(ng->network_info[i]) << ", " << get<9>(ng->network_info[i]) << ", " << get<10>(ng->network_info[i]) << ", " << get<11>(ng->network_info[i]) << ", " << get<12>(ng->network_info[i]) << endl;
	}
	cout << "---------------------------------------------------------------------------" << endl;
}
