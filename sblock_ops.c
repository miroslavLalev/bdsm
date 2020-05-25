#include <stdint.h>
#include <stdlib.h>

#include "sblock.h"

void enc_u16(uint16_t val, uint8_t *data, size_t *offset) {
    size_t ov = *offset;
    data[ov] = (val >> 8) & 0xFF;
    data[ov+1] = val & 0xFF;
    *offset+=2;
}

uint16_t dec_u16(uint8_t *data, size_t *offset) {
    size_t ov = *offset;
    uint16_t res = data[ov+1] | ((uint16_t)data[ov] << 8);
    *offset+=2;
    return res;
}

void enc_u32(uint32_t val, uint8_t *data, size_t *offset) {
    size_t ov = *offset;
    data[ov] = (val >> 24) & 0xFF;
    data[ov+1] = (val >> 16) & 0xFF;
    data[ov+2] = (val >> 8) & 0xFF;
    data[ov+3] = val & 0xFF;
    *offset+=4;
}

uint32_t dec_u32(uint8_t *data, size_t *offset) {
    size_t ov = *offset;
    uint32_t res = data[ov+3] |
                ((uint16_t)data[ov+2] << 8) |
                ((uint32_t)data[ov+1] << 16) |
                ((uint32_t)data[ov] << 24);
    *offset+=4;
    return res;
}

void enc_u64(uint64_t val, uint8_t *data, size_t *offset) {
    size_t ov = *offset;
    data[ov] = (val >> 56) & 0xFF;
    data[ov+1] = (val >> 48) & 0xFF;
    data[ov+2] = (val >> 40) & 0xFF;
    data[ov+3] = (val >> 32) & 0xFF;
    data[ov+4] = (val >> 24) & 0xFF;
    data[ov+5] = (val >> 16) & 0xFF;
    data[ov+6] = (val >> 8) & 0xFF;
    data[ov+7] = val & 0xFF;
    *offset+=8;
}

uint64_t dec_u64(uint8_t *data, size_t *offset) {
    size_t ov = *offset;
    uint64_t res = data[ov+7] |
                ((uint16_t)data[ov+6] << 8) |
                ((uint32_t)data[ov+5] << 16) |
                ((uint32_t)data[ov+4] << 24) |
                ((uint64_t)data[ov+3] << 32) |
                ((uint64_t)data[ov+2] << 40) |
                ((uint64_t)data[ov+1] << 48) |
                ((uint64_t)data[ov] << 56);
    *offset+=8;
    return res;
}

sblock_bytes sblock_encode(sblock s) {
    sblock_bytes sbb;
    size_t offset = 0;
    enc_u16(SBLOCK_FS_NUM, sbb.data, &offset);
    enc_u32(s.n_inodes, sbb.data, &offset);
    enc_u16(s.imap_blocks, sbb.data, &offset);
    enc_u16(s.zmap_blocks, sbb.data, &offset);
    enc_u16(s.first_data_zone, sbb.data, &offset);
    enc_u64(s.max_size, sbb.data, &offset);
    enc_u64(s.zones, sbb.data, &offset);
    enc_u16(s.block_size, sbb.data, &offset);
    return sbb;
}

sblock sblock_decode(sblock_bytes sbb) {
    sblock s;
    size_t offset = 0;
    s.fs_num = dec_u16(sbb.data, &offset);
    s.n_inodes = dec_u32(sbb.data, &offset);
    s.imap_blocks = dec_u16(sbb.data, &offset);
    s.zmap_blocks = dec_u16(sbb.data, &offset);
    s.first_data_zone = dec_u16(sbb.data, &offset);
    s.max_size = dec_u64(sbb.data, &offset);
    s.zones = dec_u64(sbb.data, &offset);
    s.block_size = dec_u16(sbb.data, &offset);
    return s;
}
