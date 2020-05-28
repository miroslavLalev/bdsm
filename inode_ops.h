#ifndef INODE_OPS_H
#define INODE_OPS_H

#include "inode.h"

inode_bytes inode_encode(inode n);

inode inode_decode(inode_bytes nb);

#endif