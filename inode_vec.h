#ifndef INODE_VEC_H
#define INODE_VEC_H

#include <stdlib.h>
#include <stdint.h>

#include "inode.h"

struct inode_vec_str {
    inode *nodes;
    size_t size;    
    size_t capacity;
};

typedef struct inode_vec_str inode_vec;

#endif