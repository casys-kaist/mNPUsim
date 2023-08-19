#include "memctrl.h"

/*NOTE: L2 TLB / Two-level AT is not complete (need verification and support of OoO TLB/DRAM Request submission)*/

//L2 TLB (single-level address translation)
NPUMemory::NPUMemory(NPUMemConfig config, bool fake_mode, bool is_log, int npu_idx, int total_npunum, TwoLevTLBConfig tlbconfig, TLB* shared_l2, PTWManager* ptw, DRAMAllocator* pmem_allocator, uint64_t dram_capacity)
	: m_npumemconfig(config), m_npu_idx(npu_idx), m_second_buffer(config.is_double_buffer), m_popped(false), m_no_req(true), m_spm0_idx(npu_idx), m_spm1_idx(npu_idx+total_npunum), m_is_begin(false), m_remain(0), m_spm0_req(0), m_spm1_req(0), m_token(NULL)
{
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

    m_tlb = NULL;
    if (shared_l2){
        m_offset = dram_capacity/(uint64_t)total_npunum*(uint64_t)npu_idx;
    }else{
        m_offset = 0;
    }

    if (ptw){
	    m_twolevtlb = new TwoLevTLB(NULL, shared_l2, NULL, pmem_allocator, tlbconfig, is_log, dram_capacity, dram_capacity, false, ptw);
    }else{//separated PTW
        PTWManager* newptw = new PTWManager(config.ptw_num, config.pt_step_num, tlbconfig.lev2_miss_latency, config.is_tpreg, tlbconfig.lev2_pagebits);
        m_twolevtlb = new TwoLevTLB(NULL, shared_l2, NULL, pmem_allocator, tlbconfig, is_log, dram_capacity, dram_capacity, false, newptw);
    }

    m_tlbqueue = new multimap<uint64_t, DRAMPacket>;
}


//Two-level address translation
NPUMemory::NPUMemory(NPUMemConfig config, bool fake_mode, bool is_log, int npu_idx, int total_npunum, TwoLevTLBConfig tlbconfig, TLB* shared_l2, PTWManager* ptw, DRAMAllocator* lmem_allocator, DRAMAllocator* pmem_allocator, uint64_t lmem_capacity, uint64_t pmem_capacity)
    : m_npumemconfig(config), m_npu_idx(npu_idx), m_second_buffer(config.is_double_buffer), m_popped(false), m_no_req(true), m_spm0_idx(npu_idx), m_spm1_idx(npu_idx+total_npunum), m_is_begin(false), m_spm0_req(0), m_spm1_req(0), m_token(NULL)
{
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
    m_tlb = NULL;
    m_offset = 0;
    if (ptw){
        m_twolevtlb = new TwoLevTLB(NULL, shared_l2, lmem_allocator, pmem_allocator, tlbconfig, is_log, lmem_capacity, pmem_capacity, true, ptw);
    }else{
        PTWManager* newptw = new PTWManager(config.ptw_num, config.pt_step_num, tlbconfig.lev2_miss_latency, config.is_tpreg, tlbconfig.lev2_pagebits);
        m_twolevtlb = new TwoLevTLB(NULL, shared_l2, lmem_allocator, pmem_allocator, tlbconfig, is_log, lmem_capacity, pmem_capacity, true, newptw);
    }

    m_tlbqueue = new multimap<uint64_t, DRAMPacket>;
}


//--------------------------------------------------
// name: NPUMemory::twolevtlbAccess
// arguments: same as those of TwoLevTLB::access
// return value: None
// usage: access device TLB for two-level translation
//--------------------------------------------------

void NPUMemory::twolevtlbAccess(uint64_t page_addr, uint64_t curtick, uint64_t* exectick, uint64_t* paddr, uint64_t offset, bool is_shared_l2)
{
	m_twolevtlb->access(page_addr + m_offset, curtick, exectick, paddr, offset, is_shared_l2, m_npu_idx);
}


//--------------------------------------------------
// name: NPUMemory::writeTwoLevTLBLog
// arguments: L1, L2 TLB log file name
// return value: None
// usage: write TLB access log
//--------------------------------------------------

void NPUMemory::writeTwoLevTLBLog(string l1tlb_fname, string l2tlb_fname)
{
	m_twolevtlb->writeLog(l1tlb_fname, l2tlb_fname);
}


