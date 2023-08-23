#include "spm.h"


ScratchPad::ScratchPad(uint64_t latency, uint32_t capacity, uint32_t wordsize)
    : m_latency(latency), m_wordsize(wordsize)
{
    if (capacity%wordsize){
        printf("Error: invalid word size!\n");
        exit(1);
    }
    m_word_num = capacity/wordsize;
    m_leftover = m_word_num;
    m_spmword = (SPMWord*)malloc(sizeof(SPMWord)*m_word_num);
    int i;
    for (i=0; i<m_word_num; i++){
        m_spmword[i].m_addr = ~0;
        m_spmword[i].m_tick = 0;
        m_spmword[i].m_valid = 0;
        m_spmword[i].m_dirty = 0;
    }
}


//--------------------------------------------------
// name: ScratchPad::access
// arguments: word address, tick, is_write
// return value: updated tick
// usage: data access
//--------------------------------------------------

uint64_t ScratchPad::access(uint64_t addr, uint64_t curtick, bool is_write)
{
    int i;
    for (i=0; i<m_word_num; i++){
        if ((m_spmword[i].m_addr == addr) && (m_spmword[i].m_valid)){
            break;
        }
    }
    if (i == m_word_num){
        printf("Error: the requested address is not fetched in SPM\n");
        return ~0;
    }
    m_spmword[i].m_tick = MAX(curtick, m_spmword[i].m_tick);
    m_spmword[i].m_tick += m_latency;
    if (is_write){
        m_spmword[i].m_dirty = 1;
    }
    return m_spmword[i].m_tick;
}


//--------------------------------------------------
// name: ScratchPad::flush
// arguments: saver of writeback-required word list
// return value: last tick
// usage: flush out data (invalidate all data)
//--------------------------------------------------

uint64_t ScratchPad::flush(list<uint64_t>* wb_list)
{
    uint64_t tick = 0;
    int i;
    for (i=0; i<m_word_num; i++){
        if (m_spmword[i].m_valid){
            if (tick < m_spmword[i].m_tick){//tick update
                tick = m_spmword[i].m_tick;
            }
            if (m_spmword[i].m_dirty){//writeback
                wb_list->push_back(m_spmword[i].m_addr);
            }
        }
        m_spmword[i].m_addr = ~0;
        m_spmword[i].m_tick = 0;
        m_spmword[i].m_valid = 0;
        m_spmword[i].m_dirty = 0;
    }
    m_leftover = m_word_num;
    return tick;
}


//--------------------------------------------------
// name: ScratchPad::syncCycle
// arguments: word address, current tick
// return value: updated tick
// usage: clock synchronization
//--------------------------------------------------

uint64_t ScratchPad::syncCycle(uint64_t addr, uint64_t curtick)
{
    int i;
    for (i=0; i<m_word_num; i++){
        if ((m_spmword[i].m_addr == addr) && (m_spmword[i].m_valid)){
            break;
        }
    }
    if (i == m_word_num){
        printf("Error: the requested address is not fetched in SPM\n");
        return ~0;
    }
    m_spmword[i].m_tick = MAX(curtick, m_spmword[i].m_tick);
    return m_spmword[i].m_tick;
}


//--------------------------------------------------
// name: ScratchPad::receiveTransaction
// arguments: word address, current tick
// return value: updated tick
// usage: receive callback
//--------------------------------------------------

uint64_t ScratchPad::receiveTransaction(uint64_t addr, uint64_t curtick)
{
    if (!m_leftover){
        printf("Error: no capacity left for SPM\n");
        return ~0;
    }

    int idx = m_word_num - m_leftover;
    m_spmword[idx].m_valid = true;
    m_spmword[idx].m_dirty = false;
    m_spmword[idx].m_tick = curtick;
    m_spmword[idx].m_tick += m_latency;
    return m_spmword[idx].m_tick;
}
