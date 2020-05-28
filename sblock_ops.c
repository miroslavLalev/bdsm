#include <stdint.h>
#include <stdlib.h>

#include "sblock.h"
#include "encutils.h"

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
