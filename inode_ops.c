#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "assert.h"
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
    enc_u16(n.oid, nb.data, &offset);
    enc_u16(n.gid, nb.data, &offset);
    enc_u32(n.mtime, nb.data, &offset);

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
    n.oid = dec_u16(nb.data, &offset);
    n.gid = dec_u16(nb.data, &offset);
    n.mtime = dec_u32(nb.data, &offset);

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

void inode_perm_str(uint8_t perm, char *res) {
    if ((perm | M_READ) == perm) {
        res[0] = 'r';
    } else {
        res[0] = '-';
    }
    if ((perm | M_WRITE) == perm) {
        res[1] = 'w';
    } else {
        res[1] = '-';
    }
    if ((perm | M_EXEC) == perm) {
        res[2] = 'x';
    } else {
        res[2] = '-';
    }
}

void inode_type_str(uint8_t type, char *res) {
    if ((type | M_FILE) == type) {
        res[0] = '-';
    } else if ((type | M_DIR) == type) {
        res[0] = 'd';
    } else if ((type | M_SLNK) == type) {
        res[0] = 'l';
    } else {
        res[0] = '?';
    }
}

void inode_mode_str(uint16_t mode, char *res) {
    inode_type_str(inode_get_n_type(mode), res);
    inode_perm_str(inode_get_u_perm(mode), res+1);
    inode_perm_str(inode_get_g_perm(mode), res+4);
    inode_perm_str(inode_get_a_perm(mode), res+7);
}

size_t zone_index(inode_descriptor *d) {
    uint64_t n_dzone = d->offset / d->block_size;
    if (n_dzone < 7) {
        // first 7 zones are direct
        return n_dzone;
    }
    n_dzone -= 7;

    uint16_t c_ind = d->block_size/sizeof(uint32_t);
    if (n_dzone < c_ind) {
        return 7;
    }
    n_dzone -= c_ind;

    uint32_t c_ind_ind = (d->block_size/sizeof(uint32_t)) * (d->block_size/sizeof(uint32_t));
    if (n_dzone < c_ind_ind) {
        return 8;
    }
    n_dzone -= c_ind_ind;
    
    return -1;
}

ssize_t deref_zone(inode_descriptor *d, uint32_t zone_num, uint32_t *zones) {
    uint8_t *buf = (uint8_t*)malloc(d->block_size*sizeof(uint8_t));
    if (lseek(d->fd, d->data_offset + d->block_size * zone_num, SEEK_SET) < 0) {
        return -1;
    }
    ssize_t zones_res = read(d->fd, buf, d->block_size);
    if (zones_res < d->block_size) {
        return -1;
    }

    memset(zones, 0, d->block_size);
    size_t offset = 0;
    while (offset < d->block_size) {
        zones[offset/sizeof(uint32_t)] = dec_u32(buf, &offset);
    }
    free(buf);
    return 0;
}

ssize_t persist_zone(inode_descriptor *d, uint32_t zone_num, uint32_t *zones) {
    uint8_t *buf = (uint8_t*)malloc(d->block_size);
    memset(buf, 0, d->block_size);
    
    size_t offset = 0;
    while (offset < d->block_size) {
        enc_u32(zones[offset/sizeof(uint32_t)], buf, &offset);
    }
    if (lseek(d->fd, d->data_offset + d->block_size * zone_num, SEEK_SET) < 0) {
        return -1;
    }
    ssize_t zwres = write(d->fd, buf, d->block_size);
    if (zwres <= 0) {
        return -1;
    }
    free(buf);
    return 0;
}

