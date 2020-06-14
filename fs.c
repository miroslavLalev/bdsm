#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "bdsmerr.h"
#include "layout.h"
#include "layout_ops.h"
#include "sblock.h"
#include "sblock_ops.h"
#include "fs_types.h"
#include "fs_path.h"
#include "inode.h"
#include "inode_ops.h"
#include "inode_vec.h"
#include "inode_vec_ops.h"
#include "dirent.h"
#include "dirent_ops.h"
#include "dirent_vec.h"
#include "dirent_vec_ops.h"
#include "mblock.h"
#include "mblock_ops.h"

int wrap_errno(int no) {
    return no ? no : -1;
}

fs_error bdsm_mkfs(char *fs_file) {
    int fd = open(fs_file, O_CREAT|O_TRUNC|O_RDWR, 0660);
    if (fd == -1) {
        return fs_err_create("failed to open VFS file", wrap_errno(errno));
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
        return fs_err_create("failed to write to VFS file", wrap_errno(errno));
    }
    if ((size_t)wr_bytes < buf_size) {
        return fs_err_create("incomplete write to VFS file", wrap_errno(errno));
    }

    int c_ret = close(fd);
    if (c_ret == -1) {
        return fs_err_create("failed to close VFS file", wrap_errno(errno));
    }
    return fs_no_err();
}

fs_error bdsm_fsck(char *fs_file) {
    int fd = open(fs_file, O_RDWR);
    if (fd == -1) {
        return fs_err_create("failed to open VFS file", wrap_errno(errno));
    }

    uint8_t enc_data[1024];
    ssize_t r_bytes = read(fd, &enc_data, 1024);
    if (r_bytes == -1) {
        return fs_err_create("failed to read superblock", wrap_errno(errno));
    }
    if (r_bytes < 1024) {
        return fs_err_create("corrupt file: size less than 1024 bytes", wrap_errno(0));
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
        return fs_err_create("failed to read layout", wrap_errno(errno));
    }
    if ((size_t)r_bytes < mb_size) {
        return fs_err_create("corrupt file: size less than required for layout", wrap_errno(0));
    }
    layout_extend(&l, mb_buf);
    layout_drop(&l);

    int c_ret = close(fd);
    if (c_ret == -1) {
        return fs_err_create("close VFS file err", wrap_errno(errno));
    }
    return fs_no_err();
}

inode_descriptor idesc_create(layout l, inode *n, mblock_vec *zones_mb, int fd) {
    inode_descriptor id;
    id.block_size = l.sb.block_size;
    id.data_offset = l.sb.first_data_zone;
    id.n = n;
    id.zones_mb = zones_mb;
    id.fd = fd;
    id.offset = 0;
    return id;
}

fs_error bdsm_debug(char *fs_file, fs_debug *res) {
    int fd = open(fs_file, O_RDWR);
    if (fd == -1) {
        return fs_err_create("failed to open VFS file", wrap_errno(errno));
    }

    uint8_t enc_data[1024];
    ssize_t r_bytes = read(fd, enc_data, 1024);
    if (r_bytes == -1) {
        return fs_err_create("failed to read superblock", wrap_errno(errno));
    }
    if (r_bytes < 1024) {
        return fs_err_create("corrupt file: size less than 1024 bytes", wrap_errno(0));
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
        return fs_err_create("failed to read layout", wrap_errno(errno));
    }
    if ((size_t)r_bytes < mb_size) {
        return fs_err_create("corrupt file: size less than required for layout", wrap_errno(0));
    }
    layout_extend(&l, mb_buf);
    res->block_size = l.sb.block_size;
    res->max_size = l.sb.max_size;
    res->n_inodes = l.sb.n_inodes;
    // TODO: add more data

    layout_drop(&l);

    int c_ret = close(fd);
    if (c_ret == -1) {
        return fs_err_create("close VFS file err", wrap_errno(errno));
    }
    return fs_no_err();
}

