#ifndef DIRENT_VEC_H
#define DIRENT_VEC_H

#include <stdlib.h>

#include "dirent.h"

struct dirent_vec_str {
    dirent *entries;
    size_t size;
    size_t capacity;
};

typedef struct dirent_vec_str dirent_vec;

#endif
