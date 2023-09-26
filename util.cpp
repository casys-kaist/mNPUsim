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
		+ result_path + RESULT_DIR + " "
		+ result_path + DRAMREQ_NPU_DIR;
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
	string OUTPUT_DIR;
	if(DRAMREQ_NPU_TRACE)
		OUTPUT_DIR = result_path + INTERMEDIATE_CONFIG_DIR + " " + result_path + INTERMEDIATE_DIR + " " + result_path + RESULT_DIR + " " + result_path + DRAMREQ_NPU_DIR;
	else
		OUTPUT_DIR = result_path + INTERMEDIATE_CONFIG_DIR + " " + result_path + INTERMEDIATE_DIR + " " + result_path + RESULT_DIR;

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
	cout << "unit_compute: " << ng->unit_compute << endl;
	cout << "2. network_parsing" << endl;
	cout << "model_name, layer_name, ifmap(width x height x channel), filter(width x height x channel x # of filter), stride, layer_type, ifmap_base_addr, filter_base_addr, ofmap_base_addr" << endl;
	for(int i=0;i<ng->network_info.size();i++)
	{
		cout << get<0>(ng->network_info[i]) << ", " << get<1>(ng->network_info[i]) << ", " << get<3>(ng->network_info[i]) << " x " << get<2>(ng->network_info[i]) << " x " << get<6>(ng->network_info[i]) << ", " << get<5>(ng->network_info[i]) << " x " << get<4>(ng->network_info[i]) << " x " << get<6>(ng->network_info[i]) << " x " << get<7>(ng->network_info[i]) << ", " << get<8>(ng->network_info[i]) << ", " << get<9>(ng->network_info[i]) << ", " << get<10>(ng->network_info[i]) << ", " << get<11>(ng->network_info[i]) << ", " << get<12>(ng->network_info[i]) << endl;
	}
	cout << "---------------------------------------------------------------------------" << endl;
}

//--------------------------------------------------
// name: read_arch_config
// usage: setup arch_configuration from arch_config files
//--------------------------------------------------
void read_arch_config(npu_accelerator *ng, string file_name)
{
	ifstream file;
	string str_buf;
	int index=0;

	file.open(file_name.c_str());

	while(!file.eof())
	{
		getline(file, str_buf, ',');
		str_buf.erase(remove(str_buf.begin(), str_buf.end(), ' '), str_buf.end());
		str_buf.erase(remove(str_buf.begin(), str_buf.end(), '\n'), str_buf.end());
		switch(index)
		{
		case 1:
			ng->arch_name = str_buf;
			break;
		case 3:
			ng->systolic_height = stoi(str_buf);
			break;
		case 5:
			ng->systolic_width = stoi(str_buf);
			break;
		case 7:
			ng->sram_ifmap_size = stoi(str_buf);
			break;
		case 9:
			ng->sram_filter_size = stoi(str_buf);
			break;
		case 11:
			ng->sram_ofmap_size = stoi(str_buf);
			break;
		case 13:
			ng->dataflow_type = str_buf;
			break;
		case 15:
			ng->element_unit = stoi(str_buf); // Bytes
			break;
		case 17:
			ng->cacheline_size = stoi(str_buf);
			break;
		case 19:
			ng->tile_ifmap_size = stoull(str_buf);
			break;
		case 21:
			ng->tile_filter_size = stoull(str_buf);
			break;
		case 23:
			ng->tile_ofmap_size = stoull(str_buf);
			break;
		case 25:
			ng->unit_compute = stoull(str_buf);
			break;
		}
		index++;
	}

	file.close();
}


//--------------------------------------------------
// name: read_network_config
// usage: setup network_configuration from network_config files
//--------------------------------------------------
void read_network_config(npu_accelerator *ng, string file_name)
{
	ifstream file;
	string str_buf;
	int index=0;

	file.open(file_name.c_str());

	getline(file, str_buf); //skip first line

	while(!file.eof())
	{
		getline(file, str_buf, ',');
		str_buf.erase(remove(str_buf.begin(), str_buf.end(), ' '), str_buf.end());
		str_buf.erase(remove(str_buf.begin(), str_buf.end(), '\n'), str_buf.end());
		string input_layer_name;
		int input_ifmap_height, input_ifmap_width, input_filter_height, input_filter_width, input_filter_depth, input_filter_num, input_stride;
		string input_layer_type;
		uint64_t input_ifmap_base_addr, input_filter_base_addr, input_ofmap_base_addr;
		
		if(file.eof())
			break;
		input_layer_name = str_buf;
		getline(file, str_buf, ',');
		input_ifmap_height=stoi(str_buf);
		getline(file, str_buf, ',');
		input_ifmap_width=stoi(str_buf);
		getline(file, str_buf, ',');
		input_filter_height=stoi(str_buf);
		getline(file, str_buf, ',');
		input_filter_width=stoi(str_buf);
		getline(file, str_buf, ',');
		input_filter_depth=stoi(str_buf);
		getline(file, str_buf, ',');
		input_filter_num=stoi(str_buf);
		getline(file, str_buf, ',');
		input_stride=stoi(str_buf);

		getline(file, str_buf, ',');
		input_layer_type=str_buf;

		getline(file, str_buf, ',');
		input_ifmap_base_addr=stoull(str_buf);
		getline(file, str_buf, ',');
		input_filter_base_addr=stoull(str_buf);
		getline(file, str_buf, ',');
		input_ofmap_base_addr=stoull(str_buf);

		ng->network_info.push_back(make_tuple(extract_name(file_name), input_layer_name, input_ifmap_height, input_ifmap_width, input_filter_height, input_filter_width, input_filter_depth, input_filter_num, input_stride, input_layer_type, input_ifmap_base_addr, input_filter_base_addr, input_ofmap_base_addr));
	}

	file.close();
}

//--------------------------------------------------
// name: random_value
// usage: generate random uint64_t value between max/min-input
//--------------------------------------------------
uint64_t random_value(uint64_t max_input, uint64_t min_input)
{
	random_device rd;
	mt19937_64 gen(rd());
	uniform_int_distribution<uint64_t> dis(min_input, max_input);

	uint64_t ret_value = dis(gen);

	return ret_value;
}
