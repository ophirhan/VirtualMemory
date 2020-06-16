#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#define GET_LINE(VAR, I) (VAR * PAGE_SIZE) + I
typedef struct Furthest{
    uint64_t page_in;
    uint64_t virt_furthest; // virt addr of victim
    uint64_t orig_ln_of_furthest; // phys addr containing phys addr of furthest page
    word_t phys_furthest;
    word_t distance;
} Furthest;


uint64_t transVaddrToPaddr(uint64_t virtualAddress);


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

word_t get_free_frame(const uint64_t keep, const uint64_t virtualAddress);

uint64_t transVaddrToPaddr(uint64_t virtualAddress) {
    word_t phys_addr = 0;
    int trim_left = (1LL << OFFSET_WIDTH) - 1;
    for (int i = 0; i < TABLES_DEPTH; ++i) {
        uint64_t papa = phys_addr;
        uint64_t tb_line = virtualAddress >> ((TABLES_DEPTH - i) * OFFSET_WIDTH);
        tb_line = tb_line & trim_left;
        PMread(GET_LINE(phys_addr, tb_line), &phys_addr);
        if (phys_addr == 0) {
            phys_addr = get_free_frame(papa, virtualAddress);
            clearTable(phys_addr);
            PMwrite(GET_LINE(papa, tb_line), phys_addr);
        }
    }
    uint64_t offset = virtualAddress & trim_left;
    uint64_t frame_phys = GET_LINE(phys_addr, offset);
    return frame_phys;
}

void update_furthest(uint64_t cur_virt, Furthest &fr, word_t next_frame, uint64_t this_line) {
    // calculate distance
    uint64_t reg_distance = cur_virt - fr.page_in;
    reg_distance = reg_distance > 0 ? reg_distance : -reg_distance;
    uint64_t cyclic_distance = NUM_PAGES - reg_distance;
    uint64_t cur_distance = cyclic_distance > reg_distance ? reg_distance : cyclic_distance;

    // update accordingly
    if(cur_distance > fr.distance){
        fr.distance = cur_distance;
        fr.virt_furthest = cur_virt;
        fr.phys_furthest = next_frame;
        fr.orig_ln_of_furthest = this_line;
    }
}

bool Inspect_leaf(const uint64_t cur_tb, uint64_t cur_virt, uint64_t &max_frame, Furthest &fr, word_t &next_frame) {
    bool empty;
    for (int i = 0; i < PAGE_SIZE; ++i) {// first table might be shorter, that's fine?
        uint64_t this_line = GET_LINE(cur_tb, i);
        PMread(this_line, &next_frame);
        if (next_frame != 0) {
            empty = false;
            update_furthest(cur_virt, fr, next_frame, this_line);
            if(next_frame > max_frame){
                max_frame = next_frame;
            }
        }
    }
    return empty;
}

word_t inspect_table(const uint64_t keep, const uint64_t origin_line, const uint64_t cur_tb,
                     uint64_t cur_virt, const int rec_depth, uint64_t &max_frame, Furthest &fr) {
    bool empty = true;
    word_t next_frame = 0;
    cur_virt = cur_virt << OFFSET_WIDTH;
    if(rec_depth < TABLES_DEPTH){
        for (int i = 0; i < PAGE_SIZE; ++i) {
            uint64_t this_line = GET_LINE(cur_tb, i);
            PMread(this_line, &next_frame);
            if (next_frame != 0) {
                empty = false;
                // bad practice to refactor recursion call to another method.
                uint64_t empty_tb = inspect_table(keep, this_line, next_frame, cur_virt + i,
                                                  rec_depth + 1, max_frame, fr);
                if (empty_tb != 0) {
                    return empty_tb;
                }
                if(next_frame > max_frame){
                    max_frame = next_frame;
                }
            }
        }
    }
    else{
        empty = Inspect_leaf(cur_tb, cur_virt, max_frame, fr, next_frame);
    }
    if(empty && keep != cur_tb)
    {
        PMwrite(origin_line, 0);
        return cur_tb;
    }
    return 0;
}

word_t get_free_frame(const uint64_t keep, const uint64_t virtualAddress) {
    uint64_t max_frame = 0; // max frame in use
    Furthest fr;
    fr.distance = 0;
    fr.page_in = virtualAddress;
    uint64_t empty_tb = inspect_table(keep, 0, 0, 0, 1, max_frame, fr);
    if (empty_tb != 0) {
        return empty_tb;
    }
    if(max_frame + 1 <= RAM_SIZE) { // todo <=?
        return max_frame + 1;
    }
    PMwrite(fr.orig_ln_of_furthest, 0);
    PMevict(fr.phys_furthest, fr.virt_furthest);
    return fr.phys_furthest;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    uint64_t frame_phys = transVaddrToPaddr(virtualAddress);
    PMwrite(frame_phys, value);
    return 1;
}
