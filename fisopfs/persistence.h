#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <stdio.h>

#include "defs.h"

void serialize_inode(FILE *file, const inode *node);

inode *deserialize_inode(FILE *file);

#endif
