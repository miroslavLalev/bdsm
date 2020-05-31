#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "bdsmerr.h"
#include "layout.h"
#include "layout_ops.h"
#include "sblock.h"
#include "sblock_ops.h"
#include "fs_types.h"
#include "fs_path.h"
#include "inode.h"
#include "inode_ops.h"

fs_error bdsm_mkfs(char *fs_file) {
    int fd = open(fs_file, O_CREAT|O_TRUNC|O_RDWR, 0660);
    if (fd == -1) {
        // TODO: propagate errno
        return OPEN_ERR;
    }

    sblock sb;
    // TODO: pick fields according to the size of the given file
    sb.n_inodes = 10000;
    sb.imap_blocks = 10; // 81920 total inodes
    sb.zmap_blocks = 10000; //  78 GB total size of data blocks
    sb.max_size = UINT64_MAX;
    sb.zones = UINT64_MAX;
    // TODO: block_size could be _asserted_ command line argument
    sb.block_size = 1024;

    layout l = layout_init(sb);
    size_t buf_size = layout_size(sb);
    uint8_t *buf = (uint8_t*)malloc(buf_size);
    layout_encode(&l, buf);
    layout_drop(&l);

    ssize_t wr_bytes = write(fd, buf, buf_size);
    free(buf);
    if (wr_bytes == -1) {
        // TODO: propagate errno
        return WRITE_ERR;
    }
    if ((size_t)wr_bytes < buf_size) {
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
    memset(sbb.data, 0, SBLOCK_SIZE * sizeof(uint8_t));
    memcpy(sbb.data, enc_data, r_bytes);
    sblock sb = sblock_decode(sbb);
    layout l = layout_recreate(sb);
    size_t mb_size = layout_size(sb) - 1024;
    uint8_t *mb_buf = (uint8_t*)malloc(mb_size);
    r_bytes = read(fd, mb_buf, mb_size);
    if (r_bytes == -1) {
        return READ_ERR;
    }
    if ((size_t)r_bytes < mb_size) {
        return CORRUPT_FS_ERR;
    }
    layout_extend(&l, mb_buf);
    layout_drop(&l);

    int c_ret = close(fd);
    if (c_ret == -1) {
        return CLOSE_ERR;
    }
    return NO_ERR;
}

inode_descriptor idesc_create(layout l, inode n, int fd) {
    inode_descriptor id;
    id.block_size = l.sb.block_size;
    id.data_offset = l.sb.first_data_zone;
    id.n = n;
    id.fd = fd;
    id.offset = 0;
    return id;
}

fs_error bdsm_debug(char *fs_file, fs_debug *res) {
    int fd = open(fs_file, O_RDWR);
    if (fd == -1) {
        // TODO: errno
        return OPEN_ERR;
    }

    uint8_t enc_data[1024];
    ssize_t r_bytes = read(fd, enc_data, 1024);
    if (r_bytes == -1) {
        // TODO: errno
        return READ_ERR;
    }
    if (r_bytes < 1024) {
        return CORRUPT_FS_ERR;
    }


    sblock_bytes sbb;
    memset(sbb.data, 0, SBLOCK_SIZE * sizeof(uint8_t));
    memcpy(sbb.data, enc_data, r_bytes);
    sblock sb = sblock_decode(sbb);
    layout l = layout_recreate(sb);
    size_t mb_size = layout_size(sb) - 1024;
    uint8_t *mb_buf = (uint8_t*)malloc(mb_size);
    r_bytes = read(fd, mb_buf, mb_size);
    if (r_bytes == -1) {
        return READ_ERR;
    }
    if ((size_t)r_bytes < mb_size) {
        return CORRUPT_FS_ERR;
    }
    layout_extend(&l, mb_buf);
    res->block_size = l.sb.block_size;
    res->max_size = l.sb.max_size;
    res->n_inodes = l.sb.n_inodes;
    // TODO: add more data

    layout_drop(&l);

    int c_ret = close(fd);
    if (c_ret == -1) {
        return CLOSE_ERR;
    }
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
    char **segments;
    size_t path_size = fs_path_split(dir_path, segments);
    if (path_size == 0) {
        // TODO: propagate fs_path_errno
        return INVALID_PATH_ERR;
    }
    if (path_size == 1) {
        if (
            strcmp(".", segments[0]) == 0 ||
            strcmp("..", segments[0]) == 0
        ) {
            // noop
            return NO_ERR;
        }
    }

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
    memset(sbb.data, 0, SBLOCK_SIZE * sizeof(uint8_t));
    memcpy(sbb.data, enc_data, r_bytes);
    sblock sb = sblock_decode(sbb);
    layout l = layout_recreate(sb);
    size_t mb_size = layout_size(sb) - 1024;
    uint8_t *mb_buf = (uint8_t*)malloc(mb_size);
    r_bytes = read(fd, mb_buf, mb_size);
    if (r_bytes == -1) {
        return READ_ERR;
    }
    if ((size_t)r_bytes < mb_size) {
        return CORRUPT_FS_ERR;
    }
    layout_extend(&l, mb_buf);

    int i;
    for (i=0; i<path_size-1; i++) {
        // ensure path exists
    }
    // create last segment

    layout_drop(&l);
    int c_ret = close(fd);
    if (c_ret == -1) {
        return CLOSE_ERR;
    }
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
