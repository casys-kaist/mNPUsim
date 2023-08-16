//--------------------------------------------------
// name: software_request_generator::gemm_computation
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
}
