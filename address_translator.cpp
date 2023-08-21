#include "address_translator.h"

//DRAMAllocator
DRAMAllocator::DRAMAllocator(uint32_t page_gran, uint64_t pmem_capacity, uint64_t firstpage_addr)
	: m_pmem_capacity(pmem_capacity), m_pmem_left(pmem_capacity), m_page_gran(page_gran), m_firstpage_addr(firstpage_addr)
{
	assert(!(pmem_capacity%page_gran));

	m_free_list = new queue<uint64_t>;
	m_free_pages = new set<uint64_t>;
	uint64_t page_addr;
	for (page_addr = firstpage_addr; page_addr<firstpage_addr + pmem_capacity; page_addr+=page_gran){
		m_free_list->push(page_addr);
		m_free_pages->insert(page_addr);
	}
	printf("Page address: 0x%lx ~ 0x%lx\n", firstpage_addr, firstpage_addr + pmem_capacity);
}


//--------------------------------------------------
// name: DRAMAllocator::allocate()
// arguments: physical address saver
// return value: (Allocation succeed) ? true : false
// usage: allocate new physical page
//--------------------------------------------------

bool DRAMAllocator::allocate(uint64_t* p_addr_saver)
{
	if (!m_pmem_left){
		(*p_addr_saver) = ~0;
		return false;
	}
	(*p_addr_saver) = m_free_list->front();
	m_free_list->pop();
	m_free_pages->erase(*p_addr_saver);
	m_pmem_left -= m_page_gran;
	return true;
}


//--------------------------------------------------
// name: DRAMAllocator::free()
// arguments: physical address
// return value: (Freeing succeed) ? true : false
// usage: deallocate physical page
//--------------------------------------------------

bool DRAMAllocator::free(uint64_t p_addr)
{
	if (m_pmem_left == m_pmem_capacity){
		return false;
	}
	if ((m_free_pages->find(p_addr))!=(m_free_pages->end())){//Already freed page
		return false;
	}
	m_free_list->push(p_addr);
	m_free_pages->insert(p_addr);
	m_pmem_left += m_page_gran;
	return true;
}


//AddressTranslator
AddressTranslator::AddressTranslator(uint32_t page_gran, uint64_t pmem_capacity, uint64_t first_address)
	: m_pmem_capacity(pmem_capacity), m_page_gran(page_gran)
{
	m_allocator = new DRAMAllocator(page_gran, pmem_capacity, first_address);
	m_translation_table = new map<uint64_t, uint64_t>;
	m_inverse_table = new map<uint64_t, uint64_t>;
}


//Another version of constructor (for multi-tenant NPUs)
AddressTranslator::AddressTranslator(uint32_t page_gran, uint64_t pmem_capacity, DRAMAllocator* allocator)
	: m_pmem_capacity(pmem_capacity), m_page_gran(page_gran), m_allocator(allocator)
{
	m_translation_table = new map<uint64_t, uint64_t>;
	m_inverse_table = new map<uint64_t, uint64_t>;
}


//--------------------------------------------------
// name: AddressTranslator::allocate()
// arguments: v_addr (virtual page address)
// return value: (Allocation succeed) ? true : false
// usage: map virtual page address to physical page address (allocation only)
//--------------------------------------------------

bool AddressTranslator::allocate(uint64_t v_addr)
{
	uint64_t p_addr = ~0;
	if (m_allocator->allocate(&p_addr)){
		m_translation_table->insert(make_pair(v_addr, p_addr));
		m_inverse_table->insert(make_pair(p_addr, v_addr));
		return true;
	}else{//Allocation failed
		return false;
	}
}


//--------------------------------------------------
// name: AddressTranslator::free()
// arguments: virtual page address
// return value: (Deallocation succeed) ? true : false
// usage: demap mapping of given virtual address
//--------------------------------------------------

bool AddressTranslator::free(uint64_t v_addr)
{
	map<uint64_t, uint64_t>::iterator iter = m_translation_table->find(v_addr);
	if (iter == m_translation_table->end()){//Already freed
		return false;
	}
	m_allocator->free(iter->second);
	m_translation_table->erase(v_addr);
	return true;
}


//--------------------------------------------------
// name: AddressTranslator::translate()
// arguments: virtual page address, physical page address saver
// return value: 0 for successful translation, 1 for new allocation, -1 for failure
// usage: translate virtual page address to physical page address, allocate new physical page if needed
//--------------------------------------------------

int AddressTranslator::translate(uint64_t v_addr, uint64_t* p_addr_saver)
{
	map<uint64_t, uint64_t>::iterator iter = m_translation_table->find(v_addr);
	if (iter == m_translation_table->end()){//Need allocation
		if (m_allocator->allocate(p_addr_saver)){
			m_translation_table->insert(make_pair(v_addr, *p_addr_saver));
			return 1;
		}else{
			return -1;
		}
	}
	(*p_addr_saver) = iter->second;
	return 0;
}


//--------------------------------------------------
// name: AddressTranslator::inverse()
// arguments: physical page address, virtual page address saver
// return value: 0 for successful translation, -1 for failure
// usage: inverse address translation (physical page address -> virtual page address)
//--------------------------------------------------

int AddressTranslator::inverse(uint64_t p_addr, uint64_t* v_addr_saver)
{
	map<uint64_t, uint64_t>::iterator iter = m_inverse_table->find(p_addr);
	if (iter == m_translation_table->end()){
		(*v_addr_saver) = ~0;
		return -1;
	}
	(*v_addr_saver) = iter->second;
	return 0;
}
