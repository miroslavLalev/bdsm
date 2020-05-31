#ifndef INODE_H
#define INODE_H

#include <stdint.h>

#define INODE_SIZE 64
#define ZONES_SIZE 10

// octal numbers for inode type
const uint8_t M_DIR, M_FILE, M_SLNK;

// octal numbers for permission bits
const uint8_t M_EXEC, M_WRITE, M_READ;

struct inode_str {
    uint16_t mode;
    uint16_t nr_links;
    uint64_t size;
    uint32_t zones[ZONES_SIZE]; // 7 direct, 1 indirect, 1 doubly-indirect, 1 triply-indirect

    // make up to 64 bytes in size;
    uint8_t pad[12];
};

typedef struct inode_str inode;

struct inode_bytes_str {
    uint8_t data[INODE_SIZE];
};

typedef struct inode_bytes_str inode_bytes;

#endif
