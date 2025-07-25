#include "operations.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <errno.h>
#include <unistd.h>

#include "errors.h"
#include "defs.h"
#include "persistence.h"

extern filesystem fs;
extern char *filedisk;

// Functions and wrappers for the filesystem operations

// Convert inode to stat structure
void
inode_to_stat(const inode *inode, struct stat *stbuf)
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

// Search for an inode by path
int
search_inode(const char *path, inode **result)
{
	char path_copy[PATH_MAX];
	strncpy(path_copy, path, sizeof(path_copy));
	path_copy[PATH_MAX - 1] = '\0';

	char *read_path = path_copy;
	read_path++;  // Skip the leading '/'
	char *ptr;


	inode *current = fs.root;

	if (strlen(read_path) == 0) {
		// If the path is just "/", return the root inode
		*result = current;
		return EXIT_SUCCESS;
	}

	while ((ptr = strchr(read_path, '/')) != NULL) {
		*ptr = '\0';

		bool found = false;
		if (current->dir == NULL) {
			return -ENOTDIR;
		}

		for (int i = 0; i < current->dir->size; i++) {
			if (strcmp(read_path,
			           current->dir->entries[i]->filename) == 0) {
				found = true;
				current = current->dir->entries[i]->inode;
				break;
			}
		}
		if (!found) {
			return -ENOENT;
		}
		read_path = ptr + 1;
	}
	// After processing all parts of the path, check if we are at a file or a directory
	if (strlen(read_path) != 0) {
		bool found = false;
		if (current->dir == NULL) {
			return -ENOTDIR;
		}

		for (int i = 0; i < current->dir->size; i++) {
			if (strcmp(read_path,
			           current->dir->entries[i]->filename) == 0) {
				found = true;
				current = current->dir->entries[i]->inode;
				break;
			}
		}
		if (!found) {
			return -ENOENT;
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
		fprintf(stderr, "Error: invalid path %s\n", path);
		return -ENOENT;
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
	printf("Initializing filesystem...\n");

	// Initialize the filesystem structure from file.
	FILE *input = fopen(filedisk, "rb");
	if (input) {
		printf("Loading filesystem from disk: %s\n", filedisk);
		fs.root = deserialize_inode(input);
		fclose(input);
		printf("Filesystem loaded from disk: %s\n", filedisk);
		return &fs;
	}

	printf("No persistence file found, initializing new FS.\n");

	// Fallback, initialize the filesystem structure from scratch.
	fs.root = malloc(sizeof(inode));
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
	fs.root->dir = malloc(sizeof(inode_dir));
	fs.root->dir->entries[0] = NULL;
	fs.root->dir->size = 0;

	printf("Filesystem initialized successfully.\n");
	return &fs;
}

int
filesystem_getattr(const char *path, struct stat *stbuf)
{
	inode *inode = NULL;
	int ret = search_inode(path, &inode);
	if (ret != EXIT_SUCCESS) {
		fprintf(stderr, INODE_NOT_FOUND, path);
		return -ENOENT;
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
		return -ENOENT;
	}

	inode *dir = NULL;
	int ret = search_inode(path_copy, &dir);

	if (ret != EXIT_SUCCESS) {
		fprintf(stderr, PARENT_DIRECTORY_NOT_FOUND);
		return -ENOENT;
	} else if (!dir->dir) {
		fprintf(stderr, PARENT_INODE_NOT_DIRECTORY);
		return -ENOENT;
	} else if (dir->dir->size >= MAX_DENTRIES) {
		fprintf(stderr, PARENT_DIRECTORY_FULL);
		return -ENOSPC;
	} else {
		for (int i = 0; i < dir->dir->size; i++) {
			if (strcmp(dir->dir->entries[i]->filename,
			           new_directory) == 0) {
				fprintf(stderr,
				        DIRECTORY_ALREADY_EXISTS,
				        new_directory);
				return -EEXIST;
			}
		}
	}
	dir->mtime = time(NULL);
	inode_dir *directory = dir->dir;

	dentry *new_entry = malloc(sizeof(dentry));
	strncpy(new_entry->filename, new_directory, MAX_FILENAME - 1);
	new_entry->filename[MAX_FILENAME - 1] = '\0';

	new_entry->inode = malloc(sizeof(inode));
	new_entry->inode->mode =
	        __S_IFDIR |
	        0755;  // Set mode to directory with rwxr-xr-x permissions
	new_entry->inode->nlink =
	        2;  // New directory has 2 links (itself and its parent)
	new_entry->inode->uid = getuid();
	new_entry->inode->gid = getgid();
	new_entry->inode->atime = time(NULL);
	new_entry->inode->mtime = time(NULL);
	new_entry->inode->ctime = time(NULL);
	new_entry->inode->size = 0;
	new_entry->inode->file = NULL;
	new_entry->inode->dir = malloc(sizeof(inode_dir));
	new_entry->inode->dir->size = 0;

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
	inode *directory = NULL;
	int ret = search_inode(path, &directory);
	if (ret != EXIT_SUCCESS) {
		fprintf(stderr, PARENT_DIRECTORY_NOT_FOUND);
		return -ENOENT;
	} else if (!directory->dir) {
		fprintf(stderr, PARENT_INODE_NOT_DIRECTORY);
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	for (int i = 0; i < directory->dir->size; i++) {
		if (directory->dir->entries[i] == NULL) {
			continue;
		}
		struct stat stbuf;
		inode_to_stat(directory->dir->entries[i]->inode, &stbuf);

		filler(buf, directory->dir->entries[i]->filename, &stbuf, 0);
	}

	return EXIT_SUCCESS;
}

int
filesystem_rmdir(const char *path)
{
	inode *parent = NULL;
	char parent_path[PATH_MAX];
	char child_name[MAX_FILENAME];

	if (split_parent_child(path, parent_path, child_name) != EXIT_SUCCESS) {
		return -ENOENT;
	}


	int ret = search_inode(parent_path, &parent);
	if (ret != EXIT_SUCCESS) {
		fprintf(stderr, PARENT_DIRECTORY_NOT_FOUND);
		return -ENOENT;
	} else if (!parent->dir) {
		fprintf(stderr, PARENT_INODE_NOT_DIRECTORY);
		return -ENOENT;
	}

	dentry *directory_to_remove = NULL;
	int found = 0;
	for (int i = 0; i < parent->dir->size; i++) {
		if (strcmp(parent->dir->entries[i]->filename, child_name) == 0) {
			directory_to_remove = parent->dir->entries[i];
			found = i;
			break;
		}
	}

	if (directory_to_remove == NULL ||
	    directory_to_remove->inode->dir == NULL) {
		fprintf(stderr, DIRECTORY_NOT_FOUND, child_name);
		return -ENOENT;
	}

	if (directory_to_remove->inode->dir->size > 0) {
		fprintf(stderr, DIRECTORY_NOT_EMPTY, child_name);
		return -ENOTEMPTY;
	}

	parent->mtime = time(NULL);
	for (int j = found; j < parent->dir->size - 1; j++) {
		parent->dir->entries[j] = parent->dir->entries[j + 1];
	}
	parent->dir->entries[parent->dir->size - 1] = NULL;
	parent->dir->size--;


	free(directory_to_remove->inode->dir);
	free(directory_to_remove->inode);
	free(directory_to_remove);
	return EXIT_SUCCESS;
}

int
filesystem_utimens(const char *path, const struct timespec tv[2])
{
	inode *inode = NULL;
	int ret = search_inode(path, &inode);
	if (ret != EXIT_SUCCESS) {
		fprintf(stderr, INODE_NOT_FOUND, path);
		return -ENOENT;
	}

	inode->atime = tv[0].tv_sec;
	inode->mtime = tv[1].tv_sec;
	return EXIT_SUCCESS;
}


int
filesystem_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	char new_file[MAX_FILENAME];
	char dir_parent[PATH_MAX];

	if (split_parent_child(path, dir_parent, new_file) != EXIT_SUCCESS) {
		return -ENOENT;
	}

	inode *dir_copy_inode = NULL;
	int ret = search_inode(dir_parent, &dir_copy_inode);

	if (ret != EXIT_SUCCESS) {
		fprintf(stderr, PARENT_DIRECTORY_NOT_FOUND);
		return -ENOENT;
	} else if (!dir_copy_inode->dir) {
		fprintf(stderr, PARENT_INODE_NOT_DIRECTORY);
		return -ENOENT;
	} else if (dir_copy_inode->dir->size >= MAX_DENTRIES) {
		fprintf(stderr, PARENT_DIRECTORY_FULL);
		return -ENOSPC;
	} else {
		for (int i = 0; i < dir_copy_inode->dir->size; i++) {
			if (strcmp(dir_copy_inode->dir->entries[i]->filename,
			           new_file) == 0) {
				fprintf(stderr, FILE_ALREADY_EXISTS, new_file);
				return -EEXIST;
			}
		}
	}

	dir_copy_inode->mtime = time(NULL);
	inode_dir *directory = dir_copy_inode->dir;

	dentry *new_entry = malloc(sizeof(dentry));
	strncpy(new_entry->filename, new_file, MAX_FILENAME - 1);
	new_entry->filename[MAX_FILENAME - 1] = '\0';


	new_entry->inode = malloc(sizeof(inode));
	new_entry->inode->file = malloc(sizeof(inode_file));
	memset(new_entry->inode->file->content, 0, MAX_CONTENT_SIZE);
	new_entry->inode->dir = NULL;

	new_entry->inode->mode =
	        __S_IFREG |
	        mode;  // Set mode to regular file with specified permissions
	new_entry->inode->nlink = 1;  // New file has 1 link (itself)
	new_entry->inode->uid = getuid();
	new_entry->inode->gid = getgid();
	new_entry->inode->atime = time(NULL);
	new_entry->inode->mtime = time(NULL);
	new_entry->inode->ctime = time(NULL);
	new_entry->inode->size = 0;

	new_entry->inode->file->content[0] = '\0';


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
	inode *inode = NULL;
	int ret = search_inode(path, &inode);
	if (ret != EXIT_SUCCESS) {
		fprintf(stderr, INODE_NOT_FOUND, path);
		return -ENOENT;
	} else if (!inode->file) {
		fprintf(stderr, INODE_NOT_FILE);
		return -ENOENT;
	}

	if (offset < 0 || offset > inode->size) {
		fprintf(stderr, OFFSET_OUT_OF_BOUNDS);
		return -EINVAL;
	}

	size_t bytes_to_read = inode->size - offset;
	if (bytes_to_read > size) {
		bytes_to_read = size;
	}

	memcpy(buf, inode->file->content + offset, bytes_to_read);
	inode->atime = time(NULL);
	return (int) bytes_to_read;
}


int
filesystem_open(const char *path, struct fuse_file_info *fi)
{
	inode *inode = NULL;
	int ret = search_inode(path, &inode);
	if (ret != EXIT_SUCCESS) {
		fprintf(stderr, INODE_NOT_FOUND, path);
		return -ENOENT;
	} else if (!inode->file) {
		fprintf(stderr, INODE_NOT_FILE);
		return -ENOENT;
	}
	return EXIT_SUCCESS;
}


int
filesystem_write(const char *path,
                 const char *buf,
                 size_t size,
                 off_t offset,
                 struct fuse_file_info *fi)
{
	inode *inode = NULL;
	int ret = search_inode(path, &inode);
	if (ret != EXIT_SUCCESS) {
		fprintf(stderr, INODE_NOT_FOUND, path);
		return -ENOENT;
	} else if (!inode->file) {
		fprintf(stderr, INODE_NOT_FILE);
		return -ENOENT;
	}
	if (offset < 0 || offset > MAX_CONTENT_SIZE) {
		fprintf(stderr, OFFSET_OUT_OF_BOUNDS);
		return -EINVAL;
	}

	if (offset + size > MAX_CONTENT_SIZE) {
		fprintf(stderr, WRITE_EXCEEDS_SIZE);
		return -ENOSPC;
	}

	if (offset > inode->size) {
		memset(inode->file->content + inode->size, 0, offset - inode->size);
	}

	memcpy(inode->file->content + offset, buf, size);

	off_t end_offset = offset + size;
	if (end_offset > inode->size) {
		inode->size = end_offset;
	}

	inode->mtime = time(NULL);
	inode->ctime = time(NULL);

	return (int) size;
}

int
filesystem_truncate(const char *path, off_t size)
{
	inode *inode = NULL;
	int ret = search_inode(path, &inode);

	if (ret != EXIT_SUCCESS) {
		fprintf(stderr, INODE_NOT_FOUND, path);
		return -ENOENT;
	} else if (!inode->file) {
		fprintf(stderr, INODE_NOT_FILE);
		return -ENOENT;
	}

	if (size < 0 || size > MAX_CONTENT_SIZE) {
		return -EINVAL;
	}

	if (size < inode->size) {
		for (off_t i = size; i < MAX_CONTENT_SIZE; i++) {
			inode->file->content[i] = '\0';
		}
	}

	if (size > inode->size) {
		memset(inode->file->content + inode->size, 0, size - inode->size);
	}

	inode->size = size;
	inode->mtime = time(NULL);
	return 0;
}

int
filesystem_unlink(const char *path)
{
	char file[MAX_FILENAME];
	char parent_dir[PATH_MAX];

	if (split_parent_child(path, parent_dir, file) != EXIT_SUCCESS) {
		return -ENOENT;
	}

	inode *parent = NULL;
	int ret = search_inode(parent_dir, &parent);
	if (ret != EXIT_SUCCESS) {
		fprintf(stderr, PARENT_DIRECTORY_NOT_FOUND);
		return -ENOENT;
	} else if (!parent->dir) {
		fprintf(stderr, PARENT_INODE_NOT_DIRECTORY);
		return -ENOENT;
	}

	dentry *file_to_remove = NULL;
	int found = 0;
	for (int i = 0; i < parent->dir->size; i++) {
		if (strcmp(parent->dir->entries[i]->filename, file) == 0) {
			file_to_remove = parent->dir->entries[i];
			found = i;
			break;
		}
	}
	if (file_to_remove == NULL || file_to_remove->inode->file == NULL ||
	    file_to_remove->inode->dir != NULL) {
		fprintf(stderr, INODE_NOT_FOUND, path);
		return -ENOENT;
	}

	parent->mtime = time(NULL);
	for (int j = found; j < parent->dir->size - 1; j++) {
		parent->dir->entries[j] = parent->dir->entries[j + 1];
	}
	parent->dir->size--;

	// TODO: ADD COMMENT
	free(file_to_remove->inode->file);
	free(file_to_remove->inode);
	free(file_to_remove);

	return EXIT_SUCCESS;
}

void
filesystem_destroy(void *private_data)
{
	printf("Saving filesystem to disk: %s\n", filedisk);
	FILE *output = fopen(filedisk, "wb");
	serialize_inode(output, fs.root);
	fclose(output);
	printf("Filesystem saved to disk: %s\n", filedisk);
}
