#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fuse.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include "defs.h"
#include <stdbool.h>
#include <linux/limits.h>

extern filesystem fs;

void *
filesystem_init(struct fuse_conn_info *conn)
{
	printf("Initializing filesystem...\n");
	// Initialize the filesystem structure
	fs.root = malloc(sizeof(inodo));
	fs.root->file = NULL;  // Root is a directory
	fs.root->dir = malloc(sizeof(inodo_dir));
	fs.root->dir->entries[0] = NULL;  // Initialize the root directory entries
	fs.root->dir->size = 0;           // No entries in the root directory
	fs.root->size = 0;
	// ......
	printf("Filesystem initialized successfully.\n");
	return &fs;
}

int
filesystem_mkdir(const char *path, mode_t mode)
{
	printf("Creating directory: %s\n", path);
	char path_copy[PATH_MAX];
	strncpy(path_copy, path, sizeof(path_copy));
	path_copy[PATH_MAX - 1] = '\0';

	char *read_path = path_copy;
	read_path++;  // Skip the leading '/'
	char *ptr;


	inodo_dir *directory = fs.root->dir;

	while ((ptr = strchr(read_path, '/')) != NULL) {
		*ptr = '\0';

		// if in the path there is a dot, we return an error because it is a file
		if (strrchr(read_path, '.') != NULL) {
			fprintf(stderr, "Error: Invalid directory name with dot in path: %s\n", read_path);
			return -EINVAL;  // Invalid argument
		}

		// search for the directory in the current directory
		bool found = false;
		for (int i = 0; i < directory->size; i++) {
			if (strcmp(read_path, directory->entries[i]->nombre) == 0 &&
			    directory->entries[i]->inodo->dir != NULL) {
				found = true;
				directory = directory->entries[i]->inodo->dir;  // Move to the next directory
				break;
			}
		}
		// if the directory is not found
		if (!found) {
			// if the next part of the path is empty, we create the directory
			if (strlen(ptr + 1) == 0) {
				break;
			} else {  // if the next part of the path is not empty, we return an error
				fprintf(stderr, "Error: Directory not found in path: %s\n", read_path);
				return -ENOENT;  // No such file or directory
			}
		}
		read_path = ptr + 1;  // Move to the next part of the path
	}

	// create the directory entry
	dentry *new_entry = malloc(sizeof(dentry));
	if (new_entry == NULL) {
		fprintf(stderr, "Error: Memory allocation failed for new directory entry.\n");
		return -ENOMEM;  // Out of memory
	}
	strncpy(new_entry->nombre, read_path, MAX_FILENAME - 1);
	new_entry->nombre[MAX_FILENAME - 1] = '\0';  // Ensure null termination
	new_entry->inodo = malloc(sizeof(inodo));
	if (!new_entry->inodo) {
		free(new_entry);
		fprintf(stderr, "Error: Memory allocation failed for inodo.\n");
		return -ENOMEM;  // Check memory allocation for inodo
	}

	new_entry->inodo->file = NULL;  // New directory has no file content
	new_entry->inodo->dir = malloc(sizeof(inodo_dir));
	if (!new_entry->inodo->dir) {
		free(new_entry->inodo);
		free(new_entry);
		fprintf(stderr, "Error: Memory allocation failed for new directory inode.\n");
		return -ENOMEM;  // Out of memory
	}
	new_entry->inodo->dir->size = 0;  // New directory starts empty

	directory->entries[directory->size] = new_entry;  // Add the new entry
	directory->size++;  // Increment the size of the directory
	return 0;           // Success
}
