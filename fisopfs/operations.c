#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fuse.h>
#include <stdbool.h>
#include <linux/limits.h>

#include "defs.h"

extern filesystem fs;

int
search_inodo(const char *path, inodo *result)
{
	char path_copy[PATH_MAX];
	strncpy(path_copy, path, sizeof(path_copy));
	path_copy[PATH_MAX - 1] = '\0';

	char *read_path = path_copy;
	read_path++;  // Skip the leading '/'
	char *ptr;

	inodo *current = fs.root;
	if (strcmp(read_path, "") == 0) {
		// If the path is just "/", return the root inodo
		*result = *current;
		return EXIT_SUCCESS;  // Found the inodo
	}
	while ((ptr = strchr(read_path, '/')) != NULL) {
		*ptr = '\0';

		bool found = false;
		if (current->dir == NULL) {
			fprintf(stderr, "Error: Not a directory: %s\n", read_path);
			return -ENOTDIR;  // Not a directory
		}

		for (int i = 0; i < current->dir->size; i++) {
			if (strcmp(read_path,
			           current->dir->entries[i]->nombre) == 0) {
				found = true;
				current = current->dir->entries[i]->inodo;  // Move to the next inodo
				break;
			}
		}
		if (!found) {
			fprintf(stderr,
			        "Error: Directory not found in path: %s\n",
			        read_path);
			return -ENOENT;  // No such file or directory
		}
		read_path = ptr + 1;  // Move to the next part of the path
	}
	// After processing all parts of the path, check if we are at a file or directory
	if (strlen(read_path) != 0) {
		// If there's a final part of the path, check if it exists
		// this is a file or directory at the end of the path
		bool found = false;
		if (current->dir == NULL) {
			fprintf(stderr, "Error: Not a directory: %s\n", read_path);
			return -ENOTDIR;  // Not a directory
		}

		for (int i = 0; i < current->dir->size; i++) {
			if (strcmp(read_path,
			           current->dir->entries[i]->nombre) == 0) {
				found = true;
				current = current->dir->entries[i]->inodo;  // Move to the next inodo
				break;
			}
		}
		if (!found) {
			fprintf(stderr,
			        "Error: Directory not found in path: %s\n",
			        read_path);
			return -ENOENT;  // No such file or directory
		}
	}

	*result = *current;

	return EXIT_SUCCESS;
}

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
	// ......
	printf("Filesystem initialized successfully.\n");
	return &fs;
}

int
filesystem_getattr(const char *path, struct stat *stbuf)
{
	printf("Getting attributes for: %s\n", path);
	inodo *inode = NULL;
	int ret = search_inodo(path, inode);
	if (ret != EXIT_SUCCESS || !inode) {
		fprintf(stderr, "Error: Inode not found for path: %s\n", path);
		return -ENOENT;  // No such file or directory
	}

	memset(stbuf, 0, sizeof(struct stat));  // Clear the stat structure
	// Set the basic attributes
	stbuf->st_mode = inode->mode;    // Set the mode
	stbuf->st_uid = inode->uid;      // User ID of owner
	stbuf->st_gid = inode->gid;      // Group ID of owner
	stbuf->st_atime = inode->atime;  // Last access time
	stbuf->st_mtime = inode->mtime;  // Last modification time
	stbuf->st_ctime = inode->ctime;  // Creation time
	stbuf->st_nlink = inode->nlink;  // Number of links
	stbuf->st_size = inode->size;    // Size in bytes

	return EXIT_SUCCESS;  // Success
}

int
filesystem_mkdir(const char *path, mode_t mode)
{
	printf("Creating directory: %s\n", path);

	/*
	 * First, we separate the path into the new directory name and the
	 * parent directory. We assume the path is in the format
	 * "/parent/new_directory". If the path is just "/", we return an error
	 * because we cannot create a directory at the root. If the path is
	 * invalid (e.g., "/new_directory/"), we also return an error. We will
	 * use the last slash to determine the parent directory and the new
	 * directory name.
	 */
	char new_directory[MAX_FILENAME];
	char path_copy[PATH_MAX];

	char *last_slash = strrchr(path, '/');

	if (last_slash == NULL || last_slash == path) {
		fprintf(stderr, "Error: Invalid path: %s\n", path);
		return -ENOENT;  // No such file or directory
	}
	// Calcular longitudes
	size_t dir_len = last_slash - path;
	strncpy(path_copy, path, dir_len);
	new_directory[dir_len] = '\0';
	strcpy(new_directory, last_slash + 1);


	/*
	 * Now we have the parent directory in `path_copy` and the new directory
	 * name in `new_directory`. We will search for the parent directory in
	 * the filesystem and create a new entry for the new directory. If the
	 * parent directory does not exist, we return an error. If the new
	 * directory already exists, we also return an error.
	 */


	inodo *dir = NULL;
	int ret = search_inodo(path_copy, dir);

	if (ret != EXIT_SUCCESS || !dir || !dir->dir) {
		fprintf(stderr, "Error: Parent directory not found or is not a directory.\n");
		return -ENOENT;
	} else if (dir->dir->size >= MAX_DENTRIES) {
		fprintf(stderr, "Error: Parent directory is full, cannot create new directory.\n");
		return -ENOSPC;  // No space left on device
	} else {
		// Check if the new directory already exists in the parent directory
		for (int i = 0; i < dir->dir->size; i++) {
			if (strcmp(dir->dir->entries[i]->nombre,
			           new_directory) == 0) {
				fprintf(stderr,
				        "Error: Directory already exists: %s\n",
				        new_directory);
				return -EEXIST;  // File exists
			}
		}
	}

	inodo_dir *directory = dir->dir;

	// create the directory entry
	dentry *new_entry = malloc(sizeof(dentry));
	strncpy(new_entry->nombre, new_directory, MAX_FILENAME - 1);
	new_entry->nombre[MAX_FILENAME - 1] = '\0';  // Ensure null termination
	new_entry->inodo =
	        malloc(sizeof(inodo));  // Allocate memory for the new inodo
	new_entry->inodo->file = NULL;  // New directory has no file content
	new_entry->inodo->dir = malloc(sizeof(inodo_dir));
	new_entry->inodo->dir->size = 0;  // New directory starts empty

	// Set the inodo fields for the new directory
	directory->entries[directory->size] = new_entry;  // Add the new entry
	directory->size++;    // Increment the size of the directory
	return EXIT_SUCCESS;  // Success
}

int
filesystem_readdir(const char *path,
                   void *buf,
                   fuse_fill_dir_t filler,
                   off_t offset,
                   struct fuse_file_info *fi)
{
	printf("Reading directory: %s\n", path);
	inodo *directory = NULL;
	int ret = search_inodo(path, directory);
	if (ret != EXIT_SUCCESS || !directory || !directory->dir) {
		fprintf(stderr, "Error: Parent directory not found or is not a directory.\n");
		return -ENOENT;
	}

	// TODO: where is NULL back it, we should put the stat
	filler(buf, ".", NULL, 0);   // Add current directory
	filler(buf, "..", NULL, 0);  // Add parent directory

	for (int i = 0; i < directory->dir->size; i++) {
		filler(buf,
		       directory->dir->entries[i]->nombre,
		       NULL,
		       0);  // Add each entry in the directory
	}

	return EXIT_SUCCESS;  // Success
}
