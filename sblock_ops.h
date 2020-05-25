#ifndef SBLOCK_OPS_H
#define SBLOCK_OPS_H

#include <stdint.h>

#include "sblock.h"

sblock_bytes sblock_encode(sblock sb);

sblock sblock_decode(sblock_bytes sb_bytes);

#endif
