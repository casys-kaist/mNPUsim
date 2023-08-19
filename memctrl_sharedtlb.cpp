#include "memctrl.h"

//shared_tlb
NPUMemory::NPUMemory(NPUMemConfig config, uint32_t pagebits, uint64_t dram_capacity, bool fake_mode, bool is_log, int npu_idx, int total_npunum, TLB* shared_tlb)
	: m_npumemconfig(config), m_npu_idx(npu_idx), m_second_buffer(config.is_double_buffer), m_popped(false), m_no_req(true), m_spm0_idx(npu_idx), m_spm1_idx(npu_idx+total_npunum), m_is_begin(false), m_remain(0), m_spm0_req(0), m_spm1_req(0), m_token(NULL)
{
	printf("NPU-%d with DRAM partition %ld B, Local SPM %d B, and %d-way TLB with %d entries\n", 
			npu_idx, dram_capacity, config.spm_size, config.tlb_assoc, config.tlb_entrynum);
	printf("Assigned SPM idx: %d, %d\n", m_spm0_idx, m_spm1_idx);
    m_tlb = shared_tlb;
    m_twolevtlb = NULL;
    m_shared_tlb = true;
    m_offset = dram_capacity/(uint64_t)total_npunum*(uint64_t)npu_idx;

	if (fake_mode){
		m_spm0 = NULL;
		m_spm1 = NULL;
	}else{
		m_spm0 = new ScratchPad(config.spm_latency, config.spm_size, config.spm_wordsize);
		if (config.is_double_buffer){
			m_spm1 = new ScratchPad(config.spm_latency, config.spm_size, config.spm_wordsize);
		}else{
			m_spm1 = NULL;
		}
	}

    m_tlbqueue = new multimap<uint64_t, DRAMPacket>;
}
