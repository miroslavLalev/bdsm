#ifndef DIRENT_H
#define DIRENT_H

#include <stdint.h>

#define DIRENT_NAME_SIZE 60

struct dirent_str {
    uint32_t inode_nr;
    char name[DIRENT_NAME_SIZE];
};
typedef struct dirent_str dirent;

struct dirent_bytes_str {
    uint8_t data[64];
};
typedef struct dirent_bytes_str dirent_bytes;

dirent_bytes encode_dirent(dirent d);
dirent decode_dirent(dirent_bytes db);

#endif
