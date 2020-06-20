#ifndef SBLOCK_H
#define SBLOCK_H

#include <stdint.h>

#define SBLOCK_SIZE 1024

struct sblock_str {
    uint16_t fs_num;
    uint32_t n_inodes;
    uint16_t imap_blocks;
    uint16_t zmap_blocks;
    uint64_t first_data_zone;
    uint64_t max_size;
    uint64_t zones;
    uint16_t block_size;
};

typedef struct sblock_str sblock;

struct sblock_ret {
    uint8_t data[1024];
};

typedef struct sblock_ret sblock_bytes;

#endif
