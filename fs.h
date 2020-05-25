#ifndef FS_H
#define FS_H

#include "bdsmerr.h"

// Initializes a new BDSM file system into fs_file.
fs_error bdsm_mkfs(char *fs_file);

// Checks the given BDSM file system's integrity.
fs_error bdsm_fsck(char *fs_file);

// Information for the given BDSM file system.
fs_error bdsm_debug(char *fs_file);

// Information for the given BDSM object.
fs_error bdsm_lsobj(char *fs_file, char *obj_path);

// List a directory contents.
fs_error bdsm_lsdir(char *fs_file, char *dir_path);

// Information about the given object.
fs_error bdsm_stat(char *fs_file, char *obj_path);

// Create a directory on the given path.
fs_error bdsm_mkdir(char *fs_file, char *dir_path);

// Remove a directory from the given path.
fs_error bdsm_rmdir(char *fs_file, char *dir_path);

// Copy a file from host FS to BDSM path. The path parameters are
// interchangeable.
fs_error bdsm_cpfile(char *fs_file, char *path1, char *path2);

// Remove a file for the given path.
fs_error bdsm_rmfile(char *fs_file, char *file_path);

#endif
