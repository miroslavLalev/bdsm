#ifndef BDSM_ERR_H
#define BDSM_ERR_H

enum fs_errors {
    NO_ERR,
    OPEN_ERR,
    READ_ERR,
    WRITE_ERR,
    INCOMPLETE_WRITE_ERR,
    CLOSE_ERR,
    CORRUPT_FS_ERR,
    INVALID_PATH_ERR,
    NEXST_PATH_ERR,
    NO_INODE_ERR,
};

typedef enum fs_errors fs_error;

#endif
