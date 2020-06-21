#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "dirent.h"
#include "dirent_vec.h"

dirent_vec dirent_vec_init(size_t cap) {
    dirent_vec dv;
    dv.size = 0;
    dv.entries = (dirent*)malloc(cap * sizeof(dirent));
    memset(dv.entries, 0, cap * sizeof(dirent));
    dv.capacity = cap;
    return dv;
}

void dirent_vec_push(dirent_vec *v, dirent d) {
    if (v->size == v->capacity) {
        v->entries = (dirent*)realloc(v->entries, (v->capacity*2)*sizeof(dirent));
        v->capacity*=2;
    }
    v->entries[v->size] = d;
    v->size++;
}

dirent dirent_vec_remove(dirent_vec *v, size_t i) {
    assert(v->size >= i);
    dirent d = v->entries[i];
    v->size--;
    dirent *new_nodes = (dirent*)malloc((v->capacity)*sizeof(dirent));
    memset(new_nodes, 0, (v->capacity)*sizeof(dirent));
    if (v->size == 0) {
        free(v->entries);
        v->entries = new_nodes;
        return d;
    }

    if (i == 0) {
        // [1, n)
        memcpy(new_nodes, &(v->entries[1]), v->size*sizeof(dirent));
    } else if (i == v->size) {
        // [0, n-1)
        memcpy(new_nodes, v->entries, v->size*sizeof(dirent));
    } else {
        // [0, k) + (k, n)
        memcpy(new_nodes, v->entries, i*sizeof(dirent));
        memcpy(&new_nodes[i], &(v->entries[i+1]), (v->size-i)*sizeof(dirent));
    }
    free(v->entries);
    v->entries = new_nodes;
    return d;
}

dirent dirent_vec_get(dirent_vec v, size_t i) {
    assert(v.size >= i);
    return v.entries[i];
}
