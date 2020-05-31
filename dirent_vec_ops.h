#ifndef DIRENT_VEC_OPS_H
#define DIRENT_VEC_OPS_H

#include <stdlib.h>

#include "dirent.h"
#include "dirent_vec.h"

dirent_vec dirent_vec_init(size_t cap);

void dirent_vec_push(dirent_vec *v, dirent d);

dirent dirent_vec_remove(dirent_vec *v, size_t i);

dirent dirent_vec_get(dirent_vec v, size_t i);

#endif
