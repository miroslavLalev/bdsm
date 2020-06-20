#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "layout.h"
#include "mblock.h"
#include "mblock_ops.h"
#include "sblock.h"
#include "sblock_ops.h"
#include "inode.h"
#include "inode_ops.h"
#include "inode_vec.h"
#include "inode_vec_ops.h"

size_t layout_size(sblock sb) {
    return 1024 +
        (size_t)sb.imap_blocks * MBLOCK_SIZE +
        (size_t)sb.zmap_blocks * MBLOCK_SIZE +
        (size_t)sb.n_inodes * sizeof(inode);
}

layout layout_init(sblock sb) {
    layout l;
    l.inode_mb = mblock_vec_create(sb.imap_blocks);
    l.zones_mb = mblock_vec_create(sb.zmap_blocks);
    l.nodes = inode_vec_init(sb.n_inodes);

    inode iroot;
    iroot.mode = 0;
    inode_set_mode(&iroot, M_READ|M_EXEC, M_READ|M_EXEC, M_READ|M_EXEC, M_DIR);
    iroot.nr_links = 1;
    iroot.size = 0;
    iroot.oid = 0;
    iroot.gid = 0;
    iroot.mtime = 0;
    memset(iroot.zones, 0, ZONES_SIZE * sizeof(uint32_t));
    int inr = mblock_vec_take_first(&l.inode_mb);
    assert(inr == 0); // root inode should be the first one
    int znr = mblock_vec_take_first(&l.zones_mb);
    assert(znr == 0); // root zone should be the first one
    iroot.zones[0] = znr+1;
    inode_vec_push(&l.nodes, iroot);

    size_t i;
    for (i=0; i<sb.n_inodes-1; i++) {
        inode n;
        n.mode = 0;
        n.nr_links = 0;
        n.size = 0;
        n.oid = 0;
        n.gid = 0;
        n.mtime = 0;
        memset(n.zones, 0, ZONES_SIZE * sizeof(uint32_t));
        inode_vec_push(&l.nodes, n);
    }

    // set zone location after sizing the layout
    sb.first_data_zone = layout_size(sb);
    l.sb = sb;
    return l;
}

layout layout_recreate(sblock sb) {
    layout l;
    l.sb = sb;
    l.inode_mb = mblock_vec_create(sb.imap_blocks);
    l.zones_mb = mblock_vec_create(sb.zmap_blocks);
    l.nodes = inode_vec_init(sb.n_inodes);
    return l;
}

void layout_drop(layout *l) {
    mblock_vec_drop(&l->inode_mb);
    mblock_vec_drop(&l->zones_mb);
    inode_vec_drop(&l->nodes);
}

void layout_encode(layout *l, uint8_t *buf) {
    sblock_bytes sbb = sblock_encode(l->sb);
    memcpy(buf, sbb.data, 1024);
    buf += 1024;

    size_t i;
    for (i=0; i<l->inode_mb.size; i++) {
        memcpy(buf, l->inode_mb.blocks[i].data, MBLOCK_SIZE);
        buf += MBLOCK_SIZE;
    }
    for (i=0; i<l->zones_mb.size; i++) {
        memcpy(buf, l->zones_mb.blocks[i].data, MBLOCK_SIZE);
        buf += MBLOCK_SIZE;
    }
    for (i=0; i<l->sb.n_inodes; i++) {
        inode_bytes nb = inode_encode(inode_vec_get(l->nodes, i));
        memcpy(buf, nb.data, sizeof(inode));
        buf += sizeof(inode);
    }
}

void layout_extend(layout *l, uint8_t *mb_buf) {
    mblock_vec inode_mb = mblock_vec_create(l->sb.imap_blocks);
    mblock_vec zones_mb = mblock_vec_create(l->sb.zmap_blocks);
    inode_vec nodes = inode_vec_init(l->sb.n_inodes);
    inode_vec_drop(&l->nodes);

    size_t i;
    for (i=0; i<inode_mb.size; i++) {
        memcpy(inode_mb.blocks[i].data, mb_buf, MBLOCK_SIZE);
        mb_buf += MBLOCK_SIZE;
    }
    for (i=0; i<zones_mb.size; i++) {
        memcpy(zones_mb.blocks[i].data, mb_buf, MBLOCK_SIZE);
        mb_buf += MBLOCK_SIZE;
    }
    for (i=0; i<l->sb.n_inodes; i++) {
        inode_bytes nb;
        memcpy(nb.data, mb_buf, sizeof(inode));
        mb_buf += sizeof(inode);

        inode_vec_push(&nodes, inode_decode(nb));
    }

    l->inode_mb = inode_mb;
    l->zones_mb = zones_mb;
    l->nodes = nodes;
}
