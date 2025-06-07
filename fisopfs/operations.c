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

void
inodo_to_stat(const inodo *inode, struct stat *stbuf)
{
	memset(stbuf, 0, sizeof(struct stat));  // Clear the stat structure
	stbuf->st_mode = inode->mode;           // Set the mode
	stbuf->st_uid = inode->uid;             // User ID of owner
	stbuf->st_gid = inode->gid;             // Group ID of owner
	stbuf->st_atime = inode->atime;         // Last access time
	stbuf->st_mtime = inode->mtime;         // Last modification time
	stbuf->st_ctime = inode->ctime;         // Creation time
	stbuf->st_nlink = inode->nlink;         // Number of links
	stbuf->st_size = inode->size;           // Size in bytes
}

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

	*result = current;

	return EXIT_SUCCESS;
}

void *
filesystem_init(struct fuse_conn_info *conn)
{
	// Initialize the filesystem structure
	fs.root = malloc(sizeof(inodo));
	fs.root->mode = __S_IFDIR |
	                0755;  // Set mode to directory with rwxr-xr-x permissions
	fs.root->nlink = 2;  // Root directory has 2 links (itself and its parent)
	fs.root->uid = getuid();  // Set the owner to the current user
	fs.root->gid = getgid();  // Set the group to the current user's group
	fs.root->atime = time(NULL);  // Set last access time to now
	fs.root->mtime = time(NULL);  // Set last modification time to now
	fs.root->ctime = time(NULL);  // Set creation time to now
	fs.root->size = 0;            // Root directory starts empty
	// Initialize the root inodo
	fs.root->file = NULL;  // Root is a directory
	fs.root->dir = malloc(sizeof(inodo_dir));
	fs.root->dir->entries[0] = NULL;  // Initialize the root directory entries
	fs.root->dir->size = 0;           // No entries in the root directory
	printf("Filesystem initialized successfully.\n");
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

	if (last_slash == NULL || strcmp(last_slash, "/") == 0) {
		fprintf(stderr, "Error: Invalid path: %s\n", path);
		return -ENOENT;  // No such file or directory
	}
	// Calcular longitudes
	if (last_slash == path) {
		strcpy(path_copy, "/");
		path_copy[1] = '\0';
	} else {
		size_t dir_len = last_slash - path;
		strncpy(path_copy, path, dir_len);
		path_copy[dir_len] = '\0';
	}

	strcpy(new_directory, last_slash + 1);

	/*
	 * Now we have the parent directory in `path_copy` and the new directory
	 * name in `new_directory`. We will search for the parent directory in
	 * the filesystem and create a new entry for the new directory. If the
	 * parent directory does not exist, we return an error. If the new
	 * directory already exists, we also return an error.
	 */


	inodo *dir = NULL;
	int ret = search_inodo(path_copy, &dir);

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

	// Allocate memory for the inodo and initialize it
	new_entry->inodo =
	        malloc(sizeof(inodo));  // Allocate memory for the new inodo
	new_entry->inodo->mode =
	        __S_IFDIR |
	        0755;  // Set mode to directory with rwxr-xr-x permissions
	new_entry->inodo->nlink =
	        2;  // New directory has 2 links (itself and its parent)
	new_entry->inodo->uid = getuid();  // Set the owner to the current user
	new_entry->inodo->gid =
	        getgid();  // Set the group to the current user's group
	new_entry->inodo->atime = time(NULL);  // Set last access time to now
	new_entry->inodo->mtime =
	        time(NULL);  // Set last modification time to now
	new_entry->inodo->ctime = time(NULL);  // Set creation time to now
	new_entry->inodo->size = 0;            // New directory starts empty

	// Initialize the inodo for the new directory
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
	inodo *directory = NULL;
	int ret = search_inodo(path, &directory);
	if (ret != EXIT_SUCCESS || !directory || !directory->dir) {
		fprintf(stderr, "Error: Parent directory not found or is not a directory.\n");
		return -ENOENT;
	}

	// TODO: where is NULL back it, we should put the stat
	filler(buf, ".", NULL, 0);   // Add current directory
	filler(buf, "..", NULL, 0);  // Add parent directory

	for (int i = 0; i < directory->dir->size; i++) {
		if (directory->dir->entries[i] == NULL) {
			continue;  // Skip empty entries
		}
		struct stat stbuf;
		inodo_to_stat(directory->dir->entries[i]->inodo, &stbuf);

		filler(buf,
		       directory->dir->entries[i]->nombre,
		       &stbuf,
		       0);  // Add each entry in the directory
	}

	return EXIT_SUCCESS;  // Success
}

