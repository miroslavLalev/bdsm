#ifndef INODE_H
#define INODE_H

#include <stdint.h>

#include "mblock.h"

#define ZONES_SIZE 10

// octal numbers for inode type
const uint8_t M_DIR, M_FILE, M_SLNK;

// octal numbers for permission bits
const uint8_t M_EXEC, M_WRITE, M_READ;

struct inode_str {
    uint16_t mode;
    uint16_t nr_links;
    uint64_t size;
    uint16_t oid;
    uint16_t gid;
    uint32_t mtime;
    uint32_t zones[ZONES_SIZE];

    // make up to 64 bytes in size;
    uint8_t pad[4];
};

typedef struct inode_str inode;

struct inode_bytes_str {
    uint8_t data[sizeof(inode)];
};

typedef struct inode_bytes_str inode_bytes;

struct inode_descriptor_str {
    inode *n;
    mblock_vec *zones_mb;

    int fd;
    int data_offset;
    uint16_t block_size;

    uint64_t offset;
};

typedef struct inode_descriptor_str inode_descriptor;

#endif
