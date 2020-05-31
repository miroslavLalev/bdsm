#ifndef LAYOUT_H
#define LAYOUT_H

#include "sblock.h"
#include "mblock.h"
#include "inode_vec.h"

struct layout_str {
    // the super block represents static information about
    // the underlying file system
    sblock sb;

    // inode map block vector
    mblock_vec inode_mb;

    // zones map block vector
    mblock_vec zones_mb;

    // all possible inodes, some of them are empty as per inode_mb data
    inode_vec nodes;
};

typedef struct layout_str layout;

#endif