int
filesystem_rmdir(const char *path)
{
	// Remove the directory from its parent
	inodo *parent = NULL;
	char parent_path[PATH_MAX];
	strncpy(parent_path, path, sizeof(parent_path));
	char *last_slash = strrchr(parent_path, '/');
	if (last_slash) {
		*last_slash =
		        '\0';  // Remove the last part to get the parent path
	} else {
		strcpy(parent_path, "/");  // If no slash, it's the root
	}
	last_slash++;

	int ret = search_inodo(parent_path, &parent);
	if (ret != EXIT_SUCCESS || !parent || !parent->dir) {
		fprintf(stderr, "Error: Parent directory not found.\n");
		return -ENOENT;  // No such file or directory
	}
	// Now search for the directory to remove
	dentry *directory_to_remove = NULL;
	for (int i = 0; i < parent->dir->size; i++) {
		if (strcmp(parent->dir->entries[i]->nombre, last_slash) == 0) {
			directory_to_remove = parent->dir->entries[i];
			break;
		}
	}
	if (directory_to_remove == NULL ||
	    directory_to_remove->inodo->dir == NULL) {
		fprintf(stderr, "Error: Directory not found: %s\n", last_slash);
		return -ENOENT;  // No such file or directory
	}
	if (directory_to_remove->inodo->dir->size > 0) {
		fprintf(stderr, "Error: Directory not empty: %s\n", last_slash);
		return -ENOTEMPTY;  // Directory not empty
	}
	// Remove the directory entry from the parent
	for (int i = 0; i < parent->dir->size; i++) {
		if (parent->dir->entries[i] == directory_to_remove) {
			// Shift the entries to remove the directory
			for (int j = i; j < parent->dir->size - 1; j++) {
				parent->dir->entries[j] =
				        parent->dir->entries[j + 1];
			}
			parent->dir->entries[parent->dir->size - 1] =
			        NULL;  // Clear the last entry
			break;
		}
	}
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
	return EXIT_SUCCESS;          // Success
}