ssize_t inode_desc_read_block(inode_descriptor *d, uint8_t *data) {
    size_t zi = zone_index(d);
    if (zi < 7) {
        if (lseek(d->fd, d->data_offset + d->block_size * (d->n->zones[zi]-1), SEEK_SET) < 0) {
            return -1;
        }
        ssize_t rres;
        if (d->n->size - d->offset < d->block_size) {
            rres = read(d->fd, data, d->n->size - d->offset);
        } else {
            rres = read(d->fd, data, d->block_size);
        }

        if (rres <= 0) {
            return rres;
        }
        d->offset += d->block_size;
        return rres;
    }
    if (zi == 7) {
        uint32_t *zones = (uint32_t*)malloc(d->block_size);
        if (deref_zone(d, d->n->zones[zi]-1, zones) < 0) {
            return -1;
        }

        uint16_t n = ((d->offset/d->block_size)-7);
        if (zones[n] == 0) {
            return 0;
        }
        if (lseek(d->fd, d->data_offset + d->block_size * (zones[n]-1), SEEK_SET) < 0) {
            return -1;
        }

        ssize_t rres;
        if (d->n->size - d->offset < d->block_size) {
            rres = read(d->fd, data, d->n->size - d->offset);
        } else {
            rres = read(d->fd, data, d->block_size);
        }
        if (rres <= 0) {
            return rres;
        }
        free(zones);
        d->offset += d->block_size;
        return rres;
    }
    if (zi < 10) {
        uint32_t *zones = (uint32_t*)malloc(d->block_size);
        if (deref_zone(d, d->n->zones[zi]-1, zones) < 0) {
            return -1;
        }

        uint32_t n = ((d->offset/d->block_size)-7-d->block_size/sizeof(uint32_t));
        uint32_t r = n/(d->block_size/sizeof(uint32_t));
        if (zones[r] == 0) {
            return 0;
        }
        uint32_t *zones_zones = (uint32_t*)malloc(d->block_size);
        if (deref_zone(d, zones[r]-1, zones_zones) < 0) {
            return -1;
        }
        uint32_t rr = n%(d->block_size/sizeof(uint32_t));
        if (zones_zones[rr] == 0) {
            return 0;
        }
        if (lseek(d->fd, d->data_offset + d->block_size * (zones_zones[rr]-1), SEEK_SET) < 0) {
            return -1;
        }

        ssize_t rres;
        if (d->n->size - d->offset < d->block_size) {
            rres = read(d->fd, data, d->n->size - d->offset);
        } else {
            rres = read(d->fd, data, d->block_size);
        }
        if (rres <= 0) {
            return rres;
        }
        free(zones_zones);
        free(zones);
        d->offset += d->block_size;
        return rres;
    }

    return -1;
}

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
    if (zi == 7) {    
        uint32_t *zones = (uint32_t*)malloc(d->block_size);
        if (deref_zone(d, d->n->zones[zi]-1, zones) < 0) {
            return -1;
        } 

        int new_zone = 1;
        uint16_t r = ((d->offset/d->block_size)-7);
        if (zones[r] == 0) {
            int reserved = mblock_vec_take_first(d->zones_mb);
            if (reserved <= 0) {
                return -1;
            }
            zones[r] = reserved+1;
            new_zone = 0;
        }
        if (lseek(d->fd, d->data_offset + d->block_size * (zones[r]-1), SEEK_SET) < 0) {
            return -1;
        }

        ssize_t rres;
        if (d->n->size - d->offset < d->block_size) {
            rres = write(d->fd, data, d->n->size - d->offset);
        } else {
            rres = write(d->fd, data, d->block_size);
        }
        if (rres <= 0) {
            return rres;
        }

        if (new_zone == 0 && persist_zone(d, d->n->zones[zi]-1, zones) < 0) {
            return -1;
        }

        free(zones);
        d->offset += d->block_size;
        return rres;
    }
    if (zi < 10) {
        uint32_t *zones = (uint32_t*)malloc(d->block_size);
        if (deref_zone(d, d->n->zones[zi]-1, zones) < 0) {
            return -1;
        }

        uint32_t n = ((d->offset/d->block_size)-7-d->block_size/sizeof(uint32_t));

        int new_ref_zone = 1;
        uint32_t r = n/(d->block_size/sizeof(uint32_t));
        if (zones[r] == 0) {
            int reserved = mblock_vec_take_first(d->zones_mb);
            if (reserved <= 0) {
                return -1;
            }
            zones[r] = reserved+1;
            new_ref_zone = 0;
        }

        uint32_t *zones_zones = (uint32_t*)malloc(d->block_size);
        if (deref_zone(d, zones[r]-1, zones_zones) < 0) {
            return -1;
        }

        int new_ref_ref_zone = 1;
        uint32_t rr = n%(d->block_size/sizeof(uint32_t));
        if (zones_zones[rr] == 0) {
            int reserved = mblock_vec_take_first(d->zones_mb);
            if (reserved <= 0) {
                return -1;
            }
            zones_zones[rr] = reserved+1;
            new_ref_ref_zone = 0;
        }

        if (lseek(d->fd, d->data_offset + d->block_size * (zones_zones[rr]-1), SEEK_SET) < 0) {
            return -1;
        }

        ssize_t rres;
        if (d->n->size - d->offset < d->block_size) {
            rres = write(d->fd, data, d->n->size - d->offset);
        } else {
            rres = write(d->fd, data, d->block_size);
        }
        if (rres <= 0) {
            return rres;
        }

        if (new_ref_ref_zone == 0 && persist_zone(d, zones[r]-1, zones_zones) < 0) {
            return -1;
        }
        if (new_ref_zone == 0 && persist_zone(d, d->n->zones[zi]-1, zones) < 0) {
            return -1;
        }

        free(zones_zones);
        free(zones);
        d->offset += d->block_size;
        return rres;
    }
    return -1;
}