fs_error bdsm_lsobj(char *fs_file, char *obj_path) {
    return fs_no_err();
}

fs_error bdsm_lsdir(char *fs_file, char *dir_path) {
    int fd = open(fs_file, O_RDWR);
    if (fd == -1) {
        return fs_err_create("failed to open VFS file", wrap_errno(errno));
    }

    uint8_t enc_data[1024];
    ssize_t r_bytes = read(fd, &enc_data, 1024);
    if (r_bytes == -1) {
        return fs_err_create("failed to read superblock", wrap_errno(errno));
    }
    if (r_bytes < 1024) {
        return fs_err_create("corrupt file: size less than 1024 bytes", wrap_errno(0));
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
        return fs_err_create("failed to read layout", wrap_errno(errno));
    }
    if ((size_t)r_bytes < mb_size) {
        return fs_err_create("corrupt file: size less than required for layout", wrap_errno(0));
    }
    layout_extend(&l, mb_buf);

    inode iroot = inode_vec_get(l.nodes, 0);
    inode_descriptor idesc = idesc_create(l, &iroot, &l.zones_mb, fd);
    uint8_t *buf = (uint8_t*)malloc(1024); // read first zone for now
    ssize_t id_r_bytes = inode_desc_read_block(&idesc, buf);
    if (id_r_bytes == -1) {
        return fs_err_create("failed to read root block", wrap_errno(errno));
    }

    dirent_bytes db;
    memcpy(db.data, buf, 64);
    dirent de = dirent_decode(db);

    return fs_no_err();
}

fs_error bdsm_stat(char *fs_file, char *obj_path) {
    return fs_no_err();
}