int
filesystem_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	/*
	 * First, we separate the path into the new file name and the parent
	 * directory. We assume the path is in the format "/parent/new_file". If
	 * the path is just "/", we return an error because we cannot create a
	 * file at the root. If the path is invalid (e.g., "/new_file/"), we
	 * also return an error. We will use the last slash to determine the
	 * parent directory and the new file name.
	 */
	char new_file[MAX_FILENAME];
	char dir_copy[PATH_MAX];

	char *last_slash = strrchr(path, '/');

	if (last_slash == NULL || strcmp(last_slash, "/") == 0) {
		fprintf(stderr, "Error: Invalid path: %s\n", path);
		return -ENOENT;  // No such file or directory
	}
	// Calcular longitudes
	if (last_slash == path) {
		strcpy(dir_copy, "/");
		dir_copy[1] = '\0';
	} else {
		size_t dir_len = last_slash - path;
		strncpy(dir_copy, path, dir_len);
		dir_copy[dir_len] = '\0';
	}

	strcpy(new_file, last_slash + 1);

	// Create the new file

	inodo *dir_copy_inodo = NULL;
	int ret = search_inodo(dir_copy, &dir_copy_inodo);

	if (ret != EXIT_SUCCESS || !dir_copy_inodo || !dir_copy_inodo->dir) {
		fprintf(stderr, "Error: Parent directory not found or is not a directory.\n");
		return -ENOENT;  // No such file or directory
	} else if (dir_copy_inodo->dir->size >= MAX_DENTRIES) {
		fprintf(stderr, "Error: Parent directory is full, cannot create new file.\n");
		return -ENOSPC;  // No space left on device
	} else {
		// Check if the new file already exists in the parent directory
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

	// create the file entry
	dentry *new_entry = malloc(sizeof(dentry));
	strncpy(new_entry->nombre, new_file, MAX_FILENAME - 1);
	new_entry->nombre[MAX_FILENAME - 1] = '\0';  // Ensure null termination

	// Allocate memory for the inodo and initialize it
	new_entry->inodo =
	        malloc(sizeof(inodo));  // Allocate memory for the new inodo
	new_entry->inodo->file = malloc(
	        sizeof(inodo_file));   // Allocate memory for the file content
	new_entry->inodo->dir = NULL;  // New file has no directory content

	// Set the metadata for the new file
	new_entry->inodo->mode =
	        __S_IFREG |
	        mode;  // Set mode to regular file with specified permissions
	new_entry->inodo->nlink = 1;       // New file has 1 link (itself)
	new_entry->inodo->uid = getuid();  // Set the owner to the current user
	new_entry->inodo->gid =
	        getgid();  // Set the group to the current user's group
	new_entry->inodo->atime = time(NULL);  // Set last access time to now
	new_entry->inodo->mtime =
	        time(NULL);  // Set last modification time to now
	new_entry->inodo->ctime = time(NULL);  // Set creation time to now
	new_entry->inodo->size = strlen(new_entry->inodo->file->contenido);  // New file starts with size of the inodo

	// Initialize the inodo for the new file
	new_entry->inodo->file->contenido[0] =
	        '\0';  // Initialize file content to empty

	// Set the inodo fields for the new file
	directory->entries[directory->size] = new_entry;  // Add the new entry
	directory->size++;    // Increment the size of the directory
	return EXIT_SUCCESS;  // Success
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
		bytes_to_read = size;  // Limit to the requested size
	}

	memcpy(buf, inode->file->contenido + offset, bytes_to_read);
	buf[bytes_to_read] = '\0';  // Null-terminate the buffer
	inode->atime = time(NULL);  // Update last access time
	return bytes_to_read;       // Return the number of bytes read
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

	memcpy(inode->file->contenido + offset, buf, size);

	inode->size += size;

	return size;  // Placeholder for write operation
}

int
filesystem_unlink(const char *path)
{
	char file[MAX_FILENAME];
	char parent_dir[PATH_MAX];

	char *last_slash = strrchr(path, '/');

	if (last_slash == NULL || strcmp(last_slash, "/") == 0) {
		fprintf(stderr, "Error: Invalid path: %s\n", path);
		return -ENOENT;  // No such file or directory
	}
	// Calcular longitudes
	if (last_slash == path) {
		strcpy(parent_dir, "/");
		parent_dir[1] = '\0';
	} else {
		size_t dir_len = last_slash - path;
		strncpy(parent_dir, path, dir_len);
		parent_dir[dir_len] = '\0';
	}

	strcpy(file, last_slash + 1);

	inodo *parent = NULL;
	int ret = search_inodo(parent_dir, &parent);
	if (ret != EXIT_SUCCESS || !parent || !parent->dir) {
		fprintf(stderr, "Error: Parent directory not found or is not a directory.\n");
		return -ENOENT;  // No such file or directory
	}
	dentry *file_to_remove = NULL;
	for (int i = 0; i < parent->dir->size; i++) {
		if (strcmp(parent->dir->entries[i]->nombre, file) == 0) {
			file_to_remove = parent->dir->entries[i];
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

	free(file_to_remove->inodo->file);
	free(file_to_remove->inodo);
	free(file_to_remove);

	// Remove the entry from the parent's directory
	for (int i = 0; i < parent->dir->size; i++) {
		if (parent->dir->entries[i] == file_to_remove) {
			// Shift remaining entries to fill the gap
			for (int j = i; j < parent->dir->size - 1; j++) {
				parent->dir->entries[j] =
				        parent->dir->entries[j + 1];
			}
			parent->dir->size--;
			break;
		}
	}

	return EXIT_SUCCESS;
}