ssize_t inode_desc_reset_zones(inode_descriptor *d) {
    size_t i;
    for (i=0; i<7; i++) {
        if (d->n->zones[i] == 0) {
            continue;
        }
        mblock_vec_unset(d->zones_mb, d->n->zones[i]-1);
        d->n->zones[i] = 0;
    }
    for (; i<8; i++) {
        if (d->n->zones[i] == 0) {
            continue;
        }

        uint32_t *zones = (uint32_t*)malloc(d->block_size);
        if (deref_zone(d, d->n->zones[i]-1, zones) < 0) {
            return -1;
        }

        size_t j;
        for (j=0; j<d->block_size/sizeof(uint32_t); j++) {
            if (zones[j] == 0) {
                continue;
            }
            mblock_vec_unset(d->zones_mb, zones[j]-1);
        }
        mblock_vec_unset(d->zones_mb, d->n->zones[i]-1);
        d->n->zones[i] = 0;

        free(zones);
    }
    for (; i<10; i++) {
        if (d->n->zones[i] == 0) {
            continue;
        }

        uint32_t *zones = (uint32_t*)malloc(d->block_size);
        if (deref_zone(d, d->n->zones[i]-1, zones) < 0) {
            return -1;
        }

        size_t j;
        for (j=0; j<d->block_size/sizeof(uint32_t); j++) {
            if (zones[j] == 0) {
                continue;
            }
            
            uint32_t *zones_zones = (uint32_t*)malloc(d->block_size);
            if (deref_zone(d, zones[j]-1, zones_zones) < 0) {
                return -1;
            }

            size_t k;
            for (k=0; k<d->block_size/sizeof(uint32_t); k++) {
                if (zones_zones[k] == 0) {
                    continue;
                }
                mblock_vec_unset(d->zones_mb, zones_zones[k]-1);
            }

            mblock_vec_unset(d->zones_mb, zones[j]-1);
            free(zones_zones);
        }
        mblock_vec_unset(d->zones_mb, d->n->zones[i]-1);
        d->n->zones[i] = 0;
        free(zones);
    }

    return 0;
}

void __local_asserts_inode() {
    COMPILE_TIME_ASSERT(sizeof(inode) == 64);
}
