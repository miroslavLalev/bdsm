#ifndef DIRENT_OPS_H
#define DIRENT_OPS_H

#include "dirent.h"

dirent_bytes encode_dirent(dirent d);

dirent decode_dirent(dirent_bytes db);

#endif
