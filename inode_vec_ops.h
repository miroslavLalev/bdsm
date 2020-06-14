#ifndef INODE_VEC_OPS_H
#define INODE_VEC_OPS_H

#include <stdlib.h>

#include "inode_vec.h"
#include "inode.h"

inode_vec inode_vec_init(size_t capacity);

void inode_vec_push(inode_vec *v, inode n);

inode inode_vec_remove(inode_vec *v, size_t i);

inode inode_vec_get(inode_vec v, size_t i);

void inode_vec_set(inode_vec *v, inode n, size_t i);

void inode_vec_drop(inode_vec *v);

#endif
