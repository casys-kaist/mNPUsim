#include "DRAMsim3/src/dramsim3.h"
#include "spm.h"
#include "tlb.h"

#ifndef __MEMCTRL_H__
#define __MEMCTRL_H__

using namespace dramsim3;

struct DRAMPacket
{
    uint32_t npu_idx;
	uint32_t spm_idx;
    uint32_t size;
	bool is_write;
    uint64_t paddr;
    uint64_t page_addr;
	uint64_t vaddr;
};


struct NPUMemConfig //per-NPU
{
	uint64_t tlb_hit_latency = 5;//cycle
	uint64_t tlb_miss_latency = 400;//cycle
	uint32_t tlb_assoc = 1;//Size of set (i.e. way)
	uint32_t tlb_entrynum = 2048;//setnum = entrynum/assoc
    uint32_t tlb_portnum = 4;
	uint32_t npu_clockspeed = 1;
	uint32_t dram_clockspeed = 1;
    uint32_t ptw_num = 8;
    uint32_t pt_step_num = 4;
	uint64_t spm_latency = 1;//cycle
	uint32_t spm_size = 1048576;//B
	uint32_t spm_wordsize = 64;//B
    uint32_t token_bucket = 256;
    uint32_t token_epoch = 2;
	uint8_t tlb_pref_mode = 0;//0 for no pref, 1 for SP, 2 for ASP, 3 for ...
	bool is_double_buffer = true;
    bool is_tpreg = false;
};


struct MemoryConfig //shared
{
    string dramconfig = "dram_config/HBM2_8Gb_x128.ini";
    string dramoutdir = "dramsim_output";
	uint64_t dram_capacity = 2l<<33;//8GB -> 2^33B. Not 21, 2L!)
    uint32_t npu_num = 1;
    uint32_t dram_unit = 64;//B
	uint32_t pagebits = 12;
    uint32_t channel_num = 8;
    uint32_t module_num = 1;
    bool is_dram_log = true;
	bool is_dynamic_partition = true;
    bool is_shared_tlb = false;
    bool is_shared_ptw = false;
    bool is_tiled_bw = false;
    bool is_token_bw = false;
    bool is_conserv_token = false;
    bool is_return_ptw = false;
};


class NPUMemory
{//synchronized with DRAM tick
	private:
		TLB* m_tlb;
		ScratchPad* m_spm0;
		ScratchPad* m_spm1;
        multimap<uint64_t, DRAMPacket>* m_tlbqueue;
        int* m_token;
		NPUMemConfig m_npumemconfig;
        uint64_t m_offset;
        int m_token_thres;
        int m_token_step;
        int m_total_channel_num;
		int m_npu_idx;
		int m_spm0_idx;
		int m_spm1_idx;
        uint32_t m_spm0_req;
        uint32_t m_spm1_req;
		bool m_second_buffer;//set true: SPM0 is read, SPM1 is write buffer
        bool m_shared_tlb;

	public:
		uint64_t m_remain;
		bool m_popped;
		bool m_no_req;
        bool m_is_begin;
		NPUMemory(NPUMemConfig config, uint32_t pagebits, DRAMAllocator* allocator, uint64_t dram_capacity, bool fake_mode, bool is_log, int npu_idx, int total_npunum, PTWManager* ptw);//base
        NPUMemory(NPUMemConfig config, uint32_t pagebits, uint64_t dram_capacity, bool fake_mode, bool is_log, int npu_idx, int total_npunum, TLB* shared_tlb);//shared single-level TLB
        void ptwSetup(int walker_init, int walker_upper, int walker_lower, int npu_num, bool is_return_ptw){m_tlb->setPartition(m_npu_idx, walker_init, walker_upper, walker_lower, npu_num, is_return_ptw);};
		void tlbAccess(uint64_t page_addr, uint64_t curtick, uint64_t* exectick, uint64_t* paddr);
		void writeTLBLog(string tlb_fname);
		void receiveTransaction(uint64_t addr, uint64_t curtick, int buffer_idx);
		uint64_t spmAccess(uint64_t addr, uint64_t curtick, bool is_write);
		void printTLBStat();
		uint32_t calcSPMidx(bool is_write){return (is_write == m_second_buffer)? m_spm1_idx : m_spm0_idx;};
		void flipBuffer(){m_second_buffer = !m_second_buffer;};
		uint32_t getWordSize(){return m_npumemconfig.spm_wordsize;};
		uint32_t getNPUClockSpeed(){return m_npumemconfig.npu_clockspeed;};
		uint32_t getDRAMClockSpeed(){return m_npumemconfig.dram_clockspeed;};
		uint64_t npu2dramTick(uint64_t npu_clock);
        uint64_t dram2npuTick(uint64_t dram_clock);
        void tlbFlush(uint64_t* exectick);
        void insertTLBQueue(uint64_t curtick, DRAMPacket dpacket){m_tlbqueue->insert(make_pair(curtick, dpacket));};
        void getTLBQueueHead(uint64_t* tick_saver, DRAMPacket* packet_saver);
        bool isTLBQueueEmpty(){return m_tlbqueue->begin() == m_tlbqueue->end();}
        void inverseTran(uint64_t paddr, uint64_t* vaddr_saver){m_tlb->inverseTran(paddr, vaddr_saver);};
        void setTokenThreshold(int total_channel_num);
        bool tryConsumeToken(int total_channel_idx);
        void tryAddToken();
        void increaseReq(uint32_t spm_idx){(m_spm0_idx == spm_idx)? m_spm0_req++ : m_spm1_req++;};
        void decreaseReq(uint32_t spm_idx){(m_spm0_idx == spm_idx)? m_spm0_req-- : m_spm1_req--;};
        bool isReqEmpty(uint32_t spm_idx){return ((m_spm0_idx == spm_idx)? (!m_spm0_req) : (!m_spm1_req));};
        int getTokenStep(){return m_token_step;};
};


