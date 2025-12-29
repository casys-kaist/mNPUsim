#include "../util.h"
#include "../accelerator.h"

//--------------------------------------------------
// name: software_request_generator::other_computation
// usage: other operations.
//--------------------------------------------------
void software_request_generator::other_computation()
{
	if(!SRAM_TRACE)
		pre_local_cycle=0;

	for(int i=0; i<ifmap_width*ifmap_height/systolic_width; i++)
	{
		for(int j=0;j<systolic_width;j++)
		{
			uint64_t idx = i*systolic_width + j;
			if(idx < ifmap_width*ifmap_height)
			{
				next_ifmap_set.insert(ifmap_base_addr+idx*element_unit/cacheline_size*cacheline_size);
				if(SRAM_TRACE)
				{
					next_ifmap_vector.push_back(ifmap_base_addr+idx*element_unit);
					next_filter_vector.push_back((uint64_t)-1);
					for(int a=0;a<systolic_height;a++)
						next_ofmap_vector.push_back((uint64_t)-1);
				}
			}
		}
		for(int k=0; k<stride; k++)
		{
			if(SRAM_TRACE)
			{
				for(int j=0;j<systolic_width;j++)
				{
					next_ifmap_vector.push_back((uint64_t)-1);
					next_filter_vector.push_back((uint64_t)-1);
					for(int a=0;a<systolic_height;a++)
						next_ofmap_vector.push_back((uint64_t)-1);
				}
			}
			else
				pre_local_cycle+=1;
		}
		for(int j=0;j<systolic_width;j++)
		{
			uint64_t idx = i*systolic_width + j;
			if(idx < ifmap_width*ifmap_height)
			{
				next_ofmap_set.insert(ofmap_base_addr+idx*element_unit/cacheline_size*cacheline_size);
				if(SRAM_TRACE)
				{
					next_ifmap_vector.push_back(ifmap_base_addr+idx*element_unit);
					next_filter_vector.push_back((uint64_t)-1);
					for(int a=systolic_width-1;a<systolic_height;a++)
						next_ofmap_vector.push_back((uint64_t)-1);
				}
			}
		}
	}

	move_reset_dram();
	move_reset_sram();
	tile_output();
}
