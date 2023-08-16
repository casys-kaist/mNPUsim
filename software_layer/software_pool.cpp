#include "../util.h"
#include "../accelerator.h"

//--------------------------------------------------
// name: software_request_generator::pool_computation()
// usage: pooling operations
//--------------------------------------------------
void software_request_generator::pool_computation()
{
	for(int i=0;i<filter_depth;i++)
	{
		for(int j=0;j<ifmap_iter_width;j++)
		{
			for(int k=0;k<ifmap_iter_height;k++)
			{
				uint64_t ifmap_base_idx = i * ifmap_size_one_channel + MIN(j * stride, ifmap_width-filter_width) + MIN(k * stride * ifmap_width, ifmap_height-filter_height);
				uint64_t ifmap_start_addr = ifmap_base_addr + ifmap_base_idx * element_unit;

				for(int filter_loop_x=0;filter_loop_x<filter_width;filter_loop_x++)
				{
					for(int filter_loop_y=0;filter_loop_y<filter_height;filter_loop_y++)
					{
						uint64_t ifmap_addr = ifmap_start_addr + (filter_loop_x+filter_loop_y*filter_width)*element_unit;
						next_ifmap_set.insert(ifmap_addr/cacheline_size*cacheline_size);

						if(SRAM_TRACE)
							next_ifmap_vector.push_back(ifmap_addr);

					}
				}

				if(is_full())
				{
					tile_output();
					cout << "tile(pooling): " << ++full_num << endl;
				}

				move_reset_dram();
				move_reset_sram();
			}
		}
	}

	tile_output();
	cout << "tile(pooling): " << ++full_num << endl;
}
