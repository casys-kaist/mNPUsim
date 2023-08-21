#include "common.h"

#ifndef __PTW_H__
#define __PTW_H__

struct PTWLogPacket
{
	uint64_t occupied_tick;
	uint32_t ptw_idx;
	uint32_t npu_idx;
};


class PTWManager
{
	private:
		uint64_t* m_ptw_clock;
		uint32_t* m_ptw_belong;
		uint32_t* m_npu_partition_upper;
		uint32_t* m_npu_partition_lower;
		uint32_t* m_ptw_occupation;
		uint16_t* m_tpreg;//NeuMMU. [step4, step3, step2, step1]
		uint64_t m_ptw_latency;
		uint64_t m_step_latency;
		uint32_t m_pagebits;
		uint32_t m_ptw_num;
		uint32_t m_pt_step_num;//just in case we need this
		uint32_t m_ctr;
		uint32_t m_npu_num;
		bool m_return_ptw;
		queue<uint32_t>* m_history;
		multimap<uint64_t, PTWLogPacket>* m_ptwreqlog;//tick, PTW info

	public:
		PTWManager(uint32_t ptw_num, uint32_t pt_step_num, uint64_t ptw_latency, bool is_tpreg, uint32_t pagesize);
		void setPartition(uint32_t npu_idx, uint32_t init, uint32_t upper, uint32_t lower, uint32_t npu_num, bool is_return_ptw);
		bool access(uint64_t vpn, uint32_t npu_idx, uint64_t curtick, uint64_t* exectick);
		void writePTWLog(string fname);
		uint16_t maskPTN(uint64_t vpn, uint32_t step);
		//address format: [Step4][Step3][Step2][Step1][Offset-[pagebits]b]
};


#endif
