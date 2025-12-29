#include "util.h"
#include "accelerator.h"

//--------------------------------------------------
// name: npu_group::pre_run
// usage: main function for software_request_generator
//--------------------------------------------------
void npu_group::pre_run()
{
	for(int i=0;i<num_npus;i++)
	{
		printf("Pre-run of NPU %d\n", i);
		npus[i]->computation();
	}
}

//--------------------------------------------------
// name: npu_group::run
// usage: main function for npu_group
//--------------------------------------------------

void npu_group::run()
{
	memory_cycle = 0;//DRAM clock

	printf("Run with %d NPUs\n", num_npus);
	//initial setup
	int i;
	int* npu_layer = (int*)malloc(num_npus*sizeof(int));
	bool* is_terminated = (bool*)malloc(num_npus*sizeof(bool));
	bool* is_iter_terminated = (bool*)malloc(num_npus*sizeof(bool));
	for (i=0; i<num_npus; i++){
		npu_layer[i] = 0;
		npus[i]->is_first = true;
		npus[i]->tile_full = false;
		npus[i]->is_setup = false;
		npus[i]->is_returned = false;
		npus[i]->is_begin = false;
		is_terminated[i] = false;
		is_iter_terminated[i] = false;
	}
	bool terminated_all = false;

	set<int> saved_idx;
	multimap<uint64_t, NPU_packet> cycle_idx_map;

	//body (Note: currently, no support of SRAM_TRACE)
	while (!terminated_all){
		for (i=0; i<num_npus; i++){
			if (npus[i]->is_first && (!npus[i]->is_setup) && (!is_terminated[i])){//begin of the layer
				npus[i]->set_variable(npu_layer[i]);
				npus[i]->init_file_io();
				(npus[i]->file).open(npus[i]->sram_read_ifmap_intermediate.c_str());
				(npus[i]->file_dread).open(npus[i]->dram_read_intermediate.c_str());
				(npus[i]->file_dwrite).open(npus[i]->dram_write_intermediate.c_str());
				(npus[i]->input_local_cycle) = 0;
				npus[i]->is_setup = true;
				npus[i]->is_returned = false;
				
				if (memory_cycle >= memctrl->npu2dramTick(npus[i]->compute_cycle, i)){
					printf("running NPU-%d: %d / %ld (starting at %ld cycle)\n", i, npu_layer[i]+1, (npus[i]->network_info).size(), npus[i]->compute_cycle);
					npus[i]->is_begin = true;
					mem_op(&(npus[i]->file_dread), 0, i);
				}else{
					printf("NPU-%d should start execution at %ld\n", i, npus[i]->compute_cycle);
					keep_queue.push_back(i);
				}
			}
		}

		//cycle computation
		int idx = mem_stall();
		if (idx != -1 && idx != num_npus){
			printf("mem_stall at tick %ld (NPU-%d)\n", memory_cycle, idx);
			if (!(npus[idx]->file.is_open()) && npus[idx]->is_setup){//Last stalled request for previous tile arrived
				mem_sync(idx);
				if (is_terminated[idx] && (saved_idx.find(idx) == saved_idx.end())){
					printf("End of NPU-%d execution at tick %ld (iteration %d out of %d)\n", idx, npus[idx]->compute_cycle, npus[idx]->iter_cnt, npus[idx]->iteration);
					write_output(npus[idx]->result_path, npus[idx]->execution_cycle_result, to_string(npus[idx]->compute_cycle));
					write_output(npus[idx]->result_path, npus[idx]->avg_cycle_result, to_string(npus[idx]->compute_cycle/npus[idx]->iter_cnt));
					write_output(npus[idx]->result_path, npus[idx]->memory_footprint_result, to_string(npus[idx]->max_addr)+"-"+to_string(npus[idx]->min_addr)+"="+to_string(npus[idx]->max_addr-npus[idx]->min_addr));
					if (npus[idx]->iteration == npus[idx]->iter_cnt || (npus[idx]->iteration<0 && min_finish_iter()>=npus[idx]->iteration*(-1))){
						saved_idx.insert(idx);
						mem_finished(idx);
						is_iter_terminated[idx] = true;
					}else{
						//setup for re-execution
						printf("Setup for re-execution\n");
						mem_flush(idx);
						npu_layer[idx] = 0;
						npus[idx]->is_first = true;
						npus[idx]->tile_full = false;
						npus[idx]->is_setup = false;
						npus[idx]->is_returned = false;
						is_terminated[idx] = false;
					}
				}else{
					printf("End of NPU-%d layer(or tile for out-im2col conv) %d execution at tick %ld (iteration %d out of %d)\n", idx, npu_layer[idx], npus[idx]->compute_cycle, npus[idx]->iter_cnt, npus[idx]->iteration);
					write_output(npus[idx]->result_path, npus[idx]->execution_cycle_result, to_string(npus[idx]->compute_cycle));
				}
				npus[idx]->is_setup = false;
			}
		}

		for (i=0; i<num_npus; i++){
			if (is_terminated[i] || (!(npus[i]->is_begin))){
				continue;
			}
//			if(DEBUG)
//				printf("receiving stall request from NPU-%d\n", i);
			NPU_packet pkt;
			pkt.npu_idx = i;
			if (idx == i){
				if (!(npus[i]->is_returned)){
					mem_sync(i);
				}
				npus[i]->is_returned = true;
			}
			if (npus[i]->is_returned){
				pkt.eof = npus[i]->run(&(pkt.data_file_ptr), &(pkt.op_type), &(pkt.need_sync));
				if (pkt.data_file_ptr){
					cycle_idx_map.insert(make_pair(npu2dramTick(npus[i]->compute_cycle, i), pkt));
				}
			}
		}
		//sort
		map<uint64_t, NPU_packet>::iterator iter;
		for (iter = cycle_idx_map.begin(); iter != cycle_idx_map.end();){
			NPU_packet poppkt = iter->second;
			if (idx != -1 && iter->first > memory_cycle){
				break;
			}
			idx = poppkt.npu_idx;
			if (poppkt.need_sync){
				mem_sync(idx);
			}
			mem_op(poppkt.data_file_ptr, poppkt.op_type, idx);
			if (poppkt.eof){//Force last writeback
				(npus[idx]->file).close();
				(npus[idx]->file_dread).close();
				(npus[idx]->file_dwrite).close();
				npu_layer[idx]++;
				npus[idx]->is_first = true;
				if ((npus[idx]->network_info).size() == npu_layer[idx]){
					npus[idx]->iter_cnt++;
					is_terminated[idx] = true;
				}
			}
			iter = cycle_idx_map.erase(iter);
		}

		terminated_all = true;
		for (i=0; i<num_npus; i++){
	  		terminated_all &= (is_iter_terminated[i]);
		}
	}
	//Final cycle computation
	int final_idx = ~0;
	while (saved_idx.size() < num_npus)
	{
		while (final_idx == ~0 || final_idx == num_npus){
			final_idx = mem_stall();
		}
		mem_sync(final_idx);
		printf("End of NPU-%d execution at tick %ld (Total iteration: %d)\n", final_idx, npus[final_idx]->compute_cycle, npus[final_idx]->iter_cnt);
		write_output(npus[final_idx]->result_path, npus[final_idx]->execution_cycle_result, to_string(npus[final_idx]->compute_cycle));
		write_output(npus[final_idx]->result_path, npus[final_idx]->avg_cycle_result, to_string(npus[final_idx]->compute_cycle/npus[final_idx]->iter_cnt));
		write_output(npus[final_idx]->result_path, npus[final_idx]->memory_footprint_result, to_string(npus[final_idx]->max_addr)+"-"+to_string(npus[final_idx]->min_addr)+"="+to_string(npus[final_idx]->max_addr-npus[final_idx]->min_addr));
		saved_idx.insert(final_idx);
		mem_finished(final_idx);
	}
}


