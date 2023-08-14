#include "common.h"

#ifndef __SPM_H__
#define __SPM_H__


struct SPMWord
{
    uint64_t m_addr;
    uint64_t m_tick;
    bool m_valid;
    bool m_dirty;
};


class ScratchPad
{
    private:
        SPMWord* m_spmword;
        uint64_t m_latency;
        uint32_t m_word_num;
        uint32_t m_leftover;
        uint32_t m_wordsize;//bytes

    public:
        ScratchPad(uint64_t latency, uint32_t capacity, uint32_t wordsize);
        uint64_t access(uint64_t addr, uint64_t curtick, bool is_write);
        uint64_t flush(list<uint64_t>* wb_list);
        uint64_t syncCycle(uint64_t addr, uint64_t curtick);
        uint64_t receiveTransaction(uint64_t addr, uint64_t curtick);
};


#endif
