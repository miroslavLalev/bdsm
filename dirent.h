#ifndef DIRENT_H
#define DIRENT_H

#include <stdint.h>

struct dirent_str {
    uint32_t inode_nr; // could we overflow here, based on params?
    char name[60];
};

typedef struct dirent_str dirent;

#endif
