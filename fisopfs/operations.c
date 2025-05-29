#include <stdio.h>
#include <errno.h>
#include "defs.h"

extern filesystem_t fs;

void *
filesystem_init(struct fuse_conn_info *conn)
{
	printf("Initializing filesystem...\n");
	// Initialize the filesystem structure
	fs.root = malloc(sizeof(inodo_t));
	fs.root->file = NULL;  // Root is a directory
	fs.root->dir = malloc(sizeof(inodo_dir_t));
	fs.root->dir->entries[0] = NULL;  // Initialize the root directory entries
	fs.root->size = 0;
	// ......
	printf("Filesystem initialized successfully.\n");
	return &fs;
}
