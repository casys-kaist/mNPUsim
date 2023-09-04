#include "common.h"
#include "address_translator.h"
#include "ptw.h"

#ifndef __TLB_H__
#define __TLB_H__

struct TLBLogPacket
{
	uint64_t addr;
	uint64_t paddr;
	uint32_t npu_idx;
	bool miss;
};


struct TLBEntry
{
	uint64_t m_vpn;
	uint64_t m_tick;
	uint32_t m_npuidx;//for shared-TLB
	bool m_valid;//for debug
};


class TLBSet
{
	private:
		uint64_t m_hit_latency;
		uint64_t m_miss_latency;//fixed latency for page table walk
		uint32_t m_entry_num;
		uint32_t m_valid_entry_num;
		uint32_t m_remaining_zeros;
		TLBEntry* m_table;
		PTWManager* m_ptw; //shared PTW manager
		bool* m_mrubits; //Replacement policy: Bit-PLRU
		void updateMRUbits(int idx);
		int evict();

	public:
		TLBSet(uint64_t hit_latency, uint64_t miss_latency, uint32_t entrynum, PTWManager* ptw);
		bool access(uint64_t vpn, uint64_t curtick, uint64_t* exectick, uint32_t npuidx);
		void flush(uint32_t npu_idx, uint64_t* exectick);
};


class TLB
{
	private:
		uint64_t m_vpn_bitmask;
		uint64_t m_set_bitmask;
		uint64_t m_access;
		uint64_t m_miss;
		uint32_t m_set_num;
		uint32_t m_assoc;
		uint32_t m_entry_num;
		uint32_t m_page_bits;//page size = 2^(m_page_bits)
		uint32_t m_set_bits;
		uint32_t m_portnum;
		uint8_t m_pref_mode;
		PTWManager* m_ptw;
		TLBSet** m_sets;
		multimap<uint64_t, TLBLogPacket>* m_tlbreqlog;//<tick, addr>
		multiset<uint64_t>* m_port;
		AddressTranslator* m_translator;
		uint64_t calcVPN(uint64_t addr);
		uint64_t calcSetIdx(uint64_t addr);
		void prefetch(uint64_t miss_addr, uint64_t curtick, uint64_t* exectick, uint32_t npuidx);

	public:
		TLB(uint64_t hit_latency, uint64_t miss_latency, uint32_t assoc, uint32_t entrynum, uint32_t pagebits, bool tlb_log, uint8_t pref_mode, int portnum, PTWManager* ptw);
		void setupTranslator(DRAMAllocator* allocator, uint64_t pmem_capacity, uint64_t firstaddr);
		void setPartition(uint32_t npu_idx, uint32_t init, uint32_t upper, uint32_t lower, uint32_t npu_num, bool is_return_ptw){m_ptw->setPartition(npu_idx, init, upper, lower, npu_num, is_return_ptw);};
		bool access(uint64_t addr, uint64_t curtick, uint64_t* exectick, uint64_t* paddr_saver, uint32_t npuidx);
		void translate(uint64_t addr, uint64_t* paddr_saver){m_translator->translate(addr, paddr_saver);};//shadow routine
		void inverseTran(uint64_t paddr, uint64_t* vaddr_saver){m_translator->inverse(paddr, vaddr_saver);};
		void flush(uint32_t npu_idx, uint64_t* exectick);
		uint64_t numAccess(){return m_access;};
		uint64_t numMiss(){return m_miss;};
		void writeLog(string fname);
		DRAMAllocator* getAllocator(){return m_translator->getAllocator();};
		AddressTranslator* getTranslator(){return m_translator;};
		void setTranslator(AddressTranslator* translator){m_translator = translator;};
        void pmemStat(){printf("consumed 0x%lxB out of 0x%lxB\n", m_translator->usedCapacity(), m_translator->pmemCapacity());};
};


#endif
