SRCS = accelerator.h common.h main.cpp npu_accelerator.cpp npu_group.cpp software_request_generator.cpp util.cpp util.h util_os.cpp software_layer/*.cpp
MEMSRCS = address_translator.cpp address_translator.h memconfig.cpp memctrl.cpp memctrl.h memctrl_sharedtlb.cpp npumemconfig.cpp ptw.cpp ptw.h spm.cpp spm.h tlb.cpp tlb.h
LINK = -L./DRAMsim3/src/
LIB = -L./DRAMsim3 -ldramsim3
DEBUG = -g -O2

all: single

single:
	export LD_LIBRARY_PATH=./DRAMsim3:$$LD_LIBRARY_PATH &&\
	g++ $(LINK) $(DEBUG) $(SRCS) $(MEMSRCS) -o mnpusim -std=c++11 $(LIB)

clean:
	rm -rf mnpusim single_test1/ single_alexnet/ single_test_gemv/ single_llama_3b_tpuv2/ single_opt_small_tpuv2/  single_yolo_tiny_tpuv2/ single_Resnet50_tpuv2/ single_Google_recommendation_tpuv2/ debug*.txt

single_test1:
	export LD_LIBRARY_PATH=./DRAMsim3:$$LD_LIBRARY_PATH &&\
	./mnpusim arch_config/core_architecture_list/tpu.txt network_config/netconfig_list/single/test1_network.txt dram_config/total_dram_config/single_hbm2_256gbs.cfg npumem_config/npumem_architecture_list/single.txt single_test1 misc_config/single.cfg

single_alexnet:
	export LD_LIBRARY_PATH=./DRAMsim3:$$LD_LIBRARY_PATH &&\
	./mnpusim arch_config/core_architecture_list/tpu.txt network_config/netconfig_list/single/alexnet_out_im2col.txt dram_config/total_dram_config/single_hbm2_256gbs.cfg npumem_config/npumem_architecture_list/single.txt single_alexnet misc_config/single.cfg

single_llama_3b_tpuv2:
	export LD_LIBRARY_PATH=./DRAMsim3:$$LD_LIBRARY_PATH &&\
	./mnpusim arch_config/core_architecture_list/tpuv2.txt network_config/netconfig_list/single/llama_3b.txt dram_config/total_dram_config/single_hbm2_512gbs.cfg npumem_config/npumem_architecture_list/none_tpuv2.txt single_llama_3b_tpuv2 misc_config/single.cfg

single_opt_small_tpuv2:
	export LD_LIBRARY_PATH=./DRAMsim3:$$LD_LIBRARY_PATH &&\
	./mnpusim arch_config/core_architecture_list/tpuv2.txt network_config/netconfig_list/single/opt_small.txt dram_config/total_dram_config/single_hbm2_512gbs.cfg npumem_config/npumem_architecture_list/none_tpuv2.txt single_opt_small_tpuv2 misc_config/single.cfg

single_yolo_tiny_tpuv2:
	export LD_LIBRARY_PATH=./DRAMsim3:$$LD_LIBRARY_PATH &&\
	./mnpusim arch_config/core_architecture_list/tpuv2.txt network_config/netconfig_list/single/yolo_tiny_ws.txt dram_config/total_dram_config/single_hbm2_512gbs.cfg npumem_config/npumem_architecture_list/none_tpuv2.txt single_yolo_tiny_tpuv2 misc_config/single.cfg

single_Resnet50_tpuv2:
	export LD_LIBRARY_PATH=./DRAMsim3:$$LD_LIBRARY_PATH &&\
	./mnpusim arch_config/core_architecture_list/tpuv2.txt network_config/netconfig_list/single/Resnet50_ws.txt dram_config/total_dram_config/single_hbm2_512gbs.cfg npumem_config/npumem_architecture_list/none_tpuv2.txt single_Resnet50_tpuv2 misc_config/single.cfg

single_Google_recommendation_tpuv2:
	export LD_LIBRARY_PATH=./DRAMsim3:$$LD_LIBRARY_PATH &&\
	./mnpusim arch_config/core_architecture_list/tpuv2.txt network_config/netconfig_list/single/Google_recommendation_ws.txt dram_config/total_dram_config/single_hbm2_512gbs.cfg npumem_config/npumem_architecture_list/none_tpuv2.txt single_Google_recommendation_tpuv2 misc_config/single.cfg

single_gemv:
	export LD_LIBRARY_PATH=./DRAMsim3:$$LD_LIBRARY_PATH &&\
	./mnpusim arch_config/core_architecture_list/mvunit.txt network_config/netconfig_list/single/test_gemv.txt dram_config/total_dram_config/single_hbm2_256gbs.cfg npumem_config/npumem_architecture_list/single.txt single_test_gemv misc_config/single.cfg
