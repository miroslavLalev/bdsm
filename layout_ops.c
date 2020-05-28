#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "layout.h"
#include "mblock.h"
#include "mblock_ops.h"
#include "sblock.h"
#include "sblock_ops.h"
#include "inode.h"
#include "inode_ops.h"
#include "inode_vec.h"
#include "inode_vec_ops.h"

layout layout_create(sblock sb) {
    layout l;
    l.sb = sb;
    l.inode_mb = mblock_vec_create(sb.imap_blocks);
    l.zones_mb = mblock_vec_create(sb.zmap_blocks);
    l.nodes = inode_vec_init(sb.n_inodes);

    size_t i;
    for (i=0; i<sb.n_inodes; i++) {
        inode n; // create empty nodes
        inode_vec_push(&l.nodes, n);
    }

    return l;
}

void layout_drop(layout *l) {
    mblock_vec_drop(&l->inode_mb);
    mblock_vec_drop(&l->zones_mb);
    inode_vec_drop(&l->nodes);
}

size_t layout_size(layout *l) {
    return 1024 + (size_t)l->sb.imap_blocks * MBLOCK_SIZE + (size_t)l->sb.zmap_blocks * MBLOCK_SIZE + (size_t)l->sb.n_inodes * INODE_SIZE;
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
    for (i=0; i<l->sb.n_inodes; i++) {
        inode_bytes nb = inode_encode(inode_vec_get(l->nodes, i));
        memcpy(buf, &nb.data, INODE_SIZE);
        buf += INODE_SIZE;
    }
}

void layout_extend(layout *l, uint8_t *mb_buf) {
    mblock_vec inode_mb = mblock_vec_create(l->sb.imap_blocks);
    mblock_vec zones_mb = mblock_vec_create(l->sb.zmap_blocks);
    inode_vec nodes = inode_vec_init(l->sb.n_inodes);

    size_t i;
    for (i=0; i<inode_mb.size; i++) {
        memcpy(&inode_mb.blocks[i].data, mb_buf, MBLOCK_SIZE);
        mb_buf += MBLOCK_SIZE;
    }
    for (i=0; i<zones_mb.size; i++) {
        memcpy(&zones_mb.blocks[i].data, mb_buf, MBLOCK_SIZE);
        mb_buf += MBLOCK_SIZE;
    }
    for (i=0; i<l->sb.n_inodes; i++) {
        inode_bytes nb;
        memcpy(&nb.data, mb_buf, INODE_SIZE);
        mb_buf += INODE_SIZE;

        inode_vec_push(&nodes, inode_decode(nb));
    }
    inode_vec_drop(&l->nodes);

    l->inode_mb = inode_mb;
    l->zones_mb = zones_mb;
    l->nodes = nodes;
}

layout layout_decode(uint8_t *buf) {
    sblock_bytes sbb;
    memcpy(&sbb.data, buf, 1024);
    buf += 1024;
    sblock sb = sblock_decode(sbb);

    layout l;
    l.sb = sb;
    layout_extend(&l, buf);
    return l;
}
