#ifndef LAYOUT_OPS_H
#define LAYOUT_OPS_H

#include <stdlib.h>
#include <stdint.h>

#include "layout.h"
#include "sblock.h"

layout layout_init(sblock sb);

layout layout_recreate(sblock sb);

void layout_drop(layout *l);

size_t layout_size(sblock sb);

void layout_encode(layout *l, uint8_t *buf);

// fill layout's map blocks
void layout_extend(layout *l, uint8_t *mb_buf);

#endif
