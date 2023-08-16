#include "../util.h"
#include "../accelerator.h"

//--------------------------------------------------
// name: software_request_generator::gemm_computation
// usage: gemm operations with continuous systolic array input.
// we support SPM-level tiling in gemm_computation. It supported in gemm-translated phase.
// gemm is indirectly computed into conv-layer. It converted to conv (similar to MAESTRO simulator)
// [M, K] x [K, N] gemm -> [X, Y, C] ifmap, [R, S ,C, N] filter conv: C = K (in GEMM), Y = M (in GEMM), K = N (in GEMM), X = R = S = 1
//--------------------------------------------------
void software_request_generator::gemm_computation()
{
	int sum_tile_size, selected_tile_height, selected_tile_width, selected_filter_tile_width;

	//analytical tile size
	sum_tile_size = (tile_ifmap_size + tile_filter_size + tile_ofmap_size)/element_unit * 2;

	selected_tile_height = (double)ifmap_height/systolic_width * systolic_width;
	selected_tile_width = MAX(1, (sqrt(2*(double)systolic_width*sum_tile_size+(double)(1+systolic_width)*(1+systolic_width)*ifmap_height*ifmap_height)-(1+systolic_width)*ifmap_height)/(2*systolic_width));
	selected_filter_tile_width = (sqrt((double)2*systolic_width*sum_tile_size+(double)(1+systolic_width)*(1+systolic_width)*ifmap_height*ifmap_height)-(1+systolic_width)*ifmap_height)/(2*systolic_width) * systolic_width;

	if(ifmap_height < selected_tile_height)
	{
		selected_tile_height = ifmap_height;
		selected_tile_width = MIN(MAX(1, MIN(sum_tile_size/selected_tile_height, ifmap_width)), ifmap_width);
	}
	else if(ifmap_width < selected_tile_width)
	{
		selected_tile_width = ifmap_width;
		selected_tile_height = MIN(MAX(1, MIN(sum_tile_size/selected_tile_width, ifmap_height)), ifmap_height);
	}

	selected_filter_tile_width = MIN(MAX(1, tile_filter_size/element_unit/selected_tile_width), filter_width);

	// Tile-based conversion
	int tile_id=0;
	uint64_t ifmap_tile_addr_offset = 0;
	uint64_t filter_tile_addr_offset = 0;
	uint64_t ofmap_tile_addr_offset = 0;
	uint64_t prev_ofmap_offset = 0;

	for(int i=0;i<ceil((double)ifmap_height/selected_tile_height);i++)
	{
		for(int j=0;j<ceil((double)ifmap_width/selected_tile_width);j++)
		{
			int ifmap_height_T = MIN(selected_tile_height, ifmap_height-selected_tile_height*i);
			int ifmap_width_T = MIN(selected_tile_width, ifmap_width-selected_tile_width*j);

			for(int k=0;k<ceil((double)filter_width/selected_filter_tile_width);k++)
			{
				int filter_width_T = MIN(selected_filter_tile_width, filter_width-selected_filter_tile_width*k);

				uint64_t ifmap_tile_addr = ifmap_base_addr + ifmap_tile_addr_offset * element_unit;
				uint64_t filter_tile_addr = filter_base_addr + filter_tile_addr_offset * element_unit;
				uint64_t ofmap_tile_addr = ofmap_base_addr + ofmap_tile_addr_offset * element_unit;
					
				write_config_string += layer_name+string("_T")+to_string(tile_id)+string(",")+to_string(ifmap_height_T)+string(",")+to_string(1)+string(",")+to_string(1)+string(",")+to_string(1)+string(",")+to_string(ifmap_width_T)+string(",")+to_string(filter_width_T)+string(",")+to_string(1)+string(",Conv,")+to_string(ifmap_tile_addr)+string(",")+to_string(filter_tile_addr)+string(",")+to_string(ofmap_tile_addr)+string(",\n");

				tile_id++;

				filter_tile_addr_offset += ifmap_width_T * filter_width_T;
				ofmap_tile_addr_offset += ifmap_height_T * filter_width_T;
			}

			ifmap_tile_addr_offset += ifmap_height_T * ifmap_width_T;
			if(j!=ceil((double)ifmap_width/selected_tile_width)-1)
				ofmap_tile_addr_offset = prev_ofmap_offset;
			else
				prev_ofmap_offset = ofmap_tile_addr_offset;
		}

		filter_tile_addr_offset = 0;
	}
}
