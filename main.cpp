#include "accelerator.h"
#include "util.h"

//A wrapper for callback
MemoryController* memctrl;
void callback(uint64_t addr)
{
    memctrl->callback(addr);
}

int npu_setup(string archconfig, string netconfig, npu_accelerator* npu_layer_setup, npu_accelerator* npu, string result_path, vector<string> input_co_runners)
{
	npu_layer_setup->result_path = result_path;
	npu->result_path = result_path;

	read_config(npu_layer_setup, archconfig, netconfig, input_co_runners);
	npu_layer_setup->config_update(generate_intermediate_path(result_path, netconfig, npu->npu_idx));
	read_config(npu, archconfig, generate_intermediate_path(result_path, netconfig, npu->npu_idx), input_co_runners);
}


int main(int argc, char *argv[])
{
	// argv input
	// argv[1]: arch file list
	// argv[2]: net file list
	// argv[3]: dram file
	// argv[4]: npu memory & tlb file list
	// argv[5]: result path (default: current directory)
	// argv[6]: misc

	npu_group npugroup;
	if (argc > 5){
		npugroup.result_path = string(argv[5]) + "/";
	}else{
		npugroup.result_path = "";
	}

	//dramsim setup
	MemoryConfig memconfig;
	readMemConfig(&memconfig, argv[3]);
	memconfig.dramoutdir = npugroup.result_path + memconfig.dramoutdir;
	string cmd = "mkdir -p " + memconfig.dramoutdir;
	system(cmd.c_str());
	npugroup.memctrl = new MemoryController(memconfig);
	memctrl = npugroup.memctrl;
	(npugroup.memctrl)->dramSetup(memconfig.dramconfig, memconfig.dramoutdir, callback);
	//end dramsim setup

	int i;
	npugroup.num_npus = memconfig.npu_num;
	npugroup.npus = (npu_accelerator**)malloc(sizeof(npu_accelerator*)*(memconfig.npu_num));
	//MISC confiugration
	uint64_t* compute_cycle_inits = (uint64_t*)malloc(sizeof(uint64_t)*(memconfig.npu_num));
	int* iteration_inits = (int*)malloc(sizeof(int)*(memconfig.npu_num));
	int* walker_init = (int*)malloc(sizeof(int)*(memconfig.npu_num));
	int* walker_upper = (int*)malloc(sizeof(int)*(memconfig.npu_num));
	int* walker_lower = (int*)malloc(sizeof(int)*(memconfig.npu_num));
	if (argc > 6){
		FILE* f_misc = fopen(argv[6], "r");
		for (i=0; i<memconfig.npu_num; i++){
			fscanf(f_misc, "%lu %d %d %d %d", &compute_cycle_inits[i], &iteration_inits[i], &walker_init[i], &walker_upper[i], &walker_lower[i]);
		}
		fclose(f_misc);
	}else{
		for (i=0; i<memconfig.npu_num; i++){
			compute_cycle_inits[i] = 0;
			iteration_inits[i] = 1;
			walker_init[i] = 0;
			walker_upper[i] = 0;
			walker_lower[i] = 0;
		}
	}

	//Find NPU co-runner networks
	vector<string> co_runners;
	FILE* f_net_co_runner = fopen(argv[2], "r");
	for(i=0;i<memconfig.npu_num;i++){
		char netconfig[100];
		fscanf(f_net_co_runner, "%s", netconfig);
		co_runners.push_back(extract_name(netconfig));
	}
	fclose(f_net_co_runner);
	
	//NPU setup
	FILE* f_arch = fopen(argv[1], "r");
	FILE* f_net = fopen(argv[2], "r");
	FILE* f_npumem = fopen(argv[4], "r");
	for (i=0; i<memconfig.npu_num; i++){
		npugroup.npus[i] = new npu_accelerator(i, iteration_inits[i], compute_cycle_inits[i]);
		char archconfig[100];
		char netconfig[100];
		fscanf(f_arch, "%s", archconfig);
		fscanf(f_net, "%s", netconfig);
		npu_accelerator* npu_layer_setup = new npu_accelerator(i, iteration_inits[i], compute_cycle_inits[i]);
    		vector<string> tmp_co_runners=co_runners;
		tmp_co_runners[i]=string("ME");
		npu_setup(string(archconfig), string(netconfig), npu_layer_setup, npugroup.npus[i], npugroup.result_path, tmp_co_runners);
		delete npu_layer_setup;
		//NPU-side memory setup
		char npumem_file[100];
		fscanf(f_npumem, "%s", npumem_file);
		NPUMemConfig npumemcfg;
		readNPUMemConfig(&npumemcfg, npumem_file);
		(npugroup.memctrl)->npumemSetup(npumemcfg, i);
	}
	(npugroup.memctrl)->ptwSetup(walker_init, walker_upper, walker_lower);

	fclose(f_arch);
	fclose(f_net);
	free(compute_cycle_inits);
	free(iteration_inits);
	free(walker_init);
	free(walker_upper);
	free(walker_lower);
	
	//start NPU running after NPU setup

	npugroup.pre_run();
	npugroup.run();
	(npugroup.memctrl)->writeTLBLog();
	(npugroup.memctrl)->printStat();

	return 0;
}
