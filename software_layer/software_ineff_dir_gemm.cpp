#include "../util.h"
#include "../accelerator.h"

//--------------------------------------------------
// name: software_request_generator::ineff_dir_gemm_computation()
//--------------------------------------------------
void software_request_generator::ineff_dir_gemm_computation()
{
	write_config_string += layer_name+string(",")+to_string(ifmap_height)+string(",")+to_string(ifmap_width)+string(",")+to_string(ifmap_width)+string(",")+to_string(filter_width)+string(",")+to_string(1)+string(",")+to_string(1)+string(",")+to_string(1)+string(",Ineff_Dir_Tile_Gemm,")+to_string(ifmap_base_addr)+string(",")+to_string(filter_base_addr)+string(",")+to_string(ofmap_base_addr)+string(",\n");
}
