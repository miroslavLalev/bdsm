#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "layout.h"
#include "mblock.h"
#include "mblock_ops.h"
#include "sblock.h"
#include "sblock_ops.h"

layout layout_create(sblock sb) {
    layout l;
    l.sb = sb;
    l.inode_mb = mblock_vec_create(sb.imap_blocks);
    l.zones_mb = mblock_vec_create(sb.zmap_blocks);
    return l;
}

void layout_drop(layout *l) {
    mblock_vec_drop(l->inode_mb);
    mblock_vec_drop(l->zones_mb);
}

size_t layout_size(layout *l) {
    return 1024 + (size_t)l->sb.imap_blocks * MBLOCK_SIZE + (size_t)l->sb.zmap_blocks * MBLOCK_SIZE;
}

void layout_encode(layout *l, uint8_t *buf) {
    sblock_bytes sbb = sblock_encode(l->sb);
    memcpy(buf, &sbb.data, 1024);
    buf += 1024;

    size_t i;
    for (i=0; i<l->inode_mb.size; i++) {
        memcpy(buf, &l->inode_mb.blocks[i].data, MBLOCK_SIZE);
        buf += MBLOCK_SIZE;
    }
    for (i=0; i<l->zones_mb.size; i++) {
        memcpy(buf, &l->zones_mb.blocks[i].data, MBLOCK_SIZE);
        buf += MBLOCK_SIZE;
    }
}

layout layout_decode(uint8_t *buf) {
    sblock_bytes sbb;
    memcpy(&sbb.data, buf, 1024);
    sblock sb = sblock_decode(sbb);

    mblock_vec inode_mb = mblock_vec_create(sb.imap_blocks);
    mblock_vec zones_mb = mblock_vec_create(sb.zmap_blocks);

    size_t i;
    for (i=0; i<inode_mb.size; i++) {
        memcpy(&inode_mb.blocks[i].data, buf, MBLOCK_SIZE);
    }
    for (i=0; i<zones_mb.size; i++) {
        memcpy(&zones_mb.blocks[i].data, buf, MBLOCK_SIZE);
    }
    layout l;
    l.sb = sb;
    l.inode_mb = inode_mb;
    l.zones_mb = zones_mb;
    return l;
}

