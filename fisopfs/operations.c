#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>  // Needed for S_IFDIR
#include <time.h>
#include <stdarg.h>
#include <fuse.h>
#include <stdbool.h>
#include <linux/limits.h>
#include "defs.h"
#include <linux/stat.h>

extern filesystem fs;


// Functions and wrappers for the filesystem operations

// Convert inodo to stat structure
void
inodo_to_stat(const inodo *inode, struct stat *stbuf)
{
	memset(stbuf, 0, sizeof(struct stat));
	stbuf->st_mode = inode->mode;
	stbuf->st_uid = inode->uid;
	stbuf->st_gid = inode->gid;
	stbuf->st_atime = inode->atime;
	stbuf->st_mtime = inode->mtime;
	stbuf->st_ctime = inode->ctime;
	stbuf->st_nlink = inode->nlink;
	stbuf->st_size = inode->size;
}

// Search for an inodo by path
int
search_inodo(const char *path, inodo **result)
{
	char path_copy[PATH_MAX];
	strncpy(path_copy, path, sizeof(path_copy));
	path_copy[PATH_MAX - 1] = '\0';

	char *read_path = path_copy;
	read_path++;  // Skip the leading '/'
	char *ptr;


	inodo *current = fs.root;

	if (strlen(read_path) == 0) {
		// If the path is just "/", return the root inodo
		*result = current;
		return EXIT_SUCCESS;
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
				current = current->dir->entries[i]->inodo;
				break;
			}
		}
		if (!found) {
			fprintf(stderr,
			        "Error: Directory not found in path: %s\n",
			        read_path);
			return -ENOENT;  // No such file or directory
		}
		read_path = ptr + 1;
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
			fprintf(stderr, "Error: Directory or File not found in path: %s\n", read_path);
			return -ENOENT;  // No such file or directory
		}
	}

	*result = current;

	return EXIT_SUCCESS;
}

int
split_parent_child(const char *path, char *parent, char *child)
{
	char *last_slash = strrchr(path, '/');

	if (last_slash == NULL || strcmp(last_slash, "/") == 0) {
		fprintf(stderr, "Error: Invalid path: %s\n", path);
		return -ENOENT;  // No such file or directory
	}

	if (last_slash == path) {
		strcpy(parent, "/");
		parent[1] = '\0';
	} else {
		size_t dir_len = last_slash - path;
		strncpy(parent, path, dir_len);
		parent[dir_len] = '\0';
	}

	strcpy(child, last_slash + 1);
	return EXIT_SUCCESS;
}


// Filesystem functions


// Initialize the filesystem
void *
filesystem_init(struct fuse_conn_info *conn)
{
	fs.root = malloc(sizeof(inodo));
	fs.root->mode = __S_IFDIR |
	                0755;  // Set mode to directory with rwxr-xr-x permissions
	fs.root->nlink = 2;  // Root directory has 2 links (itself and its parent)
	fs.root->uid = getuid();
	fs.root->gid = getgid();
	fs.root->atime = time(NULL);
	fs.root->mtime = time(NULL);
	fs.root->ctime = time(NULL);
	fs.root->size = 0;


	fs.root->file = NULL;
	fs.root->dir = malloc(sizeof(inodo_dir));
	fs.root->dir->entries[0] = NULL;
	fs.root->dir->size = 0;
	return &fs;
}

int
filesystem_getattr(const char *path, struct stat *stbuf)
{
	inodo *inode = NULL;
	int ret = search_inodo(path, &inode);
	if (ret != EXIT_SUCCESS || !inode) {
		fprintf(stderr, "Error: Inode not found for path: %s\n", path);
		return -ENOENT;  // No such file or directory
	}

	memset(stbuf, 0, sizeof(struct stat));

	stbuf->st_mode = inode->mode;
	stbuf->st_uid = inode->uid;
	stbuf->st_gid = inode->gid;
	stbuf->st_atime = inode->atime;
	stbuf->st_mtime = inode->mtime;
	stbuf->st_ctime = inode->ctime;
	stbuf->st_nlink = inode->nlink;
	stbuf->st_size = inode->size;

	return EXIT_SUCCESS;
}

