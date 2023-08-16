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
