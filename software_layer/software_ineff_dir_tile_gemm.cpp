#include "../util.h"
#include "../accelerator.h"

//--------------------------------------------------
// name: software_request_generator::ineff_dir_gemm_tile_computation
//--------------------------------------------------
void software_request_generator::ineff_dir_gemm_tile_computation()
{

	if(!SRAM_TRACE)
		pre_local_cycle=0;

	int sum_tile_size, selected_tile_height, selected_tile_width, selected_filter_tile_width;

	selected_tile_height = ifmap_height;
	selected_tile_width = systolic_height;
	selected_filter_tile_width = systolic_width;

	for(int j=0;j<ceil((double)ifmap_width/selected_tile_width);j++)
	{
		for(int k=0;k<ceil((double)filter_width/selected_filter_tile_width);k++)
		{
			ineff_dir_gemm_tile_computation2(j*selected_tile_width, j*selected_tile_width*filter_width+k*selected_filter_tile_width, k*selected_filter_tile_width);
			if(is_full())
			{
				move_reset_dram();
				move_reset_sram();
				tile_output();
			}
		}
	}
	move_reset_dram();
	move_reset_sram();
	tile_output();

}

void software_request_generator::ineff_dir_gemm_tile_computation2(int ifmap_tile_start_addr, int filter_tile_start_addr, int ofmap_tile_start_addr)
{
	uint64_t local_ifmap_base_addr, local_filter_base_addr, local_ofmap_base_addr;
	local_ifmap_base_addr = ifmap_base_addr + ifmap_tile_start_addr;
	local_filter_base_addr = filter_base_addr + filter_tile_start_addr;
	local_ofmap_base_addr = ofmap_base_addr + ofmap_tile_start_addr;


	// Weight broadcasting
	for(int weight_y=MIN(systolic_height, filter_height)-1; weight_y>=0; weight_y--)
	{
		for(int weight_x=0; weight_x<systolic_width; weight_x++)
		{
			if(weight_x<MIN(systolic_width, filter_width))
			{
				next_filter_set.insert((local_filter_base_addr+(filter_width*weight_y+weight_x)*element_unit)/cacheline_size*cacheline_size);
				if(SRAM_TRACE)
					next_filter_vector.push_back(local_filter_base_addr+(filter_width*weight_y+weight_x)*element_unit);
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
	for(int t=0; t<ifmap_height+ifmap_width+MIN(systolic_width, filter_width)-2; t++)
	{
		//ifmap
		for(int y=0;y<systolic_height;y++)
		{
			if(t>=y && t<y+ifmap_height)
			{
				next_ifmap_set.insert((local_ifmap_base_addr+((t-y)*ifmap_width+y)*element_unit)/cacheline_size*cacheline_size);
				if(SRAM_TRACE)
					next_ifmap_vector.push_back(local_ifmap_base_addr+((t-y)*ifmap_width+y)*element_unit);
			}
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
				if(x+y<=t && t<x+y+ifmap_height && x<filter_width && y<filter_height)
				{
					uint64_t ofmap_idx = (t-(x+y))*filter_width+x;
					next_ofmap_set.insert((local_ofmap_base_addr+ofmap_idx*element_unit)/cacheline_size*cacheline_size);
					if(SRAM_TRACE)
						next_ofmap_vector.push_back((local_ofmap_base_addr+ofmap_idx*element_unit));
				}

				else if(SRAM_TRACE)
					next_ofmap_vector.push_back((uint64_t)-1);
			}
		}
		if(!SRAM_TRACE)
			pre_local_cycle+=unit_compute;
	}
}
