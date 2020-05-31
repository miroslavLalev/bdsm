#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "inode.h"
#include "encutils.h"

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

ssize_t inode_desc_read(inode_descriptor *d, uint8_t *data, size_t size) {
    if (d->zone == ZONES_SIZE - 1 && d->offset == d->block_size) {
        return 0; // EOF
    }
    if (d->n.zones[d->zone] == 0) {
        return 0; // EOF
    }
    // TODO: size > max_size check

    uint8_t *cur_data = data;
    size_t cur_size;
    while (cur_size < size) {
        uint64_t cur_offset = d->data_offset + (d->n.zones[d->zone]-1) * d->block_size + d->offset;
        // TODO: implement indirect zones
        if (lseek(d->fd, cur_offset, SEEK_SET) == -1) {
            return -1;
        }
        ssize_t r_res = read(d->fd, cur_data, d->block_size - d->offset);
        if (r_res < 0) {
            return r_res;
        }
        if (r_res == 0) {
            return cur_size; // EOF
        }

        d->zone++;
        d->offset=0;
        cur_size += r_res;
        cur_data += r_res;
        if (d->zone == ZONES_SIZE || d->n.zones[d->zone] == 0) {
            return cur_size; // EOF
        }
    }
    return cur_size;
}

ssize_t inode_desc_write(inode_descriptor *d, uint8_t *data, size_t size) {
    return -1;
}
