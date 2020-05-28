#include <stdlib.h>

#include "inode.h"
#include "encutils.h"

inode_bytes inode_encode(inode n) {
    inode_bytes nb;
    size_t offset = 0;
    enc_u16(n.mode, nb.data, &offset);
    enc_u16(n.nr_links, nb.data, &offset);
    enc_u64(n.size, nb.data, &offset);

    size_t i;
    for (i=0; i<ZONES_SIZE; i++) {
        enc_u32(n.zones[i], nb.data, offset);
    }
    return nb;
}

inode inode_decode(inode_bytes nb) {
    inode n;
    size_t offset = 0;
    n.mode = dec_u16(nb.data, &offset);
    n.nr_links = dec_u16(nb.data, &offset);
    n.size = dec_u64(nb.data, &offset);

    size_t i;
    for (i=0; i<ZONES_SIZE; i++) {
        n.zones[i] = dec_u32(nb.data, &offset);
    }
    return n;
}
