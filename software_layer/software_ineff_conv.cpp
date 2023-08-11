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
			}
		}
	}
}