//--------------------------------------------------
// name: npu_group::dram_load_write
// usage: insert buffer element [map; (addr, size)]
// -------------------------------------------------
void npu_group::dram_load_write(ifstream *file_stream, map<uint64_t, int> *mem_buffer, int npu_idx)
{
	string str_buf_mem;
	getline(*file_stream, str_buf_mem, '\n');
	str_buf_mem.erase(remove(str_buf_mem.begin(), str_buf_mem.end(), ' '), str_buf_mem.end());
	str_buf_mem.erase(remove(str_buf_mem.begin(), str_buf_mem.end(), '\n'), str_buf_mem.end());

	if(!file_stream->eof())
	{
		int base = 0;
		for(int i=0;i<str_buf_mem.length();i++)
		{
			if(str_buf_mem.substr(i, 1)==string(","))
			{
				mem_buffer->insert({stoull(str_buf_mem.substr(base, i-base)), memctrl->getWordSize(npu_idx)});
				base = i+1;
			}
		}
	}
	else
	{
		cout << "ERROR" << endl;
		exit(1);
	}
}

//--------------------------------------------------
// name: npu_group::dram_buffer_string
// usage: return dram buffer string
// -------------------------------------------------
string npu_group::dram_buffer_string(map<uint64_t, int> *mem_buffer, uint64_t mem_cycle)
{
	string dram_string="";
	dram_string += to_string(mem_cycle)+",";

	for(map<uint64_t,int>::iterator itr=mem_buffer->begin(); itr!=mem_buffer->end(); ++itr)
		dram_string += to_string(itr->first) + ",";
	return dram_string;
}

