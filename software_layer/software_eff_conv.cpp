//--------------------------------------------------
// name: software_request_generator::conv_fold_parallel_computation
//--------------------------------------------------
void software_request_generator::conv_fold_parallel_computation()
{
	//init idx & offset
	int **local_ifmap_fold = (int **)malloc(systolic_width*sizeof(int *));
	int **local_filter_fold = (int **)malloc(systolic_width*sizeof(int *));
	int **offset = (int **)malloc(systolic_width*sizeof(int *));

	for(int x=0;x<systolic_width;x++)
	{
		local_ifmap_fold[x] = (int*)malloc(systolic_height*sizeof(int));
		local_filter_fold[x] = (int*)malloc(systolic_height*sizeof(int));
		offset[x] = (int*)malloc(systolic_height*sizeof(int));
		for(int y=0;y<systolic_height;y++)
		{
			local_ifmap_fold[x][y]=0;
			local_filter_fold[x][y]=0;
			offset[x][y]=0-x-y;
		}
	}

	while(true)
	{
		//check_finish_or_not
		bool is_fin=true;
		for(int x=0;x<systolic_width;x++)
		{
			for(int y=0;y<systolic_height;y++)
			{
				int filter_index = local_filter_fold[x][y]*systolic_width+x;
				int ifmap_index = local_ifmap_fold[x][y] * systolic_height + y;
				//ifmap_idx, filter_idx condition.
				if(ifmap_index<ifmap_iter_width*ifmap_iter_height && filter_index<filter_num)
					is_fin=false;
			}
		}

		if(is_fin)
			break;
	}


	//free
	for(int i=0;i<systolic_width;i++)
	{
		free(local_ifmap_fold[i]);
		free(local_filter_fold[i]);
		free(offset[i]);
	}
	free(local_ifmap_fold);
	free(local_filter_fold);
	free(offset);
}
