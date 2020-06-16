#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#define GET_LINE(VAR, I) (VAR * PAGE_SIZE) + I

typedef uint64_t Address;

typedef struct Furthest{
    Address orig_ln_of_furthest; // phys addr containing phys addr of furthest page
    word_t page_in;
    word_t virt_furthest; // virt index of victim
    word_t phys_furthest;
    int distance;
} Furthest;


Address transVaddrToPaddr(Address v_addr);


void clearTable(Address frameIndex) {
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

word_t get_free_frame(word_t keep, word_t v_idx);

Address transVaddrToPaddr(Address v_addr) {
    word_t p_idx = 0;
    word_t v_idx = v_addr >> OFFSET_WIDTH;
    int trim_left = (1LL << OFFSET_WIDTH) - 1;
    for (int i = 1; i <= TABLES_DEPTH; ++i) {
        word_t papa = p_idx;
        word_t tb_line = v_idx >> ((TABLES_DEPTH - i) * OFFSET_WIDTH);
        tb_line = tb_line & trim_left;
        PMread(GET_LINE(p_idx, tb_line), &p_idx);
        if (p_idx == 0) {
            p_idx = get_free_frame(papa, v_idx);
            if(i == TABLES_DEPTH){
                PMrestore(p_idx, v_idx);
            }
            else{
                clearTable(p_idx);
            }
            PMwrite(GET_LINE(papa, tb_line), p_idx);
        }
    }
    word_t offset = v_addr & trim_left;
    Address frame_phys = GET_LINE(p_idx, offset);
    return frame_phys;
}

void update_furthest(word_t cur_virt, Furthest &fr, word_t frame, Address this_line) {
    // calculate distance
    int reg_distance = cur_virt - fr.page_in;
    reg_distance = reg_distance > 0 ? reg_distance : -reg_distance;
    int cyclic_distance = NUM_PAGES - reg_distance;
    int cur_distance = cyclic_distance > reg_distance ? reg_distance : cyclic_distance;

    // update accordingly
    if(cur_distance > fr.distance){
        fr.distance = cur_distance;
        fr.virt_furthest = cur_virt;
        fr.phys_furthest = frame;
        fr.orig_ln_of_furthest = this_line;
    }
}

bool Inspect_leaf(word_t cur_tb, word_t cur_virt, word_t &max_frame, Furthest &fr) {
    bool empty = true;
    word_t frame;
    for (int i = 0; i < PAGE_SIZE; ++i) {// first table might be shorter, that's fine?
        Address this_line = GET_LINE(cur_tb, i);
        PMread(this_line, &frame);
        if (frame != 0) {
            empty = false;
            update_furthest(cur_virt, fr, frame, this_line);
            if(frame > max_frame){
                max_frame = frame;
            }
        }
    }
    return empty;
}

word_t inspect_table(const word_t keep,
                    Address origin_line,
                    const word_t cur_tb,
                    word_t cur_virt,
                    const int rec_depth,
                    word_t &max_frame,
                    Furthest &fr) {
    bool empty = true;
    if(rec_depth < TABLES_DEPTH){
        cur_virt = cur_virt << OFFSET_WIDTH;
        word_t next_tb;
        for (int i = 0; i < PAGE_SIZE; ++i) {
            Address this_line = GET_LINE(cur_tb, i);
            PMread(this_line, &next_tb);
            if (next_tb != 0) {
                empty = false;
                // bad practice to refactor recursion call to another method.
                word_t empty_tb = inspect_table(keep, this_line, next_tb, cur_virt + i,
                                                  rec_depth + 1, max_frame, fr);
                if (empty_tb != 0) {
                    return empty_tb;
                }
                if(next_tb > max_frame){
                    max_frame = next_tb;
                }
            }
        }
    }
    else{
        empty = Inspect_leaf(cur_tb, cur_virt, max_frame, fr);
    }
    if(empty && keep != cur_tb)
    {
        PMwrite(origin_line, 0);
        return cur_tb;
    }
    return 0;
}

word_t get_free_frame(word_t keep, word_t v_idx) {
    word_t max_frame = 0; // max frame index in use
    Furthest fr;
    fr.distance = 0;
    fr.page_in = v_idx;
    word_t empty_tb = inspect_table(keep, 0, 0,
                                      0,1,max_frame, fr);
    if (empty_tb != 0) {
        return empty_tb;
    }
    if(max_frame + 1 < NUM_FRAMES) {
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