//--------------------------------------------------
// name: npu_group::mem_op
// usage: memory read-write function of which sends request
// parameters: type[0: read ifmap + filter, 1: write ofmap once, 2: write ofmap twice (both filling and draining buffer for last layer)]
// -------------------------------------------------
void npu_group::mem_op(ifstream *data_file_ptr, int type, int npu_idx)
{
	map<uint64_t, int> dram_data_buffer;
	if(DEBUG)
		printf("mem_op type %d at NPU %d, tick %ld\n", type, npu_idx, memory_cycle);

	switch(type)
	{
	case 0:
		// read tile (ifmap + filter)
		dram_load_write(data_file_ptr, &dram_data_buffer, npu_idx);
		dram_load_write(data_file_ptr, &dram_data_buffer, npu_idx);
		if(DRAMREQ_NPU_TRACE)
			write_output(npus[npu_idx]->result_path, npus[npu_idx]->dramreq_read_npu, dram_buffer_string(&dram_data_buffer, memory_cycle));
		break;
	case 1:
		// write tile (ofmap)
		dram_load_write(data_file_ptr, &dram_data_buffer, npu_idx);
		if(DRAMREQ_NPU_TRACE)
			write_output(npus[npu_idx]->result_path, npus[npu_idx]->dramreq_write_npu, dram_buffer_string(&dram_data_buffer, memory_cycle));
	}
	//DRAMsim Routine
	if(DEBUG)
		printf("Send DRAM Request for NPU-%d at tick %ld\n",npu_idx, memory_cycle);
	memctrl->dramRequest(&dram_data_buffer, (type > 0), npu_idx);

	map<uint64_t, int>().swap(dram_data_buffer);
}


//--------------------------------------------------
// name: npu_group::mem_stall
// usage: receives stalled request
//--------------------------------------------------

int npu_group::mem_stall()
{
	int i;
	while (!(keep_queue.empty())){
		int unstarted_idx = keep_queue.front();
		while (memctrl->getTick(unstarted_idx) < npus[unstarted_idx]->compute_cycle){
			memctrl->atomicTick();
			for (i=0; i<num_npus; i++){
				if (memctrl->checkNPURequest(i)){//sudden termination
					break;
				}
			}
			//i is return idx here
			if (i != num_npus){
				memory_cycle = memctrl->getTick(~0);
				return i;
			}
		}
		printf("[mem_stall]running NPU-%d: 1 / %ld (starting at %ld cycle)\n", unstarted_idx, (npus[unstarted_idx]->network_info).size(), memctrl->getTick(unstarted_idx));
		npus[unstarted_idx]->is_begin = true;
		mem_op(&(npus[unstarted_idx]->file_dread), 0, unstarted_idx);
		keep_queue.pop_front();
	}
	//memctrl->atomic();
	int return_idx = (int)(memctrl->popStallRequest());

	if (return_idx == ~0 || return_idx == num_npus){//No request or keep stalling
		memory_cycle = memctrl->getTick(~0);
		return return_idx;//escape immediatly
	}
	
	memory_cycle = memctrl->getTick(~0);
	printf("Stalled request returned at tick %ld: NPU-%d (compute cycle %ld)\n", memory_cycle, return_idx, npus[return_idx]->compute_cycle);
	return return_idx;
}


//--------------------------------------------------
// name: npu_group::mem_finished
// usage: mark end of request submission for NPU-npu_idx
//--------------------------------------------------

void npu_group::mem_finished(int npu_idx)
{
	memctrl->markFinished(npu_idx);
}


//--------------------------------------------------
// name: npu_group::mem_sync
// usage: synchronize memory_cycle and npu computation cycle
//--------------------------------------------------

void npu_group::mem_sync(int npu_idx)
{
	if(DEBUG)
		printf("memory_cycle %ld / compute_cycle %ld\n", memory_cycle, npus[npu_idx]->compute_cycle);
	npus[npu_idx]->compute_cycle = MAX(npus[npu_idx]->compute_cycle, memctrl->getTick(npu_idx));
	memory_cycle = memctrl->npu2dramTick(npus[npu_idx]->compute_cycle, npu_idx);
	while (memctrl->getTick(~0) < memory_cycle){
		memctrl->atomicTick();
	}
	if(DEBUG)
		printf("mem_sync\nTick after synchronization: %ld\n", memory_cycle);
}


//--------------------------------------------------
// name: npu_group::mem_flush
// usage: tlb flush
//--------------------------------------------------

void npu_group::mem_flush(int npu_idx)
{
	memctrl->tlbFlush(npu_idx);
	mem_sync(npu_idx);
}

//--------------------------------------------------
// name: npu_group::min_finish_iter
// usage: for min of finished npus
// -------------------------------------------------
int npu_group::min_finish_iter()
{
	int min_iter=npus[0]->iter_cnt;
	for(int i=0;i<num_npus;i++)
	{
		if(npus[i]->iter_cnt<min_iter)
			min_iter=npus[i]->iter_cnt;
	}
	return min_iter;
}
