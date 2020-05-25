#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "bdsmerr.h"
#include "sblock.h"
#include "sblock_ops.h"

fs_error bdsm_mkfs(char *fs_file) {
    int fd = open(fs_file, O_CREAT|O_TRUNC|O_RDWR, 0660);
    if (fd == -1) {
        // TODO: propagate errno
        return OPEN_ERR;
    }

    sblock sb;

    // TODO: pick according to the size of the given file
    sb.n_inodes = UINT32_MAX;
    sb.imap_blocks = UINT16_MAX;
    sb.zmap_blocks = UINT16_MAX;
    sb.first_data_zone = 0; // TODO
    sb.max_size = UINT64_MAX;
    sb.zones = UINT64_MAX;
    // TODO: block_size could be command line argument
    sb.block_size = 1024;

    sblock_bytes enc = sblock_encode(sb);
    ssize_t wr_bytes = write(fd, enc.data, 1024);
    if (wr_bytes == -1) {
        // TODO: propagate errno
        return WRITE_ERR;
    }
    if (wr_bytes < 1024) {
        // no space or signal before completion
        return INCOMPLETE_WRITE_ERR;
    }

    int c_ret = close(fd);
    if (c_ret == -1) {
        // TODO: propagate errno
        return CLOSE_ERR;
    }
    return NO_ERR;
}

fs_error bdsm_fsck(char *fs_file) {
    int fd = open(fs_file, O_RDWR);
    if (fd == -1) {
        // TODO: errno
        return OPEN_ERR;
    }

    uint8_t enc_data[1024];
    ssize_t r_bytes = read(fd, &enc_data, 1024);
    if (r_bytes == -1) {
        // TODO: errno
        return READ_ERR;
    }
    if (r_bytes < 1024) {
        return CORRUPT_FS_ERR;
    }

    sblock_bytes sbb;
    memcpy(&sbb.data, &enc_data, r_bytes);
    sblock sb = sblock_decode(sbb);

    int c_ret = close(fd);
    if (c_ret == -1) {
        return CLOSE_ERR;
    }
    return NO_ERR;
}

fs_error bdsm_debug(char *fs_file) {
    return NO_ERR;
}

fs_error bdsm_lsobj(char *fs_file, char *obj_path) {
    return NO_ERR;
}

fs_error bdsm_lsdir(char *fs_file, char *dir_path) {
    return NO_ERR;
}

fs_error bdsm_stat(char *fs_file, char *obj_path) {
    return NO_ERR;
}

fs_error bdsm_mkdir(char *fs_file, char *dir_path) {
    return NO_ERR;
}

fs_error bdsm_rmdir(char *fs_file, char *dir_path) {
    return NO_ERR;
}

fs_error bdsm_cpfile(char *fs_file, char *path1, char *path2) {
    return NO_ERR;
}

fs_error bdsm_rmfile(char *fs_file, char *file_path) {
    return NO_ERR;
}