//--------------------------------------------------
// name: MemoryController::npumemSetup
// usage: Two-Level ver.
//--------------------------------------------------

void MemoryController::npumemSetup(NPUMemConfig npuconfig, TwoLevTLBConfig tlbconfig, int npu_idx)
{
    if (m_memconfig->is_twolev_tran){//two-level AT (three address spaces)
        m_pagesize_lm = (uint32_t)pow(2, tlbconfig.lev1_pagebits);
        if (m_memconfig->is_shared_l2){//shared L2 TLB
            if (!m_shared_l2){//initial setup for shared-L2 case
                m_ptw = new PTWManager(npuconfig.ptw_num, npuconfig.pt_step_num, tlbconfig.lev2_miss_latency, npuconfig.is_tpreg, tlbconfig.lev2_pagebits);
                m_shared_l2 = new TLB(tlbconfig.lev2_hit_latency, tlbconfig.lev2_miss_latency, tlbconfig.lev2_assoc, tlbconfig.lev2_entrynum, tlbconfig.lev2_pagebits, m_memconfig->is_dram_log, tlbconfig.lev2_pref_mode, tlbconfig.lev2_portnum, m_ptw);
                m_pmem_allocator = new DRAMAllocator(m_pagesize, m_memconfig->dram_capacity, 0);
                m_shared_l2->setupTranslator(m_pmem_allocator, m_memconfig->dram_capacity, 0);
            }
            if (!m_lmem_allocator){
                m_lmem_allocator = new DRAMAllocator(m_pagesize_lm, m_memconfig->dram_capacity, 0);
            }
            m_npumemory[npu_idx] = new NPUMemory(npuconfig, m_fake_mode, m_memconfig->is_dram_log, npu_idx, m_npu_num, tlbconfig, m_shared_l2, m_ptw, m_lmem_allocator, m_pmem_allocator, m_memconfig->dram_capacity, m_memconfig->dram_capacity);
        }else{
            if (m_memconfig->is_shared_ptw){
                if (!m_ptw){
                    m_ptw = new PTWManager(npuconfig.ptw_num, npuconfig.pt_step_num, tlbconfig.lev2_miss_latency, npuconfig.is_tpreg, tlbconfig.lev2_pagebits);
                }
            }
            if (m_memconfig->is_dynamic_partition){//shared DRAM allocator
                if (!m_pmem_allocator){
                    m_pmem_allocator = new DRAMAllocator(m_pagesize_lm, m_memconfig->dram_capacity, 0);
                }
                m_npumemory[npu_idx] = new NPUMemory(npuconfig, m_fake_mode, m_memconfig->is_dram_log, npu_idx, m_npu_num, tlbconfig, m_shared_l2, m_ptw, m_lmem_allocator, m_pmem_allocator, m_memconfig->dram_capacity, m_memconfig->dram_capacity);
            }else{
                DRAMAllocator* pmem_allocator = new DRAMAllocator(m_pagesize, (m_memconfig->dram_capacity)/m_npu_num, m_addr_ptr);
                m_npumemory[npu_idx] = new NPUMemory(npuconfig, m_fake_mode, m_memconfig->is_dram_log, npu_idx, m_npu_num, tlbconfig, m_shared_l2, m_ptw, NULL, pmem_allocator, m_memconfig->dram_capacity, (m_memconfig->dram_capacity)/m_npu_num);
            }
        }
    }else{//single-level AT
        m_pagesize_lm = m_pagesize;
        if (m_memconfig->is_shared_l2){//shared L2 TLB
            if (!m_shared_l2){
                m_ptw = new PTWManager(npuconfig.ptw_num, npuconfig.pt_step_num, tlbconfig.lev2_miss_latency, npuconfig.is_tpreg, tlbconfig.lev2_pagebits);
                m_shared_l2 = new TLB(tlbconfig.lev2_hit_latency, tlbconfig.lev2_miss_latency, tlbconfig.lev2_assoc, tlbconfig.lev2_entrynum, tlbconfig.lev2_pagebits, m_memconfig->is_dram_log, tlbconfig.lev2_pref_mode, tlbconfig.lev2_portnum, m_ptw);
                m_pmem_allocator = new DRAMAllocator(m_pagesize, m_memconfig->dram_capacity, 0);
                m_shared_l2->setupTranslator(m_pmem_allocator, m_memconfig->dram_capacity, 0);
            }
            m_npumemory[npu_idx] = new NPUMemory(npuconfig, m_fake_mode, m_memconfig->is_dram_log, npu_idx, m_npu_num, tlbconfig, m_shared_l2, m_ptw, m_pmem_allocator, m_memconfig->dram_capacity);
        }else{
            if (m_memconfig->is_shared_ptw){
                if (!m_ptw){
                    m_ptw = new PTWManager(npuconfig.ptw_num, npuconfig.pt_step_num, tlbconfig.lev2_miss_latency, npuconfig.is_tpreg, tlbconfig.lev2_pagebits);
                }
            }
            if (m_memconfig->is_dynamic_partition){//shared DRAM allocator
                if (!m_pmem_allocator){
                    m_pmem_allocator = new DRAMAllocator(m_pagesize, m_memconfig->dram_capacity, 0);
                }
                m_npumemory[npu_idx] = new NPUMemory(npuconfig, m_fake_mode, m_memconfig->is_dram_log, npu_idx, m_npu_num, tlbconfig, NULL, m_ptw, m_pmem_allocator, m_memconfig->dram_capacity);
            }else{
                DRAMAllocator* pmem_allocator = new DRAMAllocator(m_pagesize, (m_memconfig->dram_capacity)/m_npu_num, m_addr_ptr);
                m_addr_ptr += (m_memconfig->dram_capacity)/m_npu_num;
                m_npumemory[npu_idx] = new NPUMemory(npuconfig, m_fake_mode, m_memconfig->is_dram_log, npu_idx, m_npu_num, tlbconfig, NULL, m_ptw, pmem_allocator, m_memconfig->dram_capacity);
            }
        }
    }
    if (m_memconfig->is_token_bw){
        m_npumemory[npu_idx]->setTokenThreshold(m_memconfig->channel_num*(m_memconfig->module_num));
    }
}


