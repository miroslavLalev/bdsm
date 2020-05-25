#ifndef LAYOUT_OPS_H
#define LAYOUT_OPS_H

#include <stdlib.h>
#include <stdint.h>

#include "layout.h"
#include "sblock.h"

layout layout_create(sblock sb);

void layout_drop(layout *l);

size_t layout_size(layout *l);

void layout_encode(layout *l, uint8_t *buf);

layout layout_decode(uint8_t *buf);

#endif
