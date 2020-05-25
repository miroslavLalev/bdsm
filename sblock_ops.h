#ifndef SBLOCK_OPS_H
#define SBLOCK_OPS_H

#include <stdint.h>

#include "sblock.h"

sblock_bytes encode_sblock(sblock sb);

sblock decode_sblock(sblock_bytes sb_bytes);

#endif
