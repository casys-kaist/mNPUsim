#include "tlb.h"


TLBSet::TLBSet(uint64_t hit_latency, uint64_t miss_latency, uint32_t entrynum, PTWManager* ptw)
	: m_hit_latency(hit_latency), m_miss_latency(miss_latency), m_entry_num(entrynum), m_valid_entry_num(0), m_remaining_zeros(entrynum), m_ptw(ptw)
{
	m_table = (TLBEntry*)malloc(sizeof(TLBEntry)*entrynum);
	int i;
	for (i=0; i<entrynum; i++){
		m_table[i].m_vpn = 0;
		m_table[i].m_tick = 0;
		m_table[i].m_valid = 0;
	}
	m_mrubits = (bool*)calloc(1, entrynum);
}


//--------------------------------------------------
// name: TLBSet::updateMRUbits
// arguments: idx (accessed entry index)
// return value: None
// usage: update MRU-bits (for Bit-PLRU)
//--------------------------------------------------
void TLBSet::updateMRUbits(int idx)
{
	if (idx >= m_entry_num){
		printf("Error: wrong entry index!\n");
		return;
	}
	if (!m_mrubits[idx]){
		if (m_remaining_zeros == 1){//bit flip required
			int i;
			for (i=0; i<m_entry_num; i++){
				m_mrubits[i] = 0;
			}
			m_remaining_zeros = (m_entry_num - 1);
		}else{
			m_remaining_zeros--;
		}
	}
	m_mrubits[idx] = 1;//updates mru-bit
}


//--------------------------------------------------
// name: TLBSet::evict
// arguments: None
// return value: m_table index for evicted entry
// usage: Entry eviction
//--------------------------------------------------
int TLBSet::evict()
{
	if (m_entry_num == 1){//direct-mapped
		return 0;
	}

	//return the index of evicted entry
	if (m_valid_entry_num < m_entry_num)
		return -1;

	int i;
	for (i=0; i<m_entry_num; i++){
		if (!m_mrubits[i])
			break;
	}
	if (i==m_entry_num){//all-one mru-bits
		printf("Error: the MRU bit was not updated correctly!\n");
		return -1;
	}

	//printf("Evicted entry index: %d\n", i);//for debugging set-associative TLB
	return i;
};


//--------------------------------------------------
// name: TLBSet::access
// arguments: vpn (virtual page number), curtick (access tick), exectick (saver of execution tick)
// return value: (TLB hit) ? true : false
// usage: TLB entry access
//--------------------------------------------------

bool TLBSet::access(uint64_t vpn, uint64_t curtick, uint64_t* exectick, uint32_t npuidx)
{
	int i;
	for (i=0; i<m_entry_num; i++){
		if ((m_table[i].m_vpn == vpn) && (m_table[i].m_valid) && (m_table[i].m_npuidx == npuidx)){//hit
			updateMRUbits(i);
			(*exectick) = MAX(curtick, m_table[i].m_tick) + m_hit_latency;
			return true;
		}
	}
	//miss: need PTW
	int idx;
	if (m_valid_entry_num < m_entry_num){//cold miss
		idx = m_valid_entry_num;
		updateMRUbits(idx);
		m_table[idx].m_vpn = vpn;
		m_table[idx].m_npuidx = npuidx;
		m_table[idx].m_valid = true;
		m_valid_entry_num++;
	}else{//eviction required
		idx = evict();
		updateMRUbits(idx);
		m_table[idx].m_vpn = vpn;
		m_table[idx].m_npuidx = npuidx;
		m_table[idx].m_valid = true;
	}
	if (m_ptw){
		m_ptw->access(vpn, npuidx, MAX(curtick, m_table[idx].m_tick) + m_hit_latency, exectick);
		m_table[idx].m_tick = (*exectick);
	}else{//L1
		(*exectick) = MAX(curtick, m_table[idx].m_tick) + m_hit_latency;
		m_table[idx].m_tick = (*exectick);
		m_table[idx].m_tick += m_miss_latency;
	}
	return false;
}


//--------------------------------------------------
// name: TLBSet::flush
// arguments: curtick
// return value: None
// usage: TLB set flush for specific npu idx
//--------------------------------------------------

