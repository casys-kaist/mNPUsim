# mNPUsim: A Cycle-accurate Multi-core NPU simulator

## Notice
2024/12/10: The white paper "mNPUsim Explained" is released on GitHub repository of mNPUsim.

2024/12/05: Support for Dot product-based GEMV computation added. See newly added configuration files (arch_config/core_architecture/arch_mvunit_128.csv, network_config/network_architecture/test/test_gemv.csv, network_config/README.md) for more information.

2023/11/30: Need to update submodule (DRAMsim3 variant) with 'git submodule update --remote'

## White Paper
![Link to the white paper](https://github.com/casys-kaist/mNPUsim/blob/main/mNPUsim_Explained.pdf?raw=true)


## Simulator Outline
mNPUsim is a cycle-accurate simulator modeling NPU system with off-chip memory.
![Overview](./mnpusim_multinpu_overview.png)

## Getting Started
```bash
# Load DRAMsim3 as a submodule
git submodule update --init --recursive

# Create shared library
cd DRAMsim3
make libdramsim3.so
cd ..

# Run test
make
make single_test1
```

## Options (Set in common.h)
* SRAM_TRACE: Generates the SRAM trace in result_path/intermediate/(sram files). If SRAM_TRACE=false, only cycle is computed. (default: false)
* DRAMREQ_NPU_TRACE: Option for the dramreq trace in npu-side cycle. (default: false)
* DEBUG: Debug option. (default: false)

## Parameters
As shown in Makefile, it requires six parameters: 1) core architecture, 2) target network, 3) target dram, 4) npumem config, 5) result path, and 6) misc config.
Each configuration has list of single configuration files and result path is for output directory.

## Results
* dramsim_output: The access information of dram, tlb, and pagetable walkers. When the dram req sent, the start-cycle is written in dram.log and the end-cycle is written in dramreq.log.
* intermediate_config: Reconstructed configuration. Unlike original network file, it considers the GEMM translation (e.g the im2col-out convolution), absolute address translation.
* intermediate: The intermediate results except the memory constraints. Based on this intermediate result, our simulator generates the memory requests. The format of output line is (cycle), (list of address). Therefore, only first value (cycle) is remained when SRAM_TRACE is false.
* result: Several summary files: 1) Average cycles, 2) the size of memory footprint, 3) per layer execution cycle, and 4) PE utilization.

## Publication
* Title: mNPUsim: Evaluating the Effect of Sharing Resources in Multi-core NPUs
* Conference: IISWC 2023
* Authors: Soojin Hwang*, Sunho Lee*, Jungwoo Kim, Hongbeen Kim and Jaehyuk Huh (*: Equal contribution)

## Citation
If you use mNPUsim for your research, please cite our paper:
```bash
@inproceedings{mnpusim,
  author = {Hwang, Soojin and Lee, Sunho and Kim, Jungwoo and Kim, Hongbeen and Huh, Jaehyuk},
  title = {{mNPUsim: Evaluating the Effect of Sharing Resources in Multi-core NPUs}},
  booktitle = {2023 IEEE International Symposium on Workload Charcterization (IISWC)},
  year = {2023},
  doi = {10.1109/IISWC59245.2023.00018}
}
```