//--------------------------------------------------
// name: MemoryController::twolevtlbRequest
// arguments: memory address, data size, npu_idx, spm_idx, is_write
// return value: None
// usage: two-level translation version of tlbRequest
//--------------------------------------------------

void MemoryController::twolevtlbRequest(uint64_t addr, uint32_t size, uint32_t npu_idx, uint32_t spm_idx, bool is_write)
{
    uint64_t page_addr;
    uint64_t exectick;
    uint64_t per_size = size;
    for (page_addr=(addr - addr%m_pagesize); page_addr<(addr+size); page_addr+=m_pagesize){
        m_npumemory[npu_idx]->m_remain += 1;
		uint64_t offset = MAX(addr - page_addr, 0);
        DRAMPacket dpacket;
        dpacket.npu_idx = npu_idx;
		dpacket.spm_idx = spm_idx;
        dpacket.size = MIN(m_pagesize - offset, per_size);
		dpacket.is_write = is_write;
        dpacket.vaddr = MAX(addr, page_addr);
        dpacket.page_addr = page_addr;
        dpacket.paddr = ~0l;//before translation
        m_npumemory[npu_idx]->increaseReq(npu_idx);//tiled
        m_npumemory[npu_idx]->insertTLBQueue(m_clock, dpacket);
        per_size -= dpacket.size;
    }
}


//--------------------------------------------------
// name: MemoryController::writeTwoLevTLBLog()
// arguments: None
// return value: None
// usage: write two-level TLB log
//--------------------------------------------------

void MemoryController::writeTwoLevTLBLog()
{
    if (!(m_memconfig->is_dram_log)){
        return;
   }

	int i;
    for (i=0; i<m_npu_num; i++){
        string l1tlb_logname = "/l1tlb" + to_string(i);// + ".log";
        string l1tlb_fname = m_memconfig->dramoutdir + l1tlb_logname;
        string l2tlb_logname = "/l2tlb" + to_string(i);// + ".log";
        string l2tlb_fname = m_memconfig->dramoutdir + l2tlb_logname;
        m_npumemory[i]->writeTwoLevTLBLog(l1tlb_fname, l2tlb_fname);
    }
}
