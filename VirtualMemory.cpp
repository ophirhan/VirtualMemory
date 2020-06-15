#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#define GET_LINE(VAR, I) (VAR * PAGE_SIZE) + I


uint64_t transVaddrToPaddr(uint64_t virtualAddress);

word_t get_frame(uint64_t papa);

void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(GET_LINE(frameIndex, i), 0);
    }
}

void VMinitialize() {
    clearTable(0);
}


int VMread(uint64_t virtualAddress, word_t *value) {
    uint64_t frame_phys = transVaddrToPaddr(virtualAddress);
    PMread(frame_phys, value);
    return 1;
}

uint64_t transVaddrToPaddr(uint64_t virtualAddress) {
    word_t phys_addr = 0;
    int trim_left = (1LL << OFFSET_WIDTH) - 1;
    for (int i = 0; i < TABLES_DEPTH; ++i) {
        uint64_t papa = phys_addr;
        uint64_t tb_line = virtualAddress >> ((TABLES_DEPTH - i) * OFFSET_WIDTH);
        tb_line = tb_line & trim_left;
        PMread(GET_LINE(phys_addr, tb_line), &phys_addr);
        if (phys_addr == 0) {
            phys_addr = get_frame(papa);
            clearTable(phys_addr);
            PMwrite(GET_LINE(papa, tb_line), phys_addr);
        }
    }
    uint64_t offset = virtualAddress & trim_left;
    uint64_t frame_phys = GET_LINE(phys_addr, offset);
    return frame_phys;
}

word_t get_frame_helper(uint64_t keep,
                        uint64_t cur,
                        int rec_depth,
                        uint64_t virtualAddress,
                        uint64_t *max_page) {
    if(rec_depth >= TABLES_DEPTH){
        return 0;
    }
    bool all_zero = true;
    word_t cur_line = 0;
    for (int i = 0; i < OFFSET_WIDTH; ++i) { // offset width might be larger todo
        PMread(GET_LINE(cur, i), &cur_line);
        if (cur_line != 0) {
            all_zero = false;
            uint64_t frame_addr = get_frame_helper(cur, cur_line,rec_depth + 1, virtualAddress, max_page);
            if (frame_addr != 0) {
                return frame_addr;
            }
        }
    }
    if(all_zero && keep != cur && cur != 0)
    {
        // upadte papa, return cur
    }
    return 0;
}

word_t get_frame(uint64_t keep, uint64_t virtualAddress) {
    uint64_t max_page = 0;
    uint64_t free_frame = get_frame_helper(keep, 0, 0, virtualAddress, &max_page);
    if (free_frame == 0) {
        uint64_t victim = get_evict
        PMevict()
    }
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    uint64_t frame_phys = transVaddrToPaddr(virtualAddress);
    PMwrite(frame_phys, value);
    return 1;
}