int
filesystem_mkdir(const char *path, mode_t mode)
{
	char new_directory[MAX_FILENAME];
	char path_copy[PATH_MAX];

	if (split_parent_child(path, path_copy, new_directory) != EXIT_SUCCESS) {
		return -ENOENT;  // No such file or directory
	}

	inodo *dir = NULL;
	int ret = search_inodo(path_copy, &dir);

	if (ret != EXIT_SUCCESS || !dir || !dir->dir) {
		fprintf(stderr, "Error: Parent directory not found or is not a directory.\n");
		return -ENOENT;
	} else if (dir->dir->size >= MAX_DENTRIES) {
		fprintf(stderr, "Error: Parent directory is full, cannot create new directory.\n");
		return -ENOSPC;  // No space left on device
	} else {
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


	dentry *new_entry = malloc(sizeof(dentry));
	strncpy(new_entry->nombre, new_directory, MAX_FILENAME - 1);
	new_entry->nombre[MAX_FILENAME - 1] = '\0';


	new_entry->inodo = malloc(sizeof(inodo));
	new_entry->inodo->mode =
	        __S_IFDIR |
	        0755;  // Set mode to directory with rwxr-xr-x permissions
	new_entry->inodo->nlink =
	        2;  // New directory has 2 links (itself and its parent)
	new_entry->inodo->uid = getuid();
	new_entry->inodo->gid = getgid();
	new_entry->inodo->atime = time(NULL);
	new_entry->inodo->mtime = time(NULL);
	new_entry->inodo->ctime = time(NULL);
	new_entry->inodo->size = 0;


	new_entry->inodo->file = NULL;
	new_entry->inodo->dir = malloc(sizeof(inodo_dir));
	new_entry->inodo->dir->size = 0;


	directory->entries[directory->size] = new_entry;
	directory->size++;
	return EXIT_SUCCESS;
}

int
filesystem_readdir(const char *path,
                   void *buf,
                   fuse_fill_dir_t filler,
                   off_t offset,
                   struct fuse_file_info *fi)
{
	inodo *directory = NULL;
	int ret = search_inodo(path, &directory);
	if (ret != EXIT_SUCCESS || !directory || !directory->dir) {
		fprintf(stderr, "Error: Parent directory not found or is not a directory.\n");
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	for (int i = 0; i < directory->dir->size; i++) {
		if (directory->dir->entries[i] == NULL) {
			continue;
		}
		struct stat stbuf;
		inodo_to_stat(directory->dir->entries[i]->inodo, &stbuf);

		filler(buf, directory->dir->entries[i]->nombre, &stbuf, 0);
	}

	return EXIT_SUCCESS;  // Success
}

int
filesystem_rmdir(const char *path)
{
	inodo *parent = NULL;
	char parent_path[PATH_MAX];
	char child_name[MAX_FILENAME];

	if (split_parent_child(path, parent_path, child_name) != EXIT_SUCCESS) {
		return -ENOENT;  // No such file or directory
	}


	int ret = search_inodo(parent_path, &parent);
	if (ret != EXIT_SUCCESS || !parent || !parent->dir) {
		fprintf(stderr, "Error: Parent directory not found or is not a directory.\n");
		return -ENOENT;  // No such file or directory
	}

	dentry *directory_to_remove = NULL;
	int found = 0;
	for (int i = 0; i < parent->dir->size; i++) {
		if (strcmp(parent->dir->entries[i]->nombre, child_name) == 0) {
			directory_to_remove = parent->dir->entries[i];
			found = i;
			break;
		}
	}


	if (directory_to_remove == NULL ||
	    directory_to_remove->inodo->dir == NULL) {
		fprintf(stderr, "Error: Directory not found: %s\n", child_name);
		return -ENOENT;  // No such file or directory
	}


	if (directory_to_remove->inodo->dir->size > 0) {
		fprintf(stderr, "Error: Directory not empty: %s\n", child_name);
		return -ENOTEMPTY;  // Directory not empty
	}


	for (int j = found; j < parent->dir->size - 1; j++) {
		parent->dir->entries[j] = parent->dir->entries[j + 1];
	}
	parent->dir->entries[parent->dir->size - 1] = NULL;
	parent->dir->size--;


	free(directory_to_remove->inodo->dir);
	free(directory_to_remove->inodo);
	free(directory_to_remove);
	return EXIT_SUCCESS;
}


int
filesystem_utimens(const char *path, const struct timespec tv[2])
{
	inodo *inode = NULL;
	int ret = search_inodo(path, &inode);
	if (ret != EXIT_SUCCESS || !inode) {
		fprintf(stderr, "Error: Inode not found for path: %s\n", path);
		return -ENOENT;  // No such file or directory
	}

	inode->atime = tv[0].tv_sec;  // Update access time
	inode->mtime = tv[1].tv_sec;  // Update modification time
	return EXIT_SUCCESS;
}


int
filesystem_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	char new_file[MAX_FILENAME];
	char dir_parent[PATH_MAX];

	if (split_parent_child(path, dir_parent, new_file) != EXIT_SUCCESS) {
		return -ENOENT;  // No such file or directory
	}


	inodo *dir_copy_inodo = NULL;
	int ret = search_inodo(dir_parent, &dir_copy_inodo);

	if (ret != EXIT_SUCCESS || !dir_copy_inodo || !dir_copy_inodo->dir) {
		fprintf(stderr, "Error: Parent directory not found or is not a directory.\n");
		return -ENOENT;  // No such file or directory
	} else if (dir_copy_inodo->dir->size >= MAX_DENTRIES) {
		fprintf(stderr, "Error: Parent directory is full, cannot create new file.\n");
		return -ENOSPC;  // No space left on device
	} else {
		for (int i = 0; i < dir_copy_inodo->dir->size; i++) {
			if (strcmp(dir_copy_inodo->dir->entries[i]->nombre,
			           new_file) == 0) {
				fprintf(stderr,
				        "Error: File already exists: %s\n",
				        new_file);
				return -EEXIST;  // File exists
			}
		}
	}

	inodo_dir *directory = dir_copy_inodo->dir;

	dentry *new_entry = malloc(sizeof(dentry));
	strncpy(new_entry->nombre, new_file, MAX_FILENAME - 1);
	new_entry->nombre[MAX_FILENAME - 1] = '\0';


	new_entry->inodo = malloc(sizeof(inodo));
	new_entry->inodo->file = malloc(sizeof(inodo_file));
	new_entry->inodo->dir = NULL;

	new_entry->inodo->mode =
	        __S_IFREG |
	        mode;  // Set mode to regular file with specified permissions
	new_entry->inodo->nlink = 1;  // New file has 1 link (itself)
	new_entry->inodo->uid = getuid();
	new_entry->inodo->gid = getgid();
	new_entry->inodo->atime = time(NULL);
	new_entry->inodo->mtime = time(NULL);
	new_entry->inodo->ctime = time(NULL);
	new_entry->inodo->size = strlen(new_entry->inodo->file->content);


	new_entry->inodo->file->content[0] = '\0';


	directory->entries[directory->size] = new_entry;
	directory->size++;
	return EXIT_SUCCESS;
}

int
filesystem_read(const char *path,
                char *buf,
                size_t size,
                off_t offset,
                struct fuse_file_info *fi)
{
	inodo *inode = NULL;
	int ret = search_inodo(path, &inode);
	if (ret != EXIT_SUCCESS || !inode || !inode->file) {
		fprintf(stderr,
		        "Error: File not found or is not a regular file.\n");
		return -ENOENT;  // No such file or directory
	}

	if (offset < 0 || offset > inode->size) {
		fprintf(stderr, "Error: Offset out of bounds.\n");
		return -EINVAL;  // Invalid argument
	}

	size_t bytes_to_read = inode->size - offset;
	if (bytes_to_read >= size) {
		bytes_to_read = size;
	}

	memcpy(buf, inode->file->content + offset, bytes_to_read);
	buf[bytes_to_read] = '\0';
	inode->atime = time(NULL);
	return bytes_to_read;
}

int
filesystem_write(const char *path,
                 const char *buf,
                 size_t size,
                 off_t offset,
                 struct fuse_file_info *fi)
{
	inodo *inode = NULL;
	int ret = search_inodo(path, &inode);
	if (ret != EXIT_SUCCESS || !inode || !inode->file) {
		fprintf(stderr,
		        "Error: File not found or is not a regular file.\n");
		return -ENOENT;  // No such file or directory
	}
	if (offset < 0 || offset > inode->size) {
		fprintf(stderr, "Error: Offset out of bounds.\n");
		return -EINVAL;  // Invalid argument
	}

	if (offset + size > CONTENT_SIZE) {
		fprintf(stderr, "Error: Write exceeds file size limit.\n");
		return -ENOSPC;  // No space left on device
	}

	memcpy(inode->file->content + offset, buf, size);

	inode->size += size;

	return size;
}

int
filesystem_unlink(const char *path)
{
	char file[MAX_FILENAME];
	char parent_dir[PATH_MAX];

	if (split_parent_child(path, parent_dir, file) != EXIT_SUCCESS) {
		return -ENOENT;  // No such file or directory
	}

	inodo *parent = NULL;
	int ret = search_inodo(parent_dir, &parent);
	if (ret != EXIT_SUCCESS || !parent || !parent->dir) {
		fprintf(stderr, "Error: Parent directory not found or is not a directory.\n");
		return -ENOENT;  // No such file or directory
	}


	dentry *file_to_remove = NULL;
	int found = 0;
	for (int i = 0; i < parent->dir->size; i++) {
		if (strcmp(parent->dir->entries[i]->nombre, file) == 0) {
			file_to_remove = parent->dir->entries[i];
			found = i;
			break;
		}
	}
	if (file_to_remove == NULL || file_to_remove->inodo->file == NULL ||
	    file_to_remove->inodo->dir != NULL) {
		fprintf(stderr,
		        "Error: File not found or is a directory: %s\n",
		        file);
		return -ENOENT;  // No such file or directory
	}


	for (int j = found; j < parent->dir->size - 1; j++) {
		parent->dir->entries[j] = parent->dir->entries[j + 1];
	}
	parent->dir->size--;

	free(file_to_remove->inodo->file);
	free(file_to_remove->inodo);
	free(file_to_remove);

	return EXIT_SUCCESS;
}