void TLBSet::flush(uint32_t npu_idx, uint64_t* exectick)
{
	int i;
	(*exectick) = 0;
	map<int, TLBEntry>* flushtmp = new map<int, TLBEntry>;
	bool* mrubits = (bool*)malloc(sizeof(bool)*m_entry_num);
	for (i=0; i<m_entry_num; i++){
		mrubits[i] = 0;
	}
	int flushed = 0;
	for (i=0; i<m_entry_num; i++){
		if (m_table[i].m_npuidx == npu_idx){
			(*exectick) = MAX((*exectick), m_table[i].m_tick);
			flushed++;
			mrubits[i] = 0;
		}else{//save activated entry
			TLBEntry tmpentry = {m_table[i].m_vpn, m_table[i].m_tick, m_table[i].m_npuidx, m_table[i].m_valid};
			flushtmp->insert(make_pair(i, tmpentry));
			mrubits[i] = m_mrubits[i];
		}
	}
	map<int, TLBEntry>::iterator iter = flushtmp->begin();
	i = 0;
	for (iter=flushtmp->begin(); iter!=flushtmp->end(); iter++){
		m_table[i] = iter->second;
		m_mrubits[i] = mrubits[iter->first];
		i++;
	}
	m_valid_entry_num = m_entry_num - flushed;
	for (i=m_valid_entry_num; i<m_entry_num; i++){
		m_table[i].m_vpn = 0;
		m_table[i].m_tick = 0;
		m_table[i].m_valid = 0;
		m_mrubits[i] = 0;
	}
}


//TLB
TLB::TLB(uint64_t hit_latency, uint64_t miss_latency, uint32_t assoc, uint32_t entrynum, uint32_t pagebits, bool tlb_log, uint8_t pref_mode, int portnum, PTWManager* ptw)
	: m_assoc(assoc), m_entry_num(entrynum), m_page_bits(pagebits), m_access(0), m_miss(0), m_pref_mode(pref_mode), m_portnum(portnum), m_ptw(ptw)
{
	printf("TLB hit latency: %ld / miss latency: %ld / bandwidth: %d access per cycle\n", hit_latency, miss_latency, m_portnum);
	if (entrynum%assoc){
		printf("Error: invalid entry number and/or set associativity (%d entries, %d way)\n", entrynum, assoc);
		exit(1);
	}

	m_set_num = entrynum/assoc;

	m_sets = (TLBSet**)malloc(sizeof(TLBSet*)*m_set_num);
	int i;
	for (i=0; i<m_set_num; i++){
		m_sets[i] = new TLBSet(hit_latency, miss_latency, assoc, ptw);
	}

	m_set_bits = (uint32_t)log2(m_set_num);
	m_vpn_bitmask = (uint64_t)(~0)<<(m_page_bits + m_set_bits);
	m_set_bitmask = ((uint64_t)(~0)<<(m_page_bits)) & (~m_vpn_bitmask);
	printf("vpn_bitmask: 0x%lx\n", m_vpn_bitmask);
	printf("set_bitmask: 0x%lx\n", m_set_bitmask);

	if (tlb_log){
		m_tlbreqlog = new multimap<uint64_t, TLBLogPacket>;
	}else{
		m_tlbreqlog = NULL;
	}

	m_port = new multiset<uint64_t>;
}


//--------------------------------------------------
// name: TLB::setupTranslator
// argument: allocator pointer, pmem capacity
// return value: None
// usage: followed by TLB constructor
//--------------------------------------------------

void TLB::setupTranslator(DRAMAllocator* allocator, uint64_t pmem_capacity, uint64_t firstaddr)
{
	uint32_t page_gran = (uint32_t)pow(2, m_page_bits);
	if (allocator){
		m_translator = new AddressTranslator(page_gran, pmem_capacity, allocator);
	}else{
		m_translator = new AddressTranslator(page_gran, pmem_capacity, firstaddr);
	}
}


//--------------------------------------------------
// name: TLB::calcVPN
// argument: (virtual) memory address
// return value: virtual page number
// usage: calculate virtual page number of given memory address
//--------------------------------------------------

uint64_t TLB::calcVPN(uint64_t addr)
{
	uint64_t vpn_raw = (addr & m_vpn_bitmask)>>(m_page_bits + m_set_bits);//arithmetic right-shift
	uint64_t logical_bitmask = ~((uint64_t)(~0)<<(64 - m_page_bits - m_set_bits));
	return (vpn_raw) & (logical_bitmask);
}


