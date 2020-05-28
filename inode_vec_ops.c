#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
        inode *new_nodes = (inode*)malloc((v->capacity*2)*sizeof(inode));
        memcpy(new_nodes, v->nodes, v->size);
        free(v->nodes);
        v->nodes = new_nodes;
        v->capacity*=2;
    }
    v->nodes[v->size+1] = n;
    v->size++;
}

inode inode_vec_remove(inode_vec *v, size_t i) {
    assert(v->size >= i);
    inode n = v->nodes[i];
    v->size--;
    inode *new_nodes = (inode*)malloc((v->capacity)*sizeof(inode));
    if (v->size == 0) {
        free(v->nodes);
        v->nodes = new_nodes;
        return n;
    }

    if (i == 0) {
        memcpy(new_nodes, v->nodes+1, v->size);
    } else if (i == v->size + 1) {
        memcpy(new_nodes, v->nodes, v->size);
    } else {
        memcpy(new_nodes, v->nodes, i);
        memcpy(new_nodes+i, v->nodes+i+1, v->size-i);
    }
    free(v->nodes);
    v->nodes = new_nodes;
    return n;
}

inode inode_vec_get(inode_vec v, size_t i) {
    assert(v.size >= i);
    return v.nodes[i];
}

void inode_vec_drop(inode_vec *v) {
    free(v->nodes);
}