fs_error bdsm_mkdir(char *fs_file, char *dir_path) {
    char **segments = (char**)malloc(100*sizeof(char*));
    size_t path_size = fs_path_split(dir_path, &segments);
    if (path_size == 0) {
        return fs_err_create("failed to split path", fs_path_errno);
    }

    int fd = open(fs_file, O_RDWR);
    if (fd == -1) {
        return fs_err_create("failed to open VFS file", wrap_errno(errno));
    }

    uint8_t enc_data[1024];
    ssize_t r_bytes = read(fd, &enc_data, 1024);
    if (r_bytes == -1) {
        return fs_err_create("failed to read superblock", wrap_errno(errno));
    }
    if (r_bytes < 1024) {
        return fs_err_create("corrupt file: size less than 1024 bytes", wrap_errno(0));
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
        return fs_err_create("failed to read layout", wrap_errno(errno));
    }
    if ((size_t)r_bytes < mb_size) {
        return fs_err_create("corrupt file: size less than required for layout", wrap_errno(0));
    }
    layout_extend(&l, mb_buf);

    size_t inode_num = 0;
    size_t i;
    dirent_vec last_dv = dirent_vec_init(1);
    for (i=0; i<path_size-1; i++) {
        inode node = inode_vec_get(l.nodes, 0);
        if (inode_get_n_type(node.mode) != M_DIR) {
            return fs_err_create("failed to create dir: path contains non-dir segements", wrap_errno(0));
        }

        inode_descriptor idesc = idesc_create(l, &node, &l.zones_mb, fd);
        dirent_vec dv = dirent_vec_init(100);

        uint8_t *buf = (uint8_t*)malloc(sb.block_size*sizeof(uint8_t));
        ssize_t rres;
        while ((rres = inode_desc_read_block(&idesc, buf)) > 0) {
            if (rres < 0) {
                return fs_err_create("failed to read directory blocks", wrap_errno(errno));
            }
            size_t s_buf;
            for (s_buf=0; s_buf<(size_t)rres; s_buf+=sizeof(dirent)) {
                dirent_bytes db;
                memcpy(db.data, buf, sizeof(dirent));
                dirent_vec_push(&dv, dirent_decode(db));

                buf+=sizeof(dirent);
            }

            free(buf);
            buf = (uint8_t*)malloc(sb.block_size*sizeof(uint8_t));
        }

        size_t next_inode = inode_num;
        size_t i_ent;
        for (i_ent=0; i_ent<dv.size; i_ent++) {
            if (strcmp(dirent_vec_get(dv, i_ent).name, segments[i]) == 0) {
                next_inode = dirent_vec_get(dv, i_ent).inode_nr;
                break;
            }
        }

        if (i != path_size - 2) {
            free(dv.entries);
        } else {
            last_dv = dv;
        }
        if (next_inode == inode_num) {
            return fs_err_create("path not found", wrap_errno(0));
        }
    }

    for (i=0; i<last_dv.size; i++) {
        if (strcmp(dirent_vec_get(last_dv, i).name, segments[path_size-1]) == 0) {
            return fs_err_create("file or directory already exists", wrap_errno(0));
        }
    }

    dirent d;
    strncpy(d.name, segments[0], DIRENT_NAME_SIZE);
    d.inode_nr = mblock_vec_take_first(&l.inode_mb);
    if (d.inode_nr == 0) {
        return fs_err_create("could not allocate more inodes", wrap_errno(0));
    }
    dirent_vec_push(&last_dv, d);

    inode n;
    inode_set_mode(&n, M_READ|M_WRITE|M_EXEC, M_READ|M_EXEC, 0, M_DIR);
    n.nr_links = 1;
    n.size = 0;
    memset(n.zones, 0, ZONES_SIZE*sizeof(uint32_t));
    inode_vec_set(&l.nodes, n, d.inode_nr);

    inode parent = inode_vec_get(l.nodes, inode_num);
    inode_descriptor pdesc = idesc_create(l, &parent, &l.zones_mb, fd);

    uint16_t batch_size = sb.block_size / sizeof(dirent);
    dirent_vec batch = dirent_vec_init(batch_size);
    for (i=0; i<last_dv.size; i++) {
        dirent_vec_push(&batch, dirent_vec_get(last_dv, i));
        if (batch.size == batch_size) {
            uint8_t *buf = (uint8_t*)malloc(sb.block_size);
            uint16_t offset = 0;
            size_t j;
            for (j=0; j<batch_size; j++) {
                dirent_bytes db = dirent_encode(dirent_vec_get(batch, j));
                memcpy(buf+offset, db.data, sizeof(dirent));
                offset+=sizeof(dirent);
            }
            ssize_t wres = inode_desc_write_block(&pdesc, buf);
            if (wres < sb.block_size) {
                return fs_err_create("invalid block write", wrap_errno(errno));
            }

            free(batch.entries);
            batch = dirent_vec_init(batch_size);
        }
    }

    size_t buf_size = layout_size(sb);
    uint8_t *buf = (uint8_t*)malloc(buf_size);
    layout_encode(&l, buf);
    layout_drop(&l);

    if (lseek(fd, 0, SEEK_SET) < 0) {
        return fs_err_create("failed to seek in VFS file", wrap_errno(errno));
    }
    ssize_t wr_bytes = write(fd, buf, buf_size);
    free(buf);
    if (wr_bytes == -1) {
        return fs_err_create("failed to persist layout", wrap_errno(errno));
    }
    if ((size_t)wr_bytes < buf_size) {
        return fs_err_create("failed to persist layout: corrupted write", wrap_errno(0));
    }

    int c_ret = close(fd);
    if (c_ret == -1) {
        return fs_err_create("failed to close VFS file", wrap_errno(errno));
    }
    return fs_no_err();
}

fs_error bdsm_rmdir(char *fs_file, char *dir_path) {
    return fs_no_err();
}

fs_error bdsm_cpfile(char *fs_file, char *path1, char *path2) {
    return fs_no_err();
}

fs_error bdsm_rmfile(char *fs_file, char *file_path) {
    return fs_no_err();
}
