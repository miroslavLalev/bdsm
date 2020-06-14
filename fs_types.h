#ifndef FS_TYPES_H
#define FS_TYPES_H

#include <stdint.h>

struct fs_debug_str {
    // sblock data
    uint32_t n_inodes;
    uint64_t max_size;
    uint16_t block_size;

};

typedef struct fs_debug_str fs_debug;

struct fs_lsdir_str {
    char *name;
    char *mode;
    uint16_t nr_links;
    uint64_t size;
};

typedef struct fs_lsdir_str fs_lsdir;

#endif
