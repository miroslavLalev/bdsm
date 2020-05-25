#include <stdint.h>
#include <stdlib.h>

#include "sblock.h"

sblock_bytes encode_sblock(sblock s) {
    sblock_bytes sbb;
    sbb.data[0] = (SBLOCK_FS_NUM >> 8) & 0xFF;
    sbb.data[1] = SBLOCK_FS_NUM & 0xFF;

    uint32_t n_inodes = s.n_inodes;
    sbb.data[2] = (n_inodes >> 24) & 0xFF;
    sbb.data[3] = (n_inodes >> 16) & 0xFF;
    sbb.data[4] = (n_inodes >> 8) & 0xFF;
    sbb.data[5] = n_inodes & 0xFF;

    uint16_t imap_blocks = s.imap_blocks;
    sbb.data[6] = (imap_blocks >> 8) & 0xFF;
    sbb.data[7] = imap_blocks & 0xFF;

    uint16_t zmap_blocks = s.zmap_blocks;
    sbb.data[8] = (zmap_blocks >> 8) & 0xFF;
    sbb.data[9] = zmap_blocks & 0xFF;

    uint16_t first_data_zone = s.first_data_zone;
    sbb.data[10] = (first_data_zone >> 8) & 0xFF;
    sbb.data[11] = first_data_zone & 0xFF;

    uint64_t max_size = s.max_size;
    sbb.data[12] = (max_size >> 56) & 0xFF;
    sbb.data[13] = (max_size >> 48) & 0xFF;
    sbb.data[14] = (max_size >> 40) & 0xFF;
    sbb.data[15] = (max_size >> 32) & 0xFF;
    sbb.data[16] = (max_size >> 24) & 0xFF;
    sbb.data[17] = (max_size >> 16) & 0xFF;
    sbb.data[18] = (max_size >> 8) & 0xFF;
    sbb.data[19] = max_size & 0xFF;

    uint64_t zones = s.zones;
    sbb.data[20] = (zones >> 56) & 0xFF;
    sbb.data[21] = (zones >> 48) & 0xFF;
    sbb.data[22] = (zones >> 40) & 0xFF;
    sbb.data[23] = (zones >> 32) & 0xFF;
    sbb.data[24] = (zones >> 24) & 0xFF;
    sbb.data[25] = (zones >> 16) & 0xFF;
    sbb.data[26] = (zones >> 8) & 0xFF;
    sbb.data[27] = zones & 0xFF;

    uint16_t block_size = s.block_size;
    sbb.data[28] = (block_size >> 8) & 0xFF;
    sbb.data[29] = block_size & 0xFF;
    return sbb;
}

sblock decode_sblock(sblock_bytes sbb) {
    sblock s;
    s.fs_num = sbb.data[1] | ((uint16_t)sbb.data[0] << 8);
    s.n_inodes = sbb.data[5] | ((uint16_t)sbb.data[4] << 8) | ((uint32_t)sbb.data[3] << 16) | ((uint32_t)sbb.data[2] << 24);
    s.imap_blocks = sbb.data[7] | ((uint16_t)sbb.data[6] << 8);
    s.zmap_blocks = sbb.data[9] | ((uint16_t)sbb.data[8] << 8);
    s.first_data_zone = sbb.data[11] | ((uint16_t)sbb.data[10] << 8);
    s.max_size = sbb.data[19] | ((uint16_t)sbb.data[18] << 8) |
                ((uint32_t)sbb.data[17] << 16) | ((uint32_t)sbb.data[16] << 24) |
                ((uint64_t)sbb.data[15] << 32) | ((uint64_t)sbb.data[14] << 40) |
                ((uint64_t)sbb.data[13] << 48) | ((uint64_t)sbb.data[12] << 56);
    s.zones =  sbb.data[27] | ((uint16_t)sbb.data[26] << 8) |
                ((uint32_t)sbb.data[25] << 16) | ((uint32_t)sbb.data[24] << 24) |
                ((uint64_t)sbb.data[23] << 32) | ((uint64_t)sbb.data[22] << 40) |
                ((uint64_t)sbb.data[21] << 48) | ((uint64_t)sbb.data[20] << 56);
    s.block_size = sbb.data[29] | ((uint16_t)sbb.data[28] << 8);
    return s;
}
