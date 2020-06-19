#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "inode.h"
#include "encutils.h"
#include "mblock_ops.h"

const uint16_t U_BITS = 07;
const uint16_t G_BITS = 07 << 3;
const uint16_t A_BITS = 07 << 6;
const uint16_t T_BITS = 07 << 9;

inode_bytes inode_encode(inode n) {
    inode_bytes nb;
    size_t offset = 0;
    enc_u16(n.mode, nb.data, &offset);
    enc_u16(n.nr_links, nb.data, &offset);
    enc_u64(n.size, nb.data, &offset);

    size_t i;
    for (i=0; i<ZONES_SIZE; i++) {
        enc_u32(n.zones[i], nb.data, &offset);
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

uint8_t count_set_bits(uint8_t val) {
    uint8_t count = 0;
    while(val) {
        val &= (val - 1);
        count++;
    }
    return count;
}

void set_u_perm(uint16_t *mode, uint8_t perm) {
    *mode = (*mode & ~U_BITS) | perm;
}

void set_g_perm(uint16_t *mode, uint8_t perm) {
    *mode = (*mode & ~G_BITS) | (perm << 3);
}

void set_a_perm(uint16_t *mode, uint8_t perm) {
    *mode = (*mode & ~A_BITS) | (perm << 6);
}

void set_n_type(uint16_t *mode, uint8_t type) {
    // exactly 1 bit should be set, otherwise we are doing something wrong
    assert(count_set_bits(type) == 1);
    *mode = (*mode & ~T_BITS) | (type << 9);
}

void inode_set_mode(inode *n, uint8_t u_perm, uint8_t g_perm, uint8_t a_perm, uint8_t type) {
    set_u_perm(&n->mode, u_perm);
    set_g_perm(&n->mode, g_perm);
    set_a_perm(&n->mode, a_perm);
    set_n_type(&n->mode, type);
}

uint8_t inode_get_u_perm(uint16_t mode) {
    return mode & U_BITS;
}

uint8_t inode_get_g_perm(uint16_t mode) {
    return (mode & G_BITS) >> 3;
}

uint8_t inode_get_a_perm(uint16_t mode) {
    return (mode & A_BITS) >> 6;
}

uint8_t inode_get_n_type(uint16_t mode) {
    return (mode & T_BITS) >> 9;
}

size_t zone_index(inode_descriptor *d) {
    uint64_t n_dzone = d->offset / d->block_size;
    if (n_dzone < 7) {
        // first 7 zones are direct
        return n_dzone;
    }
    n_dzone -= 7;

    uint16_t c_indirect = d->block_size/sizeof(uint64_t);
    if (n_dzone < c_indirect) {
        return 7;
    }
    assert(1);
    return -1;
}

ssize_t inode_desc_read_block(inode_descriptor *d, uint8_t *data) {
    size_t zi = zone_index(d);
    if (zi < 7) {
        if (lseek(d->fd, d->data_offset + d->block_size * (d->n->zones[zi]-1), SEEK_SET) < 0) {
            return -1;
        }
        ssize_t rres = read(d->fd, data, d->block_size);
        if (rres <= 0) {
            return rres;
        }
        d->offset += d->block_size;
        return rres;
    }
    if (zi == 7) {
        uint8_t *buf = (uint8_t*)malloc(d->block_size*sizeof(uint8_t));
        if (lseek(d->fd, d->data_offset + d->block_size * (d->n->zones[zi]-1), SEEK_SET) < 0) {
            return -1;
        }
        ssize_t zones_res = read(d->fd, buf, d->block_size);
        if (zones_res < d->block_size) {
            return -1;
        }

        uint64_t *zones = (uint64_t*)malloc(d->block_size/sizeof(uint64_t));
        uint64_t offset = 0;
        while (offset < d->block_size) {
            zones[offset/sizeof(uint64_t)] = dec_u64(buf, &offset);
        }

        uint16_t n = ((d->offset/d->block_size)-7)/sizeof(uint64_t);
        if (lseek(d->fd, d->data_offset + d->block_size * (d->n->zones[n]-1), SEEK_SET) < 0) {
            return -1;
        }
        ssize_t rres = read(d->fd, data, d->block_size);
        if (rres <= 0) {
            return rres;
        }
        d->offset += d->block_size;
        return rres;
    }

    return -1;
}

// assume that enough zones are allocated before calling the method
ssize_t inode_desc_write_block(inode_descriptor *d, uint8_t *data) {
    size_t zi = zone_index(d);
    if (d->n->zones[zi] == 0) {
        int reserved = mblock_vec_take_first(d->zones_mb);
        if (reserved <= 0) {
            return -1;
        }
        d->n->zones[zi] = reserved+1;
    }
    if (zi < 7) {
        if (lseek(d->fd, d->data_offset + d->block_size * (d->n->zones[zi]-1), SEEK_SET) < 0) {
            return -1;
        }
        ssize_t rres = write(d->fd, data, d->block_size);
        if (rres <= 0) {
            return rres;
        }
        d->offset += d->block_size;
        return rres;
    }
    return -1;
}
