#include "../util.h"
#include "../accelerator.h"

//--------------------------------------------------
// name: software_request_generator::dir_gemm_tile_computation
// usage: direct gemm operations with continuous systolic array input.
// This function is only well-executed after systolic array-level tiling (i.e. tile dimension is smaller than systolic array).
//--------------------------------------------------
void software_request_generator::dir_gemm_tile_computation()
{
	if(!SRAM_TRACE)
		pre_local_cycle=0;

	// Weight broadcasting
	for(int weight_y=MIN(systolic_height, filter_height)-1; weight_y>=0; weight_y--)
	{
		for(int weight_x=0; weight_x<systolic_width; weight_x++)
		{
			if(weight_x<MIN(systolic_width, filter_width))
			{
				next_filter_set.insert((filter_base_addr+(filter_width*weight_y+weight_x)*element_unit)/cacheline_size*cacheline_size);
				if(SRAM_TRACE)
					next_filter_vector.push_back(filter_base_addr+(filter_width*weight_y+weight_x)*element_unit);
			}
			else if(SRAM_TRACE)
				next_filter_vector.push_back((uint64_t)-1);
		}

		if(SRAM_TRACE)
		{
			for(int i=0;i<systolic_height;i++)
			{
				next_ifmap_vector.push_back((uint64_t)-1);
				for(int j=0;j<systolic_width;j++)
					next_ofmap_vector.push_back((uint64_t)-1);
			}
		}
		else
			pre_local_cycle+=unit_compute;
	}

	// Ifmap/Ofmap flow
	for(int t=0; t<ifmap_height+MIN(systolic_width, ifmap_width); t++)
	{
		//ifmap
		for(int y=0;y<systolic_height;y++)
		{
			next_ifmap_set.insert((ifmap_base_addr+(t*ifmap_width+y)*element_unit)/cacheline_size*cacheline_size);
			if(SRAM_TRACE)
				next_ifmap_vector.push_back(ifmap_base_addr+(t*ifmap_width+y)*element_unit);
			else if(SRAM_TRACE)
				next_ifmap_vector.push_back((uint64_t)-1);
		}
		
		if(SRAM_TRACE)
		{
			for(int i=0;i<systolic_width;i++)
				next_filter_vector.push_back((uint64_t)-1);
		}

		//ofmap
		for(int y=0;y<systolic_height;y++)
		{
			for(int x=0;x<systolic_width;x++)
			{
				if(x<=t && t<x+ifmap_height && x<filter_width && y<filter_height)
				{
					uint64_t ofmap_idx = (t-x)*filter_width+x;
					next_ofmap_set.insert((ofmap_base_addr+ofmap_idx*element_unit)/cacheline_size*cacheline_size);
					if(SRAM_TRACE)
						next_ofmap_vector.push_back(ofmap_base_addr+ofmap_idx*element_unit);
				}

				else if(SRAM_TRACE)
					next_ofmap_vector.push_back((uint64_t)-1);
			}
		}
		if(!SRAM_TRACE)
			pre_local_cycle+=unit_compute;

		if(is_full())
		{
			tile_output();
			full_num++;
		}

		move_reset_dram();
		move_reset_sram();
	}

	tile_output();
}
