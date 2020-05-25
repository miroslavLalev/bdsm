#ifndef MBLOCK_OPS_H
#define MBLOCK_OPS_H

#include <stdlib.h>

#include "mblock.h"

int mblock_take_first(mblock *m);

void mblock_unset(mblock *m, size_t n);

mblock_vec mblock_container(size_t size);

#endif
