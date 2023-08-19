#include "memctrl.h"

//Per-device side memory manager
NPUMemory::NPUMemory(NPUMemConfig config, uint32_t pagebits, DRAMAllocator* allocator, uint64_t dram_capacity, bool fake_mode, bool is_log, int npu_idx, int total_npunum, PTWManager* ptw)
	: m_npumemconfig(config), m_npu_idx(npu_idx), m_second_buffer(config.is_double_buffer), m_popped(false), m_no_req(true), m_spm0_idx(npu_idx), m_spm1_idx(npu_idx+total_npunum), m_is_begin(false), m_remain(0), m_spm0_req(0), m_spm1_req(0), m_token(NULL)
{
	printf("NPU-%d with DRAM partition %ld B, Local SPM %d B, and %d-way TLB with %d entries\n", 
			npu_idx, dram_capacity, config.spm_size, config.tlb_assoc, config.tlb_entrynum);
	printf("Assigned SPM idx: %d, %d\n", m_spm0_idx, m_spm1_idx);
    if (ptw){
	    m_tlb = new TLB(config.tlb_hit_latency, config.tlb_miss_latency, config.tlb_assoc, config.tlb_entrynum, pagebits, is_log, config.tlb_pref_mode, config.tlb_portnum, ptw);
    }else{
        PTWManager* newptw = new PTWManager(config.ptw_num, config.pt_step_num, config.tlb_miss_latency, config.is_tpreg, pagebits);
        m_tlb = new TLB(config.tlb_hit_latency, config.tlb_miss_latency, config.tlb_assoc, config.tlb_entrynum, pagebits, is_log, config.tlb_pref_mode, config.tlb_portnum, newptw);
    }
	m_tlb->setupTranslator(allocator, dram_capacity, allocator->firstAddr());
    m_shared_tlb = false;
    m_offset = 0;

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


//--------------------------------------------------
// name: NPUMemory::tlbAccess
// arguments: same as those of TLB::access
// return value: None
// usage: access device TLB
//--------------------------------------------------

void NPUMemory::tlbAccess(uint64_t page_addr, uint64_t curtick, uint64_t* exectick, uint64_t* paddr)
{
    if (m_tlb){
        if (m_shared_tlb){
            m_tlb->access(page_addr + m_offset, curtick, exectick, paddr, m_npu_idx);
        }
        else{
	        m_tlb->access(page_addr, curtick, exectick, paddr, m_npu_idx);
        }
    }
}


//--------------------------------------------------
// name: NPUMemory::writeTLBLog
// arguments: TLB log file name
// return value: None
// usage: write TLB access log
//--------------------------------------------------

void NPUMemory::writeTLBLog(string tlb_fname)
{
	m_tlb->writeLog(tlb_fname);
}


//--------------------------------------------------
// name: NPUMemory::receiveTransaction
// arguments: (virtual) address, clocktick, buffer_idx
// return value: None
// usage: access SPM for callback
//--------------------------------------------------

void NPUMemory::receiveTransaction(uint64_t addr, uint64_t curtick, int buffer_idx)
{
	if (buffer_idx == m_spm0_idx){
		m_spm0->receiveTransaction(addr, curtick);
	}else if (buffer_idx == m_spm1_idx){
		m_spm1->receiveTransaction(addr, curtick);
	}else{
		printf("Fatal Error: wrong SPM idx assigned (SPM0 %d, SPM1 %d, while requested %d\n)", m_spm0_idx, m_spm1_idx, buffer_idx);
	}
}


//--------------------------------------------------
// name: NPUMemory::spmAccess
// arguments: (virtual) address, clocktick, is_write
// return value: exec tick
// usage: access SPM
//--------------------------------------------------
uint64_t NPUMemory::spmAccess(uint64_t addr, uint64_t curtick, bool is_write)
{
	if (is_write == m_second_buffer){
		return m_spm1->access(addr, curtick, is_write);
	}else{
		return m_spm0->access(addr, curtick, is_write);
	}
}


//--------------------------------------------------
// name: NPUMemory::printTLBStat
// arguments: None
// return value: None
// usage: print TLB stat
//--------------------------------------------------

void NPUMemory::printTLBStat()
{
	printf("*** Stat of TLB %d ***\n", m_npu_idx);
	if (m_tlb){
		printf("Access: %ld\n", m_tlb->numAccess());
		printf("Miss: %ld\n", m_tlb->numMiss());
	}else{
		m_twolevtlb->printStat();
	}
	printf("*** End of TLB %d Stat\n", m_npu_idx);
}


//--------------------------------------------------
// name: NPUMemory::npu2dramTick
// arguments: npu_clock (a cycle in NPU clock frequency)
// return value: npu_clock converted into DRAM clock
// usage: A calculator
//--------------------------------------------------

uint64_t NPUMemory::npu2dramTick(uint64_t npu_clock)
{
	if (m_npumemconfig.dram_clockspeed == m_npumemconfig.npu_clockspeed){
		return npu_clock;
	}else{
		return (uint64_t)ceil((double)npu_clock*(double)(m_npumemconfig.dram_clockspeed)/(double)(m_npumemconfig.npu_clockspeed));
	}
}


//--------------------------------------------------
// name: NPUMemory::dram2npuTick
// usage: reverse of npu2dramTick
//--------------------------------------------------

uint64_t NPUMemory::dram2npuTick(uint64_t dram_clock)
{
    if (m_npumemconfig.dram_clockspeed == m_npumemconfig.npu_clockspeed){
        return dram_clock;
    }else{
        return (uint64_t)ceil((double)dram_clock*(double)(m_npumemconfig.npu_clockspeed)/(double)(m_npumemconfig.dram_clockspeed));
    }
}

//--------------------------------------------------
// name: NPUMemory::tlbFlush()
//--------------------------------------------------

void NPUMemory::tlbFlush(uint64_t* exectick)
{
    m_tlb->flush(m_npu_idx, exectick);
}


//--------------------------------------------------
// name: NPUMemory::getTLBQueueHead()
// usage: subroutine for OoO request submission
//--------------------------------------------------

void NPUMemory::getTLBQueueHead(uint64_t* tick_saver, DRAMPacket* packet_saver)
{
    multimap<uint64_t, DRAMPacket>::iterator iter = m_tlbqueue->begin();

    if (m_tlbqueue->end() != iter){
        (*tick_saver) = iter->first;
        (*packet_saver) = iter->second;
        m_tlbqueue->erase(iter);
    }else{
        (*tick_saver) = ~0l;
    }
}


//--------------------------------------------------
// name: NPUMemory::setTokenThreshold()
// usage: set token threshold for fair sharing
//--------------------------------------------------

void NPUMemory::setTokenThreshold(int total_channel_num)
{
    printf("Assigned %d token per channel\n", m_npumemconfig.token_bucket);
    m_token_thres = m_npumemconfig.token_bucket;
    m_token_step = m_npumemconfig.token_epoch;
    m_total_channel_num = total_channel_num;
    m_token = (int*)malloc(sizeof(int)*total_channel_num);
    int i;
    for (i=0; i<total_channel_num; i++){
        m_token[i] = m_token_thres;
    }
}


//--------------------------------------------------
// name: NPUMemory::tryConsumeToken()
// usage: try to consume token. return false if no token exists.
//--------------------------------------------------

bool NPUMemory::tryConsumeToken(int total_channel_idx)
{
    if (m_token[total_channel_idx] > 0){
        m_token[total_channel_idx]--;
        return true;
    }else{
        return false;
    }
}


//--------------------------------------------------
// name: NPUMemory::tryAddToken()
// usage: try to add token.
//--------------------------------------------------

void NPUMemory::tryAddToken()
{
    int i;
    for (i=0; i<m_total_channel_num; i++){
        if (m_token[i] < m_token_thres){
            m_token[i]++;
        }
    }
}


//Shared memory controller for DRAM access management
MemoryController::MemoryController(MemoryConfig memconfig, bool twolevel)
    : m_clock(0), m_dramsim(NULL), m_npu_num(memconfig.npu_num), m_twolevtlb(twolevel), m_dram_unit(memconfig.dram_unit), m_addr_ptr(0),
    m_dramrdtraffic(0), m_dramwrtraffic(0), m_dramrdaccess(0), m_dramwraccess(0), m_log_activated(false), m_reqlog_activated(false), m_fake_mode(true), m_module_num(memconfig.module_num)
{
    m_memconfig = new MemoryConfig;
    (*m_memconfig) = memconfig;
	int i;
	m_npumemory = (NPUMemory**)malloc(sizeof(NPUMemory*)*m_npu_num);
	m_pagesize = (uint32_t)pow(2, memconfig.pagebits);
	m_pmem_allocator = NULL;
    m_shared_l1 = NULL;
	m_shared_l2 = NULL;
	m_lmem_allocator = NULL;
    m_ptw = NULL;
    m_tlbqueue = new multimap<uint64_t, DRAMPacket>;
    m_pagequeue = new multimap<uint64_t, DRAMPacket>;
	m_finished_set = new set<uint32_t>;

	//queue initialization
	m_buffer_num = 2*m_npu_num;
	m_reqs = (list<uint64_t>**)malloc(sizeof(list<uint64_t>*)*m_buffer_num);
	for (i=0; i<m_buffer_num; i++){
		m_reqs[i] = new list<uint64_t>;
	}

    m_cur_buffer_idx = ~0;
}//should be followed by npumemSetup(->twolevTLBSetup,) and dramSetup


//--------------------------------------------------
// name: MemoryController::npumemSetup
// arguments: npu-side memory config, npu_idx
// return value: None
// usage: setup NPU-side memory constraints and TLB
// Note: should be used right after initalization, before dramRequest
//--------------------------------------------------

void MemoryController::npumemSetup(NPUMemConfig config, int npu_idx)
{
    if (m_memconfig->is_shared_tlb){
        if (!m_shared_l1){
            m_ptw = new PTWManager(config.ptw_num, config.pt_step_num, config.tlb_miss_latency, config.is_tpreg, m_memconfig->pagebits);
            m_shared_l1 = new TLB(config.tlb_hit_latency, config.tlb_miss_latency, config.tlb_assoc, config.tlb_entrynum, m_memconfig->pagebits, m_memconfig->is_dram_log, config.tlb_pref_mode, config.tlb_portnum, m_ptw);
            m_shared_l1->setupTranslator(m_pmem_allocator, m_memconfig->dram_capacity, 0);
        }
        m_npumemory[npu_idx] = new NPUMemory(config, m_memconfig->pagebits, m_memconfig->dram_capacity, m_fake_mode, m_memconfig->is_dram_log, npu_idx, m_npu_num, m_shared_l1);
        if (m_memconfig->is_token_bw){
            m_npumemory[npu_idx]->setTokenThreshold(m_memconfig->channel_num*(m_memconfig->module_num));
        }
        return;
    }
    //separated TLB
    if (m_memconfig->is_shared_ptw){
        if (!m_ptw){
            m_ptw = new PTWManager(config.ptw_num, config.pt_step_num, config.tlb_miss_latency, config.is_tpreg, m_memconfig->pagebits);
        }
    }
	if (m_memconfig->is_dynamic_partition){
		if (!m_pmem_allocator){
			m_pmem_allocator = new DRAMAllocator(m_pagesize, m_memconfig->dram_capacity, 0);//Share PMEM allocator
		}
		m_npumemory[npu_idx] = new NPUMemory(config, m_memconfig->pagebits, m_pmem_allocator, m_memconfig->dram_capacity, m_fake_mode, m_memconfig->is_dram_log, npu_idx, m_npu_num, m_ptw);
	}else{
		DRAMAllocator* allocator = new DRAMAllocator(m_pagesize, (m_memconfig->dram_capacity)/m_npu_num, m_addr_ptr);//Separated PA space
		m_addr_ptr += (m_memconfig->dram_capacity)/m_npu_num;
		m_npumemory[npu_idx] = new NPUMemory(config, m_memconfig->pagebits, allocator, m_memconfig->dram_capacity/m_npu_num, m_fake_mode, m_memconfig->is_dram_log, npu_idx, m_npu_num, m_ptw);
	}
    if (m_memconfig->is_token_bw){
        m_npumemory[npu_idx]->setTokenThreshold(m_memconfig->channel_num*(m_memconfig->module_num));
    }
}


//--------------------------------------------------
// name: MemoryController::ptwSetup (for single-level TLB)
// usage: setting up shared-PTW partition
//--------------------------------------------------

void MemoryController::ptwSetup(int* walker_init, int* walker_upper, int* walker_lower)
{
    if ((!m_memconfig->is_shared_ptw) && (!m_memconfig->is_shared_tlb)){
        //No need to setup PTW partition
        printf("Separated PTW. No partitioning applied.\n");
        return;
    }
    int i;
    for (i=0; i < m_npu_num; i++){
        if (walker_init[i]){//not greedy
            m_npumemory[i]->ptwSetup(walker_init[i], walker_upper[i], walker_lower[i], m_npu_num, m_memconfig->is_return_ptw);
        }else{
            printf("Greedy PTW partitioning.\n");
        }
    }
}


//--------------------------------------------------
// name: MemoryController::dramSetup
// arguments: dramsim3 setup arguments
// return value: None
// usage: setup dramsim
// Note: should be used right after initalization, before dramRequest / cb_func is this->callback
//--------------------------------------------------

void MemoryController::dramSetup(const string &dramconfig, const string &dramoutdir, function<void(uint64_t)>cb_func)
{
    cout << "DRAM configuration file name: " << dramconfig << endl;
    m_dramsim = (MemorySystem**)malloc(sizeof(MemorySystem*)*m_module_num);
    int i;
    for (i=0; i<m_module_num; i++){
        m_dramsim[i] = new MemorySystem(dramconfig, dramoutdir, cb_func, cb_func);
    }
    printf("%d Modules in total\n", m_module_num);
}

//--------------------------------------------------
// name: MemoryController::callback
// arguments: memory address
// return value: None
// usage: callback function for memory read operation using dramsim3.
//--------------------------------------------------

void MemoryController::callback(uint64_t addr)
{
    //search from request queue
    int i;
    for (i=0; i<m_buffer_num; i++){
        list<uint64_t>::iterator iter = find(m_reqs[i]->begin(), m_reqs[i]->end(), addr);
        if (iter != (m_reqs[i]->end())){
            m_reqs[i]->remove(addr);
            break;
        }
    }

	m_npumemory[i%m_npu_num]->m_remain -= 1;
    m_npumemory[i%m_npu_num]->decreaseReq(i%m_npu_num);
	writeDRAMLog(m_clock, addr, i%m_npu_num);

    //send to associated SPM
    //printf("Callback to SPM %d\n", i);
	if (m_fake_mode){
		return;
	}else{
		m_npumemory[i%m_npu_num]->receiveTransaction(addr, m_clock, i);
	}
};


//--------------------------------------------------
// name: MemoryController::dramRequest
// arguments: list(map) of (memory address, data size), is_write, npu_idx (for multi tenancy)
// return value: None
// usage: main function of memory-side (parallel execution with core-side). should be followed by sendRequest()
//--------------------------------------------------

void MemoryController::dramRequest(map<uint64_t, int>* requests, bool is_write, uint32_t npu_idx)
{
	uint32_t spm_idx = m_npumemory[npu_idx]->calcSPMidx(is_write);

	//real request into queue..
	map<uint64_t, int>::iterator m_iter;
    int howmany = 0;
    for (m_iter=requests->begin(); m_iter!=requests->end(); m_iter++){
        uint64_t addr = m_iter->first;
        int size = m_iter->second;
        int partition;
        for (partition = 0; partition < size; partition += m_dram_unit){
		    if (!m_twolevtlb){
        	    tlbRequest(addr + partition, MIN(m_dram_unit, size - partition), npu_idx, spm_idx, is_write);
		    }else{
			    twolevtlbRequest(addr + partition, MIN(m_dram_unit, size - partition), npu_idx, spm_idx, is_write);
		    }
            if (is_write){
                m_dramwraccess++;
            }else{
                m_dramrdaccess++;
            }
            howmany++;
        }
    }
    printf("end of dramRequest for NPU-%d - %d requests, at tick %ld\n", npu_idx, howmany, m_clock);
}


//--------------------------------------------------
// name: MemoryController::tlbRequest
// arguments: memory address, data size, npu idx, spm_idx, is_write
// return value: None
// usage: subroutine of MemoryController::dramRequest
//--------------------------------------------------

void MemoryController::tlbRequest(uint64_t addr, uint32_t size, uint32_t npu_idx, uint32_t spm_idx, bool is_write)
{
    uint64_t page_addr;
    uint64_t exectick;
	uint64_t per_size = size;
    //printf("tlbRequest() call for address 0x%lx, tick %ld\n", addr, m_clock);
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
// name: MemoryController::calcModuleIdx()
//--------------------------------------------------

uint32_t MemoryController::calcModuleIdx(uint64_t p_addr)
{
    if ((m_memconfig->is_dynamic_partition)||(m_memconfig->module_num != m_memconfig->npu_num)){
        //page-wise interleaving
        return (uint32_t)((p_addr/(uint64_t)(m_pagesize*m_memconfig->channel_num))%(uint64_t)m_module_num);
    }else{
        return (uint32_t)(p_addr/((m_memconfig->dram_capacity)/(uint64_t)(m_memconfig->module_num)));
    }
}


//--------------------------------------------------
// name: MemoryController:: calcModuleAddr()
//
//--------------------------------------------------

uint64_t MemoryController::calcModuleAddr(uint64_t p_addr)
{
    if ((m_memconfig->is_dynamic_partition)||(m_memconfig->module_num != m_memconfig->npu_num)){
        //page-wise interleaving
        uint64_t offset = p_addr%(uint64_t)(m_pagesize*(m_memconfig->channel_num));
        uint64_t pagenum = (p_addr/(uint64_t)(m_pagesize*(m_memconfig->channel_num)))/(uint64_t)m_module_num;
        return (pagenum*(uint64_t)m_pagesize*(m_memconfig->channel_num)) + offset;
    }else{
        return (p_addr%((m_memconfig->dram_capacity)/(uint64_t)(m_memconfig->module_num)));
    }
};

//--------------------------------------------------
// name: MemoryController::clockTick()
//--------------------------------------------------

void MemoryController::clockTick()
{
    m_clock++;
    int j;
    for (j=0; j<m_module_num; j++){
        m_dramsim[j]->ClockTick();
    }
    if (m_memconfig->is_token_bw){
        int i;
        for (i=0; i<m_npu_num; i++){
            if (!(m_clock%(m_npumemory[i]->getTokenStep()))){
            m_npumemory[i]->tryAddToken();
            }
        }
    }
}


//--------------------------------------------------
// name: MemoryController::atomic()
// arguments: None
// return value: None
// usage: The atomic version of sendRequest
//--------------------------------------------------

void MemoryController::atomic()
{
    //printf("Tick %ld\n", m_clock);
    //printf("Page queue size: %d\n", m_pagequeue->size());
    multimap<uint64_t, DRAMPacket>::iterator iter;
    //merge per-NPU tlbqueue into shared tlbqueue
    int i;
    bool* is_queue_empty = (bool*)malloc(sizeof(bool)*m_npu_num);
    for (i=0; i<m_npu_num; i++){
        is_queue_empty[i] = false;
    }
    while (true){
        for (i=0; i<m_npu_num; i++){
            if (!is_queue_empty[i]){
                uint64_t tmp_tick;
                DRAMPacket tmp_packet;
                m_npumemory[i]->getTLBQueueHead(&tmp_tick, &tmp_packet);
                if (tmp_tick == (~0l)){
                    is_queue_empty[i] = true;
                }else{
                    //printf("Pop TLBqueue! (Time %ld, VA 0x%lx)\n", tmp_tick, tmp_packet.vaddr);
                    m_tlbqueue->insert(make_pair(tmp_tick, tmp_packet));
                }
            }
        }
        bool total = true;
        for (i=0; i<m_npu_num; i++){
            total &= is_queue_empty[i];
        }
        if (total){
            break;
        }
    }
    free(is_queue_empty);

    if (m_cur_buffer_idx == ~0){//tiled
        if (m_pagequeue->begin() != m_pagequeue->end()){
            m_cur_buffer_idx = ((m_pagequeue->begin())->second).npu_idx;
        }
    }
    //TLB access
    for (iter=m_tlbqueue->begin(); iter!=m_tlbqueue->end();){
        uint64_t exectick;
        uint64_t paddr;
        assert(iter->first == m_clock);
        DRAMPacket dpacket = iter->second;
        if (!m_twolevtlb){
            m_npumemory[dpacket.npu_idx]->tlbAccess(dpacket.page_addr, m_clock, &exectick, &paddr);
        }else{
            uint64_t offset = (dpacket.vaddr)%m_pagesize;
            m_npumemory[dpacket.npu_idx]->twolevtlbAccess(dpacket.page_addr, m_clock, &exectick, &paddr, offset, m_memconfig->is_shared_l2);
        }
        m_npumemory[dpacket.npu_idx]->m_popped = false;
        m_npumemory[dpacket.npu_idx]->m_no_req = false;
        dpacket.paddr = paddr + (dpacket.vaddr - dpacket.page_addr);
        m_reqs[dpacket.spm_idx]->push_back(dpacket.paddr);
        iter = m_tlbqueue->erase(iter);
        m_pagequeue->insert(make_pair(exectick, dpacket));
        if (exectick == ~(0l)){
            printf("Error: invalid exectick for address 0x%lx!\n", dpacket.vaddr);
            exit(-1);
        }
    }

    //DRAMAccess
    for (iter=m_pagequeue->begin(); iter!=m_pagequeue->end();){
        uint64_t exectick = iter->first;
        DRAMPacket dpacket = iter->second;
        if (m_clock >= exectick){
            uint64_t waddr = dpacket.paddr;
            uint64_t v_waddr = dpacket.vaddr;
            //printf("TryTransaction with addr 0x%lx at tick %ld\n", waddr, m_clock);
            if (m_memconfig->is_tiled_bw){//tiled-blocking
                if (dpacket.npu_idx != m_cur_buffer_idx){
                    iter++;
                    continue;
                }
            }
            if (m_memconfig->is_token_bw){
                int total_chan = calcTotalChannel(waddr);
                if (!m_npumemory[dpacket.npu_idx]->tryConsumeToken(total_chan)){
                    //Try to borrow token
                    if (!((m_memconfig->is_conserv_token)&& tryBorrowToken(dpacket.npu_idx, total_chan))){
                        iter++;
                        continue;
                    }
                }
            }
            if (!(m_dramsim[calcModuleIdx(waddr)]->WillAcceptTransaction(waddr, dpacket.is_write))){
                //return already-consumed token
                if (m_memconfig->is_token_bw){
                    m_npumemory[dpacket.npu_idx]->tryAddToken();
                }
                iter++;
                continue;
            }
            //m_npumemory[dpacket.npu_idx]->decreaseReq(dpacket.npu_idx);//tiled
            m_npumemory[dpacket.npu_idx]->m_is_begin = true;
            writeDRAMReqLog(m_clock, waddr, dpacket.npu_idx);//log
            m_dramsim[calcModuleIdx(waddr)]->AddTransaction(waddr, dpacket.is_write);
            iter = m_pagequeue->erase(iter);
        }else{
            //printf("Iter tick %ld exceeds current tick %ld!\n", exectick, m_clock);
            break;//note that m_pagequeue is sorted wrt tick!
        }
    }
}


//--------------------------------------------------
// name: MemoryController::tryBorrowToken
// arguments: requested npu idx, channel idx (global), current tick
// return value: (is rent succeed) ? 1 :0
// usage: Very naive & simple conservative token sharing
//--------------------------------------------------

bool MemoryController::tryBorrowToken(uint32_t npu_idx, int total_chan)
{
    //search for neighbor's token usage
    multimap<uint64_t, DRAMPacket>::iterator iter;
    int* candidate_list = (int*)calloc(m_npu_num, sizeof(int));
    for (iter=m_pagequeue->begin(); iter!=m_pagequeue->end(); iter++){
        uint64_t exectick = iter->first;
        DRAMPacket dpacket = iter->second;
        if (dpacket.npu_idx == npu_idx){//not a neighbor
            continue;
        }
        else if (exectick > m_clock + (uint64_t)(m_npumemory[dpacket.npu_idx]->getTokenStep())){//if exceed window
            continue;
        }
        else if (calcTotalChannel(dpacket.paddr) != total_chan){//not in the same channel
            continue;
        }
        candidate_list[dpacket.npu_idx]++;
    }
    int i;
    bool borrow = false;
    for (i=0; i<m_npu_num; i++){
        if ((!candidate_list[i]) && i != npu_idx){
            //borrow one token
            assert(m_npumemory[i]->tryConsumeToken(total_chan));
            borrow = true;
            break;
        }
    }
    free(candidate_list);
    return borrow;
}


//--------------------------------------------------
// name: MemoryController::checkSPMRequest
// arguments: spm idx
// return value: (is request queue empty) ? 1 : 0
// usage: check if all requests are finished for certain SPM
//--------------------------------------------------

bool MemoryController::checkSPMRequest(uint32_t spm_idx)
{
//    if ((m_reqs[spm_idx]->size() == 0) && (m_cur_buffer_idx == spm_idx) && (m_npumemory[spm_idx%m_npu_num]->isReqEmpty(spm_idx))){
//        m_cur_buffer_idx = ~0;
//    }
    return (m_reqs[spm_idx]->size() == 0);
}


//--------------------------------------------------
// name: MemoryController::popSPMRequest
// arguments: spm idx
// return value: None
// usage: pop out all requests for certain SPM
//--------------------------------------------------

void MemoryController::popSPMRequest(uint32_t spm_idx)
{
    while (!checkSPMRequest(spm_idx)){
        atomicTick();
    }
}


//--------------------------------------------------
// name: MemoryController::checkNPURequest
// arguments: npu idx
// return value: (is request queue empty) ? 1 : 0
// usage: check if all requests are finished for certain NPU
//--------------------------------------------------

bool MemoryController::checkNPURequest(uint32_t npu_idx)
{
	if (m_finished_set->find(npu_idx) != m_finished_set->end()){
		return false;
	}
    if (!(m_npumemory[npu_idx]->m_is_begin)){
        return false;
    }
    //tiled
    if (checkSPMRequest(npu_idx) && checkSPMRequest(npu_idx + m_npu_num) && (m_cur_buffer_idx == npu_idx) && (m_npumemory[npu_idx]->isReqEmpty(npu_idx))){
        m_cur_buffer_idx = ~0;
    }
    return (checkSPMRequest(npu_idx) & checkSPMRequest(npu_idx + m_npu_num) & (m_npumemory[npu_idx]->isTLBQueueEmpty()));
}


//--------------------------------------------------
// name: MemoryController::popNPURequest
// arguments: npu idx
// return value: None
// usage: pop out all requests for certain NPU
//--------------------------------------------------

void MemoryController::popNPURequest(uint32_t npu_idx)
{
    while (!checkNPURequest(npu_idx)){
        atomicTick();
    }
}


//--------------------------------------------------
// name: MemoryController::popRequest
// arguments: None
// return value: None
// usage: pop out all requests in DRAM
//--------------------------------------------------

void MemoryController::popRequest()
{
    int i;
    for (i=0; i<m_buffer_num; i++){
        popSPMRequest(i);
    }
	//printf("popRequest finished\n");
}


//--------------------------------------------------
// name: MemoryController::popStallRequest
// arguments: None
// return value: npu_idx
// usage: NPU0~NPU{n-1} 중에서 어느 하나라도 모든 request가 처리되었을 때까지 stall하고 해당 npu_idx를 return (first-touch)
//--------------------------------------------------

uint32_t MemoryController::popStallRequest()
{
    atomic();
	uint32_t idx, endidx;
	bool noreq = true;
	for (idx=0; idx<m_npu_num; idx++){
		//printf("m_no_req of NPU-%d: %d (remain: %d)\n", idx, m_npumemory[idx]->m_no_req, m_npumemory[idx]->m_remain);
		noreq = (noreq & (m_npumemory[idx]->m_no_req));
	}
	if (noreq){
        //printf("No request!\n");
		return ~0;
	}

	for (idx=0; idx<m_npu_num; idx++){
		if (m_npumemory[idx]->m_popped){
			m_npumemory[idx]->m_no_req = true;
			m_npumemory[idx]->m_popped = false;
            //printf("Stalled Returner!\n");
			return idx;
		}
	}

    //atomic ver.
    for (idx=0; idx<m_npu_num; idx++){
        if (checkNPURequest(idx)&&(!(m_npumemory[idx]->m_no_req))){
            endidx = idx;
            break;
        }
    }
    atomicTick();

    if (idx != m_npu_num){
	    for (idx=0; idx<m_npu_num; idx++){
		    if (checkNPURequest(idx)&&(!(m_npumemory[idx]->m_no_req))){
			    m_npumemory[idx]->m_popped = true;
		    }
	    }
	    m_npumemory[endidx]->m_popped = false;
	    m_npumemory[endidx]->m_no_req = true;
	    return endidx;
    }else{
        return idx;
    }
}


//--------------------------------------------------
// name: MemoryController::syncTick()
// arguments: curtick (current tick of outside - might be npu), npu_idx
// return value: synchroized tick in NPU clock
// usage: Tick synchronization for memory controller
//--------------------------------------------------

uint64_t MemoryController::syncTick(uint64_t curtick, uint32_t npu_idx)
{
	//printf("Clock synchronization from %ld to %ld\n", m_clock, curtick);
	if (npu_idx == ~0){//it's only for dram clock
    	while (m_clock < curtick){
            atomicTick();
		}
		return m_clock;
	}
	else{
		uint64_t newtick = m_npumemory[npu_idx]->npu2dramTick(curtick);
		while (m_clock < newtick){
            atomicTick();
		}
		return m_npumemory[npu_idx]->dram2npuTick(m_clock);
	}
}


//--------------------------------------------------
// name: MemoryController::getTick
// arguments: npu idx
// return value: tick (npu clock)
// usage: get tick of NPU-memory
//--------------------------------------------------

uint64_t MemoryController::getTick(uint32_t npu_idx)
{
	if (npu_idx == ~0){
		return m_clock;
	}
	else{
		return m_npumemory[npu_idx]->dram2npuTick(m_clock);
	}
}



//--------------------------------------------------
// name: MemoryController::sramRequest()
// arguments: address, data size, npu index (multi-npu), is_write flag
// return value: None
// usage: SPM access request from core simulator
//--------------------------------------------------

void MemoryController::sramRequest(uint64_t addr, uint32_t size, uint32_t npu_idx, bool is_write)
{
    uint64_t exectick = m_npumemory[npu_idx]->spmAccess(addr, m_clock, is_write);
}

//--------------------------------------------------
// name: MemoryController::writeDRAMLog()
// arguments: current tick, access address
// return value: None
// usage: write DRAM log if user wanted
//--------------------------------------------------

void MemoryController::writeDRAMLog(uint64_t curtick, uint64_t addr, uint32_t npu_idx)
{
	if (!(m_memconfig->is_dram_log)){
		return;
	}

	string logname = "/dram.log";
	string fname = m_memconfig->dramoutdir + logname;
	FILE* f;
	if (m_log_activated){
		f = fopen(fname.c_str(), "a");
	}else{
		f = fopen(fname.c_str(), "w");
		fprintf(f, "cycle, address (hex), channel-idx, npu-idx, module-idx\n");
		m_log_activated = true;
	}

    uint32_t module_idx = calcModuleIdx(addr);
	fprintf(f, "%ld, %lx, %d, %d, %d\n", curtick, addr, m_dramsim[module_idx]->getChannel(addr), npu_idx, module_idx); //for HBM2-8GB, it is equal to (addr>>11)&7
	fclose(f);
}


//--------------------------------------------------
// name: MemoryController::writeDRAMReqLog()
// arguments: current tick, access address of request
// return value: None
// usage: write DRAM request log if user wanted
//--------------------------------------------------

void MemoryController::writeDRAMReqLog(uint64_t curtick, uint64_t addr, uint32_t npu_idx)
{
	if (!(m_memconfig->is_dram_log)){
		return;
	}

    string logname = "/dramreq.log";
    string fname = m_memconfig->dramoutdir + logname;
    FILE* f;
    if (m_reqlog_activated){
        f = fopen(fname.c_str(), "a");
    }else{
        f = fopen(fname.c_str(), "w");
        fprintf(f, "cycle, address (hex), channel, npu_idx, module_idx\n");
        m_reqlog_activated = true;
    }

    uint32_t module_idx = calcModuleIdx(addr);
    fprintf(f, "%ld, %lx, %d, %d, %d\n", curtick, addr, m_dramsim[module_idx]->getChannel(addr), npu_idx, module_idx); //for HBM2-8GB, it is equal to (addr>>11)&7
    fclose(f);
}


//--------------------------------------------------
// name: MemoryController::writeTLBLog()
// arguments: None
// return value: None
// usage: write TLB log if user wanted
//--------------------------------------------------

void MemoryController::writeTLBLog()
{
    if (!(m_memconfig->is_dram_log)){
        return;
    }

    int i;
    for (i=0; i<m_npu_num; i++){
        string tlb_logname = "/tlb" + to_string(i);
        string tlb_fname = m_memconfig->dramoutdir + tlb_logname;
        m_npumemory[i]->writeTLBLog(tlb_fname);
    }
}


//--------------------------------------------------
// name: MemoryController::printStat
// arguments: None
// return value: None
// usage: print DRAM and TLB stat as an standard output
//--------------------------------------------------

void MemoryController::printStat()
{
    printf("*** Stat of DRAM ***\n");
    printf("Read Request: %ld\n", m_dramrdaccess);
    printf("Write Request: %ld\n", m_dramwraccess);
    printf("*** End of DRAM Stat ***\n");

    int i;
    for (i=0; i<m_npu_num; i++){
		m_npumemory[i]->printTLBStat();
    }
}


//--------------------------------------------------
// name: MemoryController::tlbFlush
//--------------------------------------------------

void MemoryController::tlbFlush(uint32_t npu_idx)
{
    uint64_t exectick;
    m_npumemory[npu_idx]->tlbFlush(&exectick);
    while (m_clock < exectick){
        atomicTick();
    }
}
