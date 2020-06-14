#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "dirent.h"
#include "encutils.h"

dirent_bytes dirent_encode(dirent d) {
    dirent_bytes db;
    size_t offset = 0;
    enc_u32(d.inode_nr, db.data, &offset);
    enc_str(d.name, DIRENT_NAME_SIZE, db.data, &offset);
    return db;
}

dirent dirent_decode(dirent_bytes db) {
    dirent d;
    size_t offset = 0;
    d.inode_nr = dec_u32(db.data, &offset);

    char *name_str = dec_str(db.data, &offset, DIRENT_NAME_SIZE);
    memcpy(d.name, name_str, DIRENT_NAME_SIZE);
    return d;
}
