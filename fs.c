#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

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

const uint16_t FS_NUM = ((uint16_t)'b' << 4) + ((uint16_t)'d' << 3) + ((uint16_t)'s' << 2) + (uint16_t)'m';

fs_error bdsm_mkfs(char *fs_file) {
    int fd = open(fs_file, O_RDWR);
    if (fd == -1) {
        return fs_err_create("failed to open VFS file", wrap_errno(errno));
    }

    struct stat s;
    if (fstat(fd, &s) < 0) {
        return fs_err_create("could not get VFS file metadata", wrap_errno(errno));
    }
    // make the max size divisible by 512
    uint64_t fs_size = (s.st_size / 512) * 512;

    off_t full_size = 0;
    int batch_size = 4 * 1024;
    uint8_t *buf = (uint8_t*)malloc(batch_size);
    while (full_size < s.st_size) {
        memset(buf, 0, batch_size);

        int wsize = batch_size;
        if (s.st_size - full_size < batch_size) {
            wsize = s.st_size - full_size;
        }

        if (write(fd, buf, wsize) <= 0) {
            return fs_err_create("could not init fs: failed to empty file", wrap_errno(errno));
        }
        full_size+=batch_size;
    }
    if (lseek(fd, 0, SEEK_SET) < 0) {
        return fs_err_create("failed to return fd", wrap_errno(errno));
    }


    sblock sb;
    sb.fs_num = FS_NUM;
    sb.block_size = 1024;
    sb.zmap_blocks = (fs_size - 10 * 1024 * 1024)/ (MBLOCK_ITEMS * 1024);
    sb.n_inodes = 10 * 1024 * 1024 * sizeof(inode) / MBLOCK_ITEMS;
    sb.imap_blocks = sb.n_inodes / MBLOCK_ITEMS;
    sb.max_size = fs_size;

    layout l = layout_init(sb);
    size_t buf_size = layout_size(sb);
    buf = (uint8_t*)malloc(buf_size);
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

layout parse_layout(int fd, fs_error *err) {
    layout l;
    uint8_t enc_data[1024];
    ssize_t r_bytes = read(fd, &enc_data, 1024);
    if (r_bytes == -1) {
        *err = fs_err_create("failed to read superblock", wrap_errno(errno));
        return l;
    }
    if (r_bytes < 1024) {
        *err = fs_err_create("corrupt file: size less than 1024 bytes", wrap_errno(0));
        return l;
    }

    sblock_bytes sbb;
    memset(sbb.data, 0, SBLOCK_SIZE * sizeof(uint8_t));
    memcpy(sbb.data, enc_data, r_bytes);
    sblock sb = sblock_decode(sbb);
    if (sb.fs_num != FS_NUM) {
        *err = fs_err_create("corrupt VFS: not a BDSM filesystem", wrap_errno(0));
        return l;
    }

    l = layout_recreate(sb);
    size_t mb_size = layout_size(sb) - 1024;
    uint8_t *mb_buf = (uint8_t*)malloc(mb_size);
    r_bytes = read(fd, mb_buf, mb_size);
    if (r_bytes == -1) {
        *err = fs_err_create("failed to read layout", wrap_errno(errno));
        return l;
    }
    if ((size_t)r_bytes < mb_size) {
        *err = fs_err_create("corrupt file: size less than required for layout", wrap_errno(0));
        return l;
    }
    layout_extend(&l, mb_buf);
    return l;
}

fs_error bdsm_fsck(char *fs_file) {
    int fd = open(fs_file, O_RDWR);
    if (fd == -1) {
        return fs_err_create("failed to open VFS file", wrap_errno(errno));
    }

    fs_error err = fs_no_err();
    layout l = parse_layout(fd, &err);
    if (err.errnum != 0) {
        return err;
    }
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

    fs_error err = fs_no_err();
    layout l = parse_layout(fd, &err);
    if (err.errnum != 0) {
        return err;
    }

    res->block_size = l.sb.block_size;
    res->max_size = l.sb.max_size;
    res->n_inodes = l.sb.n_inodes;
    res->mbi = l.sb.imap_blocks;
    res->mbz = l.sb.zmap_blocks;

    res->inodes = inode_vec_init(l.nodes.capacity);
    size_t i;
    for (i=0; i<l.nodes.size; i++) {
        inode node = inode_vec_get(l.nodes, i);
        if (node.mode != 0) {
            inode_vec_push(&res->inodes, node);
        }
    }
    // TODO: add more data

    layout_drop(&l);

    int c_ret = close(fd);
    if (c_ret == -1) {
        return fs_err_create("close VFS file err", wrap_errno(errno));
    }
    return fs_no_err();
}

void print_inode(inode n, dirent de) {
    char mstr[11];
    inode_mode_str(n.mode, mstr);

    time_t t = 0;
    memcpy(&t, &n.mtime, sizeof(time_t));
    char *t_str = asctime(gmtime(&t));
    if (t_str[strlen(t_str)-1] == '\n') {
        // asctime might put newline at the end
        t_str[strlen(t_str)-1] = '\0';
    }

    struct passwd *pwd = getpwuid(n.oid);
    struct group *grp = getgrgid(n.gid);

    if (pwd == NULL || grp == NULL) {
        printf("%s %d %d %d %ld %s %s\n",
            mstr,
            n.nr_links,
            n.oid,
            n.gid,
            n.size,
            t_str,
            de.name);
        return;
    }
    printf("%s %d %s %s %ld %s %s\n",
        mstr,
        n.nr_links,
        pwd->pw_name,
        grp->gr_name,
        n.size,
        t_str,
        de.name);
}

fs_error load_dirent_vec(int fd, layout *l, size_t inode_num, dirent_vec *dv) {
    inode node = inode_vec_get(l->nodes, inode_num);
    assert(inode_get_n_type(node.mode) == M_DIR);
    
    inode_descriptor idesc = idesc_create(*l, &node, &l->zones_mb, fd);
    uint8_t *buf = (uint8_t*)malloc(l->sb.block_size*sizeof(uint8_t));
    memset(buf, 0, l->sb.block_size*sizeof(uint8_t));
    ssize_t rres;
    while ((rres = inode_desc_read_block(&idesc, buf)) != 0) {
        if (rres < 0) {
            return fs_err_create("failed to read directory blocks", wrap_errno(errno));
        }
        bool is_end;
        size_t s_buf;
        for (s_buf=0; s_buf<(size_t)rres; s_buf+=sizeof(dirent)) {
            dirent_bytes db;
            memset(db.data, 0, sizeof(dirent));
            memcpy(db.data, buf, sizeof(dirent));
            dirent de = dirent_decode(db);
            if (strcmp(de.name, "") == 0) {
                is_end = true;
                break;
            }
            dirent_vec_push(dv, de);

            buf+=sizeof(dirent);
        }
        if (is_end) {
            break;
        }
        buf = (uint8_t*)malloc(l->sb.block_size*sizeof(uint8_t));
    }
    return fs_no_err();
}

struct resolve_res_str {
    dirent_vec dv;
    size_t pnode;
    size_t last_node;
    char *last_segment;
};

typedef struct resolve_res_str resolve_res;

resolve_res new_resolve_res(dirent_vec dv, size_t pnode, size_t last_node) {
    resolve_res res;
    res.dv = dv;
    res.pnode = pnode;
    res.last_node = last_node;
    return res;
}

resolve_res resolve_parent(int fd, char *path, layout *l, fs_error *err) {
    dirent_vec last_dv = dirent_vec_init(5);

    char **segments = (char**)malloc(100*sizeof(char*));
    size_t path_size = fs_path_split(path, &segments);
    if (fs_path_errno != 0) {
        *err = fs_err_create("failed to split path", wrap_errno(fs_path_errno));
        return new_resolve_res(last_dv, 0, 0);
    }

    resolve_res res = new_resolve_res(last_dv, 0, 0);
    *err = load_dirent_vec(fd, l, res.pnode, &res.dv);
    if (err->errnum != 0) {
        return res;
    }
    if (path_size > 0) {
        res.last_segment = segments[path_size-1];

        size_t i;
        for (i=0; i<path_size-1; i++) {
            size_t i_ent, prev_inode = res.pnode;
            for (i_ent=0; i_ent<res.dv.size; i_ent++) {
                if (strcmp(dirent_vec_get(res.dv, i_ent).name, segments[i]) == 0) {
                res.pnode = dirent_vec_get(res.dv, i_ent).inode_nr;
                    break;
                }
            }
            if (prev_inode == res.pnode) {
                *err = fs_err_create("path not found", wrap_errno(0));
                return res;
            }

            inode node = inode_vec_get(l->nodes, res.pnode);
            if (inode_get_n_type(node.mode) != M_DIR) {
                *err = fs_err_create("failed to list dir: path contains non-dir segements", wrap_errno(0));
                return res;
            }

            inode_descriptor idesc = idesc_create(*l, &node, &l->zones_mb, fd);
            dirent_vec dv = dirent_vec_init(5);

            uint8_t *buf = (uint8_t*)malloc(l->sb.block_size*sizeof(uint8_t));
            ssize_t rres;
            while ((rres = inode_desc_read_block(&idesc, buf)) != 0) {
                if (rres < 0) {
                    *err = fs_err_create("failed to read directory blocks", wrap_errno(errno));
                    return res;
                }
                bool is_end;
                size_t s_buf;
                for (s_buf=0; s_buf<(size_t)rres; s_buf+=sizeof(dirent)) {
                    dirent_bytes db;
                    memcpy(db.data, buf, sizeof(dirent));
                    dirent de = dirent_decode(db);
                    if (strcmp(de.name, "") == 0) {
                        is_end = true;
                        break;
                    }
                    dirent_vec_push(&dv, de);

                    buf+=sizeof(dirent);
                }
                if (is_end) {
                    break;
                }

                buf = (uint8_t*)malloc(l->sb.block_size*sizeof(uint8_t));
            }

            free(res.dv.entries);
            res.dv = dv;
        }

        for(i=0; i<res.dv.size; i++) {
            dirent de = dirent_vec_get(res.dv, i);
            if (strcmp(de.name, segments[path_size-1]) == 0) {
                res.last_node = de.inode_nr;
            }
        }
    }

    return res;
}

fs_error bdsm_lsobj(char *fs_file, char *obj_path) {
    int fd = open(fs_file, O_RDWR);
    if (fd == -1) {
        return fs_err_create("failed to open VFS file", wrap_errno(errno));
    }

    fs_error err = fs_no_err();
    layout l = parse_layout(fd, &err);
    if (err.errnum != 0) {
        return err;
    }

    err = fs_no_err();
    resolve_res res = resolve_parent(fd, obj_path, &l, &err);
    if (err.errnum != 0) {
        return err;
    }

    dirent de;
    de.name[0] = '\0';
    if (res.last_node != 0) {
        size_t i;
        for (i=0; i<res.dv.size; i++) {
            dirent tmp = dirent_vec_get(res.dv, i);
            if (strcmp(tmp.name, res.last_segment) == 0) {
                de = tmp;
                break;
            }
        }
    } else {
        de.name[0] = '+';
        de.name[1] = '\0';
        de.inode_nr = 0;
    }
    
    if (strlen(de.name) == 0) {
        return fs_err_create("file not found", wrap_errno(0));
    }

    print_inode(inode_vec_get(l.nodes, res.last_node), de);

    int c_ret = close(fd);
    if (c_ret == -1) {
        return fs_err_create("close VFS file err", wrap_errno(errno));
    }
    return fs_no_err();
}

fs_error flush_dv(int fd, layout *l, inode *parent, dirent_vec dv) {
    parent->size = 0;
    inode_descriptor pdesc = idesc_create(*l, parent, &l->zones_mb, fd);

    uint16_t batch_size = l->sb.block_size / sizeof(dirent);
    dirent_vec batch = dirent_vec_init(batch_size);
    size_t i;
    for (i=0; i<dv.size; i++) {
        dirent_vec_push(&batch, dirent_vec_get(dv, i));
        // on batch size or last element
        if (batch.size == batch_size || i == dv.size - 1) {
            uint8_t *buf = (uint8_t*)malloc(l->sb.block_size);
            memset(buf, 0, l->sb.block_size);
            uint16_t offset = 0;
            size_t j;
            for (j=0; j<batch.size; j++) {
                dirent_bytes db = dirent_encode(dirent_vec_get(batch, j));
                memcpy(buf+offset, db.data, sizeof(dirent));
                offset+=sizeof(dirent);
            }
            ssize_t wres = inode_desc_write_block(&pdesc, buf);
            if (wres < l->sb.block_size) {
                return fs_err_create("invalid block write", wrap_errno(errno));
            }
            parent->size += (uint64_t)wres;
            free(batch.entries);
            batch = dirent_vec_init(batch_size);
        }
    }

    return fs_no_err();
}

fs_error add_dirent(int fd, layout *l, uint32_t pnode, dirent d) {
    inode parent = inode_vec_get(l->nodes, pnode);
    if (inode_get_n_type(parent.mode) != M_DIR) {
        return fs_err_create("could not add dirent: parent should be a directory", wrap_errno(0));
    }

    dirent_vec dv = dirent_vec_init(5);
    fs_error err = load_dirent_vec(fd, l, pnode, &dv);
    if (err.errnum != 0) {
        return err;
    }
    size_t i;
    for (i=0; i<dv.size; i++) {
        dirent de = dirent_vec_get(dv, i);
        if (strcmp(de.name, d.name) == 0) {
            // dirent already exists, nothing to do
            return fs_no_err();
        }
    }
    dirent_vec_push(&dv, d);

    err = flush_dv(fd, l, &parent, dv);
    if (err.errnum != 0) {
        return err;
    }

    free(dv.entries);
    // update the parent reference
    inode_vec_set(&l->nodes, parent, pnode);
    return fs_no_err();
}

fs_error bdsm_lsdir(char *fs_file, char *dir_path) {
    int fd = open(fs_file, O_RDWR);
    if (fd == -1) {
        return fs_err_create("failed to open VFS file", wrap_errno(errno));
    }

    fs_error err = fs_no_err();
    layout l = parse_layout(fd, &err);
    if (err.errnum != 0) {
        return err;
    }

    err = fs_no_err();
    resolve_res res = resolve_parent(fd, dir_path, &l, &err);
    if (err.errnum != 0) {
        return err;
    }

    inode node = inode_vec_get(l.nodes, res.last_node);
    if (inode_get_n_type(node.mode) != M_DIR) {
        return fs_err_create("could not list: not a directory", wrap_errno(0));
    }

    dirent_vec dv = dirent_vec_init(5);
    err = load_dirent_vec(fd, &l, res.last_node, &dv);
    if (err.errnum != 0) {
        return err;
    }

    size_t i;
    for (i=0; i<dv.size; i++) {
        dirent de = dirent_vec_get(dv, i);
        print_inode(inode_vec_get(l.nodes, de.inode_nr), de);
    }
    free(dv.entries);
    int c_ret = close(fd);
    if (c_ret == -1) {
        return fs_err_create("failed to close VFS file", wrap_errno(errno));
    }
    return fs_no_err();
}

fs_error bdsm_stat(char *fs_file, char *obj_path) {
    int fd = open(fs_file, O_RDWR);
    if (fd == -1) {
        return fs_err_create("failed to open VFS file", wrap_errno(errno));
    }

    fs_error err = fs_no_err();
    layout l = parse_layout(fd, &err);
    if (err.errnum != 0) {
        return err;
    }

    err = fs_no_err();
    resolve_res res = resolve_parent(fd, obj_path, &l, &err);
    if (err.errnum != 0) {
        return err;
    }

    inode n = inode_vec_get(l.nodes, res.last_node);
    char mstr[11];
    inode_mode_str(n.mode, mstr);

    time_t t = 0;
    memcpy(&t, &n.mtime, sizeof(time_t));
    char *t_str = asctime(gmtime(&t));
    if (t_str[strlen(t_str)-1] == '\n') {
        // asctime might put newline at the end
        t_str[strlen(t_str)-1] = '\0';
    }

    printf("%s %d %d %d %ld %s\n",
        mstr,
        n.nr_links,
        n.oid,
        n.gid,
        n.size,
        t_str);
    printf("\tzones -> [");
    size_t i;
    for (i=0; i<ZONES_SIZE-1; i++) {
        printf("%d,", n.zones[i]);
    }
    printf("%d]\n", n.zones[ZONES_SIZE-1]);

    int c_ret = close(fd);
    if (c_ret == -1) {
        return fs_err_create("failed to close VFS file", wrap_errno(errno));
    }
    return fs_no_err();
}

fs_error bdsm_mkdir(char *fs_file, char *dir_path) {
    int fd = open(fs_file, O_RDWR);
    if (fd == -1) {
        return fs_err_create("failed to open VFS file", wrap_errno(errno));
    }

    fs_error err = fs_no_err();
    layout l = parse_layout(fd, &err);
    if (err.errnum != 0) {
        return err;
    }

    resolve_res res = resolve_parent(fd, dir_path, &l, &err);
    if (err.errnum != 0) {
        return err;
    }

    size_t i;
    for (i=0; i<res.dv.size; i++) {
        if (strcmp(dirent_vec_get(res.dv, i).name, res.last_segment) == 0) {
            return fs_err_create("file or directory already exists", wrap_errno(0));
        }
    }

    dirent d;
    strncpy(d.name, res.last_segment, DIRENT_NAME_SIZE);
    d.inode_nr = mblock_vec_take_first(&l.inode_mb);
    if (d.inode_nr <= 0) {
        return fs_err_create("could not allocate more inodes", wrap_errno(0));
    }
    dirent_vec_push(&res.dv, d);

    inode n;
    n.mode = 0;
    inode_set_mode(&n, M_READ|M_WRITE|M_EXEC, M_READ|M_EXEC, M_READ|M_EXEC, M_DIR);
    n.nr_links = 1;
    n.size = 0;
    n.oid = 0;
    n.gid = 0;

    time_t now = time(NULL);
    memcpy(&n.mtime, &now, sizeof(n.mtime));

    memset(n.zones, 0, ZONES_SIZE*sizeof(uint32_t));
    inode_vec_set(&l.nodes, n, d.inode_nr);

    err = add_dirent(fd, &l, res.pnode, d);
    if (err.errnum != 0) {
        return err;
    }

    size_t buf_size = layout_size(l.sb);
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
    int fd = open(fs_file, O_RDWR);
    if (fd == -1) {
        return fs_err_create("failed to open VFS file", wrap_errno(errno));
    }

    fs_error err = fs_no_err();
    layout l = parse_layout(fd, &err);
    if (err.errnum != 0) {
        return err;
    }

    resolve_res res = resolve_parent(fd, dir_path, &l, &err);
    if (err.errnum != 0) {
        return err;
    }

    dirent d;
    d.name[0] = '\0';
    size_t i, d_ind;
    for (i=0; i<res.dv.size; i++) {
        dirent tmp = dirent_vec_get(res.dv, i);
        if (strcmp(tmp.name, res.last_segment) == 0) {
            d = tmp;
            d_ind = i;
            break;
        }
    }
    if (strlen(d.name) == 0) {
        return fs_err_create("directory does not exist", wrap_errno(0));
    }

    inode node = inode_vec_get(l.nodes, d.inode_nr);
    if (inode_get_n_type(node.mode) != M_DIR) {
        return fs_err_create("file not a directory", wrap_errno(0));
    }

    dirent_vec dv = dirent_vec_init(5);
    err = load_dirent_vec(fd, &l, res.last_node, &dv);
    if (err.errnum != 0) {
        return err;
    }
    if (dv.size != 0) {
        return fs_err_create("directory not empty", wrap_errno(0));
    }
    free(dv.entries);

    // remove dirent
    dirent_vec_remove(&res.dv, d_ind);
    dirent dummy;
    dummy.inode_nr = 0;
    dummy.name[0] = '\0';
    dirent_vec_push(&res.dv, dummy);

    inode parent = inode_vec_get(l.nodes, res.pnode);
    err = flush_dv(fd, &l, &parent, res.dv);
    if (err.errnum != 0) {
        return err;
    }

    inode_descriptor desc = idesc_create(l, &node, &l.zones_mb, fd);
    if (inode_desc_reset_zones(&desc) < 0) {
        return fs_err_create("could not delete zones", wrap_errno(errno));
    }

    // remove inode
    node.mode = 0;
    node.nr_links = 0;
    node.size = 0;

    inode_vec_set(&l.nodes, node, d.inode_nr);
    mblock_vec_unset(&l.inode_mb, d.inode_nr);

    size_t buf_size = layout_size(l.sb);
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

fs_error cpy_fs_vfs(char *fs_file, char *in, char *out) {
    int fd = open(fs_file, O_RDWR);
    if (fd == -1) {
        return fs_err_create("failed to open VFS file", wrap_errno(errno));
    }

    fs_error err = fs_no_err();
    layout l = parse_layout(fd, &err);
    if (err.errnum != 0) {
        return err;
    }

    int in_fd = open(in, O_RDONLY);
    if (in_fd == -1) {
        return fs_err_create("failed to open input file", wrap_errno(errno));
    }

    struct stat in_s;
    if (fstat(in_fd, &in_s) < 0) {
        return fs_err_create("failed to stat input file", wrap_errno(errno));
    }

    resolve_res res = resolve_parent(fd, out, &l, &err);
    if (err.errnum != 0) {
        return err;
    }

    size_t inode_num = 0;
    size_t i;
    for (i=0; i<res.dv.size; i++) {
        dirent de = dirent_vec_get(res.dv, i);
        if (strcmp(de.name, res.last_segment) == 0) {
            inode_num = de.inode_nr;
            break;
        }
    }

    if (inode_num == 0) {
        uint8_t umode = 00;
        if ((in_s.st_mode | S_IRUSR) == in_s.st_mode) {
            umode |= M_READ;
        }
        if ((in_s.st_mode | S_IWUSR) == in_s.st_mode) {
            umode |= M_WRITE;
        }
        if ((in_s.st_mode | S_IXUSR) == in_s.st_mode) {
            umode |= M_EXEC;
        }

        uint8_t gmode = 00;
        if ((in_s.st_mode | S_IRGRP) == in_s.st_mode) {
            gmode |= M_READ;
        }
        if ((in_s.st_mode | S_IWGRP) == in_s.st_mode) {
            gmode |= M_WRITE;
        }
        if ((in_s.st_mode | S_IXGRP) == in_s.st_mode) {
            gmode |= M_EXEC;
        }

        uint8_t amode = 00;
        if ((in_s.st_mode | S_IROTH) == in_s.st_mode) {
            amode |= M_READ;
        }
        if ((in_s.st_mode | S_IWOTH) == in_s.st_mode) {
            amode |= M_WRITE;
        }
        if ((in_s.st_mode | S_IXOTH) == in_s.st_mode) {
            amode |= M_EXEC;
        }

        inode n;
        n.mode = 0;
        inode_set_mode(&n, umode, gmode, amode, M_FILE);
        n.nr_links = 0;
        n.size = in_s.st_size;
        n.oid = in_s.st_uid & 0xFFFF;
        n.gid = in_s.st_gid & 0xFFFF;

        time_t now = time(NULL);
        memcpy(&n.mtime, &now, sizeof(n.mtime));
        memset(n.zones, 0, ZONES_SIZE*sizeof(uint32_t));

        int reserved = mblock_vec_take_first(&l.inode_mb);
        if (reserved <= 0) {
            return fs_err_create("failed to create file: no inodes left", wrap_errno(0));
        }
        inode_vec_set(&l.nodes, n, reserved);
        inode_num = reserved;
    }

    inode n = inode_vec_get(l.nodes, inode_num);
    if (inode_get_n_type(n.mode) != M_FILE) {
        return fs_err_create("failed to copy: object not a file", wrap_errno(0));
    }

    n.size = in_s.st_size;
    inode_descriptor ndesc = idesc_create(l, &n, &l.zones_mb, fd);

    int64_t bytes_to_write = in_s.st_size;
    uint8_t *buf = (uint8_t*)malloc(l.sb.block_size);
    while (bytes_to_write > 0) {
        memset(buf, 0, l.sb.block_size);

        if (read(in_fd, buf, l.sb.block_size) <= 0) {
            return fs_err_create("failed to read input file", wrap_errno(errno));
        }
        if (inode_desc_write_block(&ndesc, buf) <= 0) {
            return fs_err_create("failed to write VFS file", wrap_errno(0));
        }
        bytes_to_write -= l.sb.block_size;
    }
    free(buf);
    inode_vec_set(&l.nodes, n, inode_num);

    dirent d;
    d.inode_nr = inode_num;
    memset(d.name, 0, DIRENT_NAME_SIZE);
    strncpy(d.name, res.last_segment, DIRENT_NAME_SIZE);
    err = add_dirent(fd, &l, res.pnode, d);
    if (err.errnum != 0) {
        return err;
    }

    size_t buf_size = layout_size(l.sb);
    buf = (uint8_t*)malloc(buf_size);
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

    if (close(fd) == -1) {
        return fs_err_create("failed to close VFS file", wrap_errno(errno));
    }
    if (close(in_fd) == -1) {
        return fs_err_create("failed to close input file", wrap_errno(0));
    }
    return fs_no_err();

}

fs_error cpy_vfs_fs(char *fs_file, char *in, char *out) {
    int fd = open(fs_file, O_RDWR);
    if (fd == -1) {
        return fs_err_create("failed to open VFS file", wrap_errno(errno));
    }

    fs_error err = fs_no_err();
    layout l = parse_layout(fd, &err);
    if (err.errnum != 0) {
        return err;
    }

    int out_fd = open(out, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if (out_fd == -1) {
        return fs_err_create("failed to open output file", wrap_errno(errno));
    }

    resolve_res res = resolve_parent(fd, in, &l, &err);
    if (err.errnum != 0) {
        return err;
    }
    inode node = inode_vec_get(l.nodes, res.last_node);
    if (inode_get_n_type(node.mode) != M_FILE) {
        return fs_err_create("could not copy: object not a file", wrap_errno(0));
    }

    inode_descriptor idesc = idesc_create(l, &node, &l.zones_mb, fd);
    int64_t size = node.size;
    while (size > 0) {
        uint8_t *buf = (uint8_t*)malloc(l.sb.block_size);
        if (inode_desc_read_block(&idesc, buf) <= 0) {
            return fs_err_create("failed to read virtual file", wrap_errno(0));
        }

        ssize_t wres;
        if (l.sb.block_size > size) {
            wres = write(out_fd, buf, size);
        } else {
            wres = write(out_fd, buf, l.sb.block_size);
        }
        if (wres <= 0) {
            return fs_err_create("failed to write output file", wrap_errno(errno));
        }
        size -= l.sb.block_size;
    }

    if (close(fd) == -1) {
        return fs_err_create("failed to close VFS file", wrap_errno(errno));
    }
    return fs_no_err();
}

fs_error bdsm_cpfile(char *fs_file, char *path1, char *path2) {
    if (is_fs_path(path1) == 0) {
        return cpy_vfs_fs(fs_file, path1, path2);
    }
    return cpy_fs_vfs(fs_file, path1, path2);
}

fs_error bdsm_rmfile(char *fs_file, char *file_path) {
    int fd = open(fs_file, O_RDWR);
    if (fd == -1) {
        return fs_err_create("failed to open VFS file", wrap_errno(errno));
    }

    fs_error err = fs_no_err();
    layout l = parse_layout(fd, &err);
    if (err.errnum != 0) {
        return err;
    }

    resolve_res res = resolve_parent(fd, file_path, &l, &err);
    if (err.errnum != 0) {
        return err;
    }
    dirent d;
    d.name[0] = '\0';
    size_t i, d_ind;
    for (i=0; i < res.dv.size; i++) {
        dirent tmp = dirent_vec_get(res.dv, i);
        if (strcmp(tmp.name, res.last_segment) == 0) {
            d = tmp;
            d_ind = i;
            break;
        }
    }
    if (strlen(d.name) == 0) {
        return fs_err_create("could not find file", wrap_errno(0));
    }

    inode node = inode_vec_get(l.nodes, res.last_node);
    if (inode_get_n_type(node.mode) != M_FILE) {
        return fs_err_create("could not remove: object not a file", wrap_errno(0));
    }

    inode_descriptor ndesc = idesc_create(l, &node, &l.zones_mb, fd);
    if (inode_desc_reset_zones(&ndesc) < 0) {
        return fs_err_create("failed to delete zones", wrap_errno(0));
    }
    mblock_vec_unset(&l.inode_mb, res.last_node);
    node.mode = 0;
    node.nr_links = 0;
    node.size = 0;
    node.oid = 0;
    node.gid = 0;
    node.mtime = 0;
    inode_vec_set(&l.nodes, node, res.last_node);


    // remove dirent
    dirent_vec_remove(&res.dv, d_ind);
    dirent dummy;
    dummy.inode_nr = 0;
    dummy.name[0] = '\0';
    dirent_vec_push(&res.dv, dummy);

    inode parent = inode_vec_get(l.nodes, res.pnode);
    err = flush_dv(fd, &l, &parent, res.dv);
    if (err.errnum != 0) {
        return err;
    }

    size_t buf_size = layout_size(l.sb);
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

    if (close(fd) == -1) {
        return fs_err_create("failed to close VFS file", wrap_errno(errno));
    }
    return fs_no_err();
}
