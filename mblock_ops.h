#ifndef MBLOCK_OPS_H
#define MBLOCK_OPS_H

#include <stdlib.h>

#include "mblock.h"

mblock_vec mblock_vec_create(size_t size);

void mblock_vec_drop(mblock_vec mv);

int mblock_vec_take_first(mblock_vec mv);

void mblock_vec_unset(mblock_vec mv, int k);

#endif
