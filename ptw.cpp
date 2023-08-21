#include "ptw.h"


PTWManager::PTWManager(uint32_t ptw_num, uint32_t pt_step_num, uint64_t ptw_latency, bool is_tpreg, uint32_t pagebits)
	: m_ptw_num(ptw_num), m_pt_step_num(pt_step_num), m_ptw_latency(ptw_latency), m_ctr(0), m_step_latency(ptw_latency/pt_step_num), m_pagebits(pagebits)
{
	printf("%d PTWs with miss latency of %ld\n", m_ptw_num, ptw_latency);
	m_ptw_clock = (uint64_t*)calloc(m_ptw_num, sizeof(uint64_t));
	m_npu_partition_upper = NULL;
	m_npu_partition_lower = NULL;
	m_ptw_belong = NULL;
	m_ptw_occupation = NULL;
	m_ptwreqlog = new multimap<uint64_t, PTWLogPacket>;
	if (is_tpreg){
		m_tpreg = (uint16_t*)calloc(m_pt_step_num, sizeof(uint16_t));
	}else{
		m_tpreg = NULL;
	}
	m_return_ptw = false;
	m_history = NULL;
}


//--------------------------------------------------
// name: PTWManager::setPartition
// usage: setup partition for shared PTW
//--------------------------------------------------

void PTWManager::setPartition(uint32_t npu_idx, uint32_t init, uint32_t upper, uint32_t lower, uint32_t npu_num, bool is_return_ptw)
{
	m_npu_num = npu_num;
	m_return_ptw = is_return_ptw;
	if (m_return_ptw){
		m_history = new queue<uint32_t>;
	}

	int i;
	if (!m_npu_partition_upper){
		m_npu_partition_upper = (uint32_t*)calloc(npu_num, sizeof(uint32_t));
	}
	if (!m_npu_partition_lower){
		m_npu_partition_lower = (uint32_t*)calloc(npu_num, sizeof(uint32_t));
	}
	if (!m_ptw_belong){
		m_ptw_belong = (uint32_t*)malloc(m_ptw_num*sizeof(uint32_t));
		for (i=0; i<m_ptw_num; i++){
			m_ptw_belong[i] = ~0;
		}
	}
	if (!m_ptw_occupation){
		m_ptw_occupation = (uint32_t*)calloc(npu_num, sizeof(uint32_t));
	}
	for (i=0; i<init; i++){
		m_ptw_belong[m_ctr+i] = npu_idx;
	}
	m_ptw_occupation[npu_idx] = init;
	if (m_return_ptw){
		m_npu_partition_upper[npu_idx] = m_ptw_num;
		m_npu_partition_lower[npu_idx] = 0;
	}else{
		m_npu_partition_upper[npu_idx] = upper;
		m_npu_partition_lower[npu_idx] = lower;
	}
	m_ctr += init;
}


//--------------------------------------------------
// name: PTWManager::access
// usage: PTW access for TLB miss
//--------------------------------------------------

bool PTWManager::access(uint64_t vpn, uint32_t npu_idx, uint64_t curtick, uint64_t* exectick)
{
	int i;
	//find earliest-finished one
	uint64_t fastest = ~0;
	int local_minima = -1;
	for (i=0; i<m_ptw_num; i++){
		if (fastest > m_ptw_clock[i]){
			if ((!m_ptw_belong) || (m_ptw_belong[i] == npu_idx) || ((m_ptw_occupation[npu_idx] < m_npu_partition_upper[npu_idx]) &&
									((m_ptw_belong[i] == ~0) || (m_ptw_occupation[m_ptw_belong[i]] > m_npu_partition_lower[m_ptw_belong[i]])))){
				//No partitioning (greedy) || belong to current npu || (occupation does not exceed upper bound && (stealed PTW has no belonger || stealing does not violate belonger's lower bound))
				local_minima = i;
				fastest = m_ptw_clock[i];
			}
		}
	}

	m_ptw_clock[local_minima] = MAX(m_ptw_clock[local_minima], curtick);
	PTWLogPacket pkt = {m_ptw_clock[local_minima], local_minima, npu_idx};
	m_ptwreqlog->insert(make_pair(curtick, pkt));
	//TPReg?
	if (m_tpreg){
		uint32_t stopidx = m_pt_step_num;
		for (i=0; i<m_pt_step_num; i++){
			if ((m_tpreg[i] != maskPTN(vpn, m_pt_step_num - i))&&(stopidx == m_pt_step_num)){
				stopidx = i;
			}
			m_tpreg[i] = maskPTN(vpn, m_pt_step_num - i);//cached
		}
		m_ptw_clock[local_minima] += (m_ptw_latency/m_pt_step_num * (m_pt_step_num - stopidx));
	}else{
		m_ptw_clock[local_minima] += m_ptw_latency;
	}
	(*exectick) = m_ptw_clock[local_minima];

	if (m_ptw_belong){
		if (m_ptw_belong[local_minima] != npu_idx){
			m_ptw_occupation[npu_idx]++;
			if (m_ptw_belong[local_minima] != ~0){
				m_ptw_occupation[m_ptw_belong[local_minima]]--;
			}
		}
		m_ptw_belong[local_minima] = npu_idx;
	}

	if (m_return_ptw){
		//check history
		bool history_hetero = false;
		queue<uint32_t>* tmpqueue = new queue<uint32_t>;
		swap(tmpqueue, m_history);
		uint32_t tmpsize = tmpqueue->size();
		for (i=0; i<tmpsize; i++){
			uint32_t popped = tmpqueue->front();
			tmpqueue->pop();
			if (popped != npu_idx){
				history_hetero = true;
			}
			m_history->push(popped);
		}

		if (history_hetero){
			//force to return all occupancy
			for (i=0; i<m_npu_num; i++){
				m_npu_partition_upper[i] = m_ptw_num/m_npu_num;
				m_npu_partition_lower[i] = m_ptw_num/m_npu_num;
			}
		}else{
			//get all occupancy
			for (i=0; i<m_ptw_num; i++){
				m_ptw_belong[i] = npu_idx;
			}
			for (i=0; i<m_npu_num; i++){
				m_ptw_occupation[i] = 0;
			}
			m_ptw_occupation[npu_idx] = m_ptw_num;
		}

		//update history
		if (m_history->size() == m_ptw_num){
			m_history->pop();
		}
		m_history->push(npu_idx);
	}

	return true;
}


//--------------------------------------------------
// name: PTWManager::writePTWLog
// usage: 
//--------------------------------------------------

void PTWManager::writePTWLog(string fname)
{
	string real_fname = fname + "_ptw.log";
	FILE* f = fopen(real_fname.c_str(), "w");
	fprintf(f, "request arrival cycle, request accepted cycle, ptw_idx, npu_idx\n");
	map<uint64_t, PTWLogPacket>::iterator iter;
	for (iter=m_ptwreqlog->begin(); iter!=m_ptwreqlog->end(); iter++){
		fprintf(f, "%ld, %ld, %d, %d\n", iter->first, (iter->second).occupied_tick, (iter->second).ptw_idx, (iter->second).npu_idx);
	}
	fclose(f);
}


//--------------------------------------------------
// name: PTWManager::maskPTN
// NOTE: step starts from '1'
//--------------------------------------------------

uint16_t PTWManager::maskPTN(uint64_t vpn, uint32_t step)
{
	int rightside_offset = (64-m_pagebits)/m_pt_step_num * (step - 1);
	uint64_t mask = ~((~0)<<((64-m_pagebits)/m_pt_step_num));
	return (uint16_t)((vpn>>rightside_offset)&mask);
}
