#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "inode_vec.h"
#include "inode.h"

inode_vec inode_vec_init(size_t cap) {
    inode_vec v;
    v.size = 0;
    v.nodes = (inode*)malloc(cap*sizeof(inode));
    v.capacity = cap;
    return v;
}

void inode_vec_push(inode_vec *v, inode n) {
    if (v->size == v->capacity) {
        v->nodes = (inode*)realloc(v->nodes, (v->capacity*2)*sizeof(inode));
        v->capacity*=2;
    }
    v->nodes[v->size] = n;
    v->size++;
}

inode inode_vec_get(inode_vec v, size_t i) {
    assert(v.size >= i);
    return v.nodes[i];
}

void inode_vec_set(inode_vec *v, inode n, size_t i) {
    assert(v->size >= i);
    v->nodes[i] = n;
}

void inode_vec_drop(inode_vec *v) {
    free(v->nodes);
}
