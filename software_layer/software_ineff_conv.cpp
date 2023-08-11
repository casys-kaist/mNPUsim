//--------------------------------------------------
// name: software_request_generator::conv_non_fold_parallel_computation
//--------------------------------------------------
// Dataflow: os-stationary

void conv_non_fold_parallel_computation()
{
	for(int local_ifmap_fold=0;local_ifmap_fold<ifmap_fold;local_ifmap_fold++)
	{
		int live_ifmap = MIN(ifmap_iter_width * ifmap_iter_height - systolic_height * local_ifmap_fold, systolic_height);
		for(int local_filter_fold=0;local_filter_fold<filter_fold;local_filter_fold++)
		{
			int live_filter = MIN(filter_num-systolic_width * local_filter_fold, systolic_width);
			int cycle_in_this_fold = filter_size + 2*(MIN(live_filter, live_ifmap) - 1);

			for(int local_cycle=0; local_cycle<cycle_in_this_fold; local_cycle++)
			{
				// find filter element
				for(int systolic_x=0;systolic_x<systolic_width;systolic_x++)
				{
					int filter_index = local_filter_fold * systolic_width + systolic_x;
					uint64_t systolic_x_filter_base = filter_base_addr + filter_index*filter_size*element_unit;
					int offset = local_cycle-systolic_x;
					if(filter_index<filter_num && offset>=0 && offset<filter_size)
					{
						next_filter_set.insert((systolic_x_filter_base + offset * element_unit) / cacheline_size * cacheline_size);
						if(SRAM_TRACE)
							next_filter_vector.push_back(systolic_x_filter_base + offset * element_unit);
					}
					else if(SRAM_TRACE)
						next_filter_vector.push_back((uint64_t)-1);
				}

				// find ifmap element
				for(int systolic_y=0;systolic_y<systolic_height;systolic_y++)
				{
					int ifmap_index = local_ifmap_fold * systolic_height + systolic_y;
					int systolic_y_ifmap_base_x = MIN(ifmap_index % ifmap_iter_width * stride, ifmap_width-filter_width);
					int systolic_y_ifmap_base_y = MIN(ifmap_index / ifmap_iter_width * stride, ifmap_height-filter_height);
					uint64_t systolic_y_ifmap_base = ifmap_base_addr + systolic_y_ifmap_base_y * ifmap_width * element_unit + systolic_y_ifmap_base_x * element_unit;
					int offset = local_cycle-systolic_y;

					if(ifmap_index < ifmap_iter_width * ifmap_iter_height && offset >= 0 && offset<filter_size)
					{
						int tuned_offset = 0;
						while(offset >= filter_width)
						{
							if(offset >= filter_size_one_channel)
							{
								tuned_offset += ifmap_size_one_channel;
								offset -= filter_size_one_channel;
							}
							else
							{
								tuned_offset += ifmap_width;
								offset -= filter_width;
							}
						}
						tuned_offset += offset;

						next_ifmap_set.insert((systolic_y_ifmap_base+tuned_offset * element_unit) / cacheline_size * cacheline_size);
						if(SRAM_TRACE)
							next_ifmap_vector.push_back(systolic_y_ifmap_base + tuned_offset * element_unit);
					}
					else if(SRAM_TRACE)
						next_ifmap_vector.push_back((uint64_t)-1);
				}

				// find ofmap element
				for(int systolic_x=0;systolic_x<systolic_width;systolic_x++)
				{
					for(int systolic_y=0;systolic_y<systolic_height;systolic_y++)
					{
						int filter_index = local_filter_fold*systolic_width+systolic_x;
						int ofmap_one_channel_size = ifmap_iter_width * ifmap_iter_height;
						int ifmap_index = local_ifmap_fold * systolic_height + systolic_y;
						uint64_t ofmap_addr = ofmap_base_addr + (filter_index * ofmap_one_channel_size + ifmap_index) * element_unit;
						int offset = local_cycle-systolic_x-systolic_y;
						if(offset >= 0 && offset < filter_size)
						{
							next_ofmap_set.insert(ofmap_addr / cacheline_size * cacheline_size);
							if(SRAM_TRACE)
								next_ofmap_vector.push_back(ofmap_addr);
						}
						else if(SRAM_TRACE)
							next_ofmap_vector.push_back((uint64_t)-1);
					}
				}

				if(is_full())
				{
					tile_output();
					cout << "tile(ineff_conv): " << ++full_num << endl;
				}

				//move: next_(filter|ifmap)_set -> dram_(filter|ifmap)_set + reset: next_(filter|ifmap)_set
				move_reset_dram();
				move_reset_sram();
			}

		}
	}

	tile_output();
	cout << "tile(ineff_conv): " << ++full_num << endl;
}