class MemoryController
{
    private:
        MemoryConfig* m_memconfig;
		NPUMemory** m_npumemory;
        MemorySystem** m_dramsim;
		DRAMAllocator* m_pmem_allocator;
        TLB* m_shared_l1;
        PTWManager* m_ptw;
        list<uint64_t>** m_reqs;
        multimap<uint64_t, DRAMPacket>* m_tlbqueue;
        multimap<uint64_t, DRAMPacket>* m_pagequeue;//<dram clock tick, paddr (npu_idx, size, addr)>. multimap for repetition of tick.
        multimap<uint64_t, uint64_t>* m_dramlog;//<dram clock tick, addr>
		set<uint32_t>* m_finished_set;
        uint64_t m_clock;
        uint64_t m_dramrdtraffic;
        uint64_t m_dramwrtraffic;
        uint64_t m_dramrdaccess;
        uint64_t m_dramwraccess;
		uint64_t m_addr_ptr;
        uint32_t m_pagesize;
		uint32_t m_pagesize_lm;
        uint32_t m_npu_num;
        uint32_t m_buffer_num;
        uint32_t m_dram_unit;
        uint32_t m_module_num;
        uint32_t m_cur_buffer_idx;
		bool m_fake_mode;//decoupling SPM-core timing and DRAM timing
		bool m_log_activated;
        bool m_reqlog_activated;

    public:
        MemoryController(MemoryConfig memconfig);
        void setConfig(MemoryConfig config) {(*m_memconfig) = config;};
		void npumemSetup(NPUMemConfig config, int npu_idx);
        void ptwSetup(int* walker_init, int* walker_upper, int* walker_lower);
        void dramSetup(const string &dramconfig, const string &dramoutdir, function<void(uint64_t)>cb_func);
        void getDRAMsim(MemorySystem** dramsim_saver, int module_idx) {(*dramsim_saver) = m_dramsim[module_idx];};
        void callback(uint64_t addr);
		void flipBuffer(uint32_t npu_idx) {m_npumemory[npu_idx]->flipBuffer();};
        void dramRequest(map<uint64_t, int>* requests, bool is_write, uint32_t npu_idx);
		void tlbRequest(uint64_t addr, uint32_t size, uint32_t npu_idx, uint32_t spm_idx, bool is_write);
        uint32_t calcModuleIdx(uint64_t p_addr);
        uint64_t calcModuleAddr(uint64_t p_addr);
        int calcTotalChannel(uint64_t waddr){return calcModuleIdx(waddr)*(m_memconfig->channel_num) + m_dramsim[calcModuleIdx(waddr)]->getChannel(waddr);};
        void clockTick();
        void atomic();
        void atomicTick(){atomic(); clockTick();};
        bool tryBorrowToken(uint32_t npu_idx, int total_chan);
        bool checkSPMRequest(uint32_t spm_idx);
        void popSPMRequest(uint32_t spm_idx);
        bool checkNPURequest(uint32_t npu_idx);
        void popNPURequest(uint32_t npu_idx);
        void popRequest();
		uint32_t popStallRequest();
        uint32_t getWordSize(int npu_idx){return m_npumemory[npu_idx]->getWordSize();};
        uint32_t getPageSize(){return m_pagesize;};
        uint64_t syncTick(uint64_t curtick, uint32_t npu_idx);
        uint64_t getTick(uint32_t npu_idx);
        void sramRequest(uint64_t addr, uint32_t size, uint32_t npu_idx, bool is_write);
		void writeDRAMLog(uint64_t curtick, uint64_t addr, uint32_t npu_idx);
		void writeDRAMReqLog(uint64_t curtick, uint64_t addr, uint32_t npu_idx);
        void writeTLBLog();
        void printStat();
		void markFinished(uint32_t npu_idx){m_finished_set->insert(npu_idx);};
		uint64_t npu2dramTick(uint64_t tick, uint32_t npu_idx){return m_npumemory[npu_idx]->npu2dramTick(tick);};
        void tlbFlush(uint32_t npu_idx);
        void inverseTran(uint32_t npu_idx, uint64_t paddr, uint64_t* vaddr_saver){m_npumemory[npu_idx]->inverseTran(paddr, vaddr_saver);};
};


void readMemConfig(MemoryConfig* memconfig, char* file_name);
void readNPUMemConfig(NPUMemConfig* config, char* file_name);

#endif
