#include "common.h"

#ifndef __ADDRESS_TRANSLATOR_H__
#define __ADDRESS_TRANSLATOR_H__

/*Modeling physical memory space (pages)*/
class DRAMAllocator
{
	private:
		queue<uint64_t>* m_free_list;
		set<uint64_t>* m_free_pages;
		uint64_t m_pmem_capacity;
		uint64_t m_pmem_left;
		uint64_t m_firstpage_addr;
		uint32_t m_page_gran;

	public:
		DRAMAllocator(uint32_t page_gran, uint64_t pmem_capacity, uint64_t m_firstpage_addr);
		bool allocate(uint64_t* p_addr_saver);
		bool free(uint64_t p_addr);
		uint64_t pmemCapacity() {return m_pmem_capacity;};
		uint32_t pageSize() {return m_page_gran;};
		uint64_t firstAddr() {return m_firstpage_addr;};
};


class AddressTranslator
{
	private:
		DRAMAllocator* m_allocator;
		map<uint64_t, uint64_t>* m_translation_table;
        map<uint64_t, uint64_t>* m_inverse_table;
		uint64_t m_pmem_capacity;
		uint32_t m_page_gran;

	public:
		AddressTranslator(uint32_t page_gran, uint64_t pmem_capacity, uint64_t first_address);
		AddressTranslator(uint32_t page_gran, uint64_t pmem_capacity, DRAMAllocator* allocator);
        //sharing allocator for multi-tenant NPU, dynamic partitioning
		uint64_t pmemCapacity() {return m_pmem_capacity;};
		uint32_t pageSize() {return m_page_gran;};
		bool allocate(uint64_t v_addr);
		bool free(uint64_t v_addr);
		int translate(uint64_t v_addr, uint64_t* p_addr_saver);
        int inverse(uint64_t p_addr, uint64_t* v_addr_saver);
		DRAMAllocator* getAllocator(){return m_allocator;};
};


#endif
