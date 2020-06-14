#include <stdint.h>
#include <math.h>
#include <stdio.h>

#include "mblock.h"

int rightmost_set_bit(uint8_t val) {
    return (int)log2(val & (~val + 1)) + 1;
}

int rightmost_unset_bit(uint8_t val) {
    if (val==0) {
        return 1;
    }
    if (val == 0xFF) {
        // all bits set
        return -1;
    }
    return rightmost_set_bit(~val);
}

int mblock_take_first(mblock *m) {
    int i;
    for (i=0; i<MBLOCK_SIZE; i++) {
        int r_unset = rightmost_unset_bit(m->data[i]);
        if (r_unset != -1) {
            // set and return
            m->data[i] = (1 << (r_unset-1)) | m->data[i];
            return r_unset * (i+1);
        }
    }
    return -1;
}

void mblock_unset(mblock *m, int n) {
    m->data[n/8] &= ~(1 << ((n % 8) - 1));
}

mblock_vec mblock_vec_create(size_t size) {
    mblock_vec mv;
    mv.blocks = (mblock*)malloc(size*sizeof(mblock));
    mv.size = size;
    return mv;
}

void mblock_vec_drop(mblock_vec* mv) {
    free(mv->blocks);
}

int mblock_vec_take_first(mblock_vec *mv) {
    size_t i;
    for (i=0; i<mv->size; i++) {
        int taken = mblock_take_first(&mv->blocks[i]);
        if (taken != -1) {
            return taken * (i+1);
        }
    }
    return -1;
}

void mblock_vec_unset(mblock_vec *mv, int k) {
    mblock_unset(&mv->blocks[k/mv->size], k%mv->size);
}
