#ifndef LAYOUT_H
#define LAYOUT_H

#include "sblock.h"
#include "mblock.h"
#include "inode_vec.h"

struct layout_str {
    sblock sb;
    mblock_vec inode_mb;
    mblock_vec zones_mb;
    inode_vec nodes;

    // TODO: inodes, zones
};

typedef struct layout_str layout;

#endif