//--------------------------------------------------
// name: TLB::calcSetIdx
// argument: (virtual) memory address
// return value: set index
// usage: calcuate TLB setindex of given memory address
//--------------------------------------------------

uint64_t TLB::calcSetIdx(uint64_t addr)
{
	return (addr & m_set_bitmask)>>m_page_bits;
};


//--------------------------------------------------
// name: TLB::access
// argument:
// return value: Hit or miss?
// usage: TLB access
//--------------------------------------------------

bool TLB::access(uint64_t addr, uint64_t curtick, uint64_t* exectick, uint64_t* paddr_saver, uint32_t npuidx)
{
	//address translation
	m_translator->translate(addr, paddr_saver);
	//printf("0x%lx -> 0x%lx (NPU-%d)\n", addr, *paddr_saver, npuidx);

	if (!m_entry_num){
		(*exectick) = curtick;
		return false;//Always miss!
	}

	uint64_t tmptick = curtick;
	if (m_portnum > 0){
		while (m_port->count(tmptick) == m_portnum){
			tmptick++;
		}
		m_port->insert(tmptick);
	}

	uint64_t vpn = calcVPN(addr);
	int setidx = (int)calcSetIdx(addr);
	if (setidx >= m_set_num){
		printf("Error: invalid set idx %d\n", setidx);
		return false;
	}

	bool is_hit = m_sets[setidx]->access(vpn, tmptick, exectick, npuidx);
	if (!is_hit){
		//prefetch(addr, curtick, exectick, npuidx);
		//printf("Miss at NPU-%d, address 0x%lx, tick %ld\n", npuidx, addr, *exectick);
		m_miss++;
	}
	m_access++;

	if (m_tlbreqlog){
		TLBLogPacket pkt = {addr, *paddr_saver, npuidx, !is_hit};
		m_tlbreqlog->insert(make_pair(tmptick, pkt));//submission time
	}

	return is_hit;
}


//--------------------------------------------------
// name: TLB::flush
// usage: TLB flush
//--------------------------------------------------

void TLB::flush(uint32_t npu_idx, uint64_t* exectick)
{
	uint64_t tmptick = 0;
	(*exectick) = 0;
	int i;
	for (i=0; i<m_set_num; i++){
		m_sets[i]->flush(npu_idx, &tmptick);
		(*exectick) = MAX(*exectick, tmptick);
	}
}


//--------------------------------------------------
// name: TLB::writeLog()
// arguments: log file name
// return value: None
// usage: subroutine of MemoryController::writeLog(), writing TLB request log
//--------------------------------------------------

void TLB::writeLog(string fname)
{
	if (!m_tlbreqlog){
		return;
	}
	string real_fname = fname + ".log";
	FILE* f = fopen(real_fname.c_str(), "w");
	fprintf(f, "cycle, virtual address (hex), physical address (hex), npu_idx, miss?\n");
	map<uint64_t, TLBLogPacket>::iterator iter;
	for (iter=m_tlbreqlog->begin(); iter!=m_tlbreqlog->end(); iter++){
		fprintf(f, "%ld, %lx, %lx, %d, %d\n", iter->first, (iter->second).addr, (iter->second).paddr, (iter->second).npu_idx, (iter->second).miss);
	}
	fclose(f);

	if (m_ptw){
		m_ptw->writePTWLog(fname);
	}
}


//--------------------------------------------------
// name: TLB::prefetch()
// arguments: current miss address, outside access time tick, post-execution tick saver
// return value: None
// usage: subroutine of TLB access, for prefetching
//--------------------------------------------------

void TLB::prefetch(uint64_t miss_addr, uint64_t curtick, uint64_t* exectick, uint32_t npuidx)
{
	if (!m_pref_mode){//No prefetching
		return;
	}
	else if (m_pref_mode == 1){//SP
		uint64_t addr = miss_addr + (1<<m_page_bits);
		uint64_t vpn = calcVPN(addr);
		int setidx = (int)calcSetIdx(addr);
		m_sets[setidx]->access(vpn, curtick, exectick, npuidx);
	}
	else{
		printf("Fatal: Unsupported prefetching mode!\n");
	}
}
