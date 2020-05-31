#ifndef DIRENT_OPS_H
#define DIRENT_OPS_H

#include "dirent.h"

dirent_bytes dirent_encode(dirent d);

dirent dirent_decode(dirent_bytes db);

#endif
