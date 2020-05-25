#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "mblock.h"

int rightmost_set_bit(uint8_t val) {
    return (int)log2(val & (-val)) + 1;
}

int rightmost_unset_bit(uint8_t val) {
    if (val==0) {
        return 1;
    }
    if ((val & (val+1)) == 0) {
        // all bits set
        return -1;
    }
    return rightmost_set_bit(~val);
}

int mblock_take_first(mblock *m) {
    if (m==NULL) {
        return -1;
    }

    int i;
    for (i=0; i<MBLOCK_SIZE; i++) {
        int r_unset = rightmost_unset_bit(m->data[i]);
        if (r_unset != -1) {
            // set and return
            m->data[i] = (1 << r_unset) | m->data[i];
            return r_unset * (i+1);
        }
    }
    return -1;
}

void mblock_unset(mblock *m, int n) {
    int byte = n / 8;
    int bit = n % 8;
    m->data[byte] &= ~(1 << (bit-1));
}

mblock_vec mblock_container(size_t size) {
    mblock_vec mv;
    mv.blocks = malloc(size * sizeof(mblock));
    mv.size = size;
    return mv;
}
