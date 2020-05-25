#ifndef MBLOCK_H
#define MBLOCK_H

#include <stdlib.h>
#include <stdint.h>

#define MBLOCK_SIZE 1024
#define MBLOCK_ITEMS (MBLOCK_SIZE * 8)

struct mblock_str {
    uint8_t data[MBLOCK_SIZE];
};

typedef struct mblock_str mblock;

struct mblock_vec_str {
    mblock *blocks;
    size_t size;
};

typedef struct mblock_vec_str mblock_vec;

#endif
