#ifndef FS_PATH_H
#define FS_PATH_H

#include <stdlib.h>

extern int fs_path_errno;

size_t fs_path_split(char *path, char **res);

#endif
