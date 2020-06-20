#ifndef INODE_OPS_H
#define INODE_OPS_H

#include <stdint.h>
#include <unistd.h>

#include "inode.h"

inode_bytes inode_encode(inode n);

inode inode_decode(inode_bytes nb);

void inode_set_mode(inode *n, uint8_t u_perm, uint8_t g_perm, uint8_t a_perm, uint8_t type);

uint8_t inode_get_u_perm(uint16_t mode);

uint8_t inode_get_g_perm(uint16_t mode);

uint8_t inode_get_a_perm(uint16_t mode);

uint8_t inode_get_n_type(uint16_t mode);

inode_descriptor inode_desc_create(inode n);

ssize_t inode_desc_read_block(inode_descriptor *d, uint8_t *data);

ssize_t inode_desc_write_block(inode_descriptor *d, uint8_t *data);

ssize_t inode_desc_reset_zones(inode_descriptor *d);

#endif
