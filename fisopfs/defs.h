#ifndef FS_DEFS_H
#define FS_DEFS_H

#include <sys/types.h>

#define MAX_CONTENT_SIZE 1024
#define MAX_DENTRIES 128
#define MAX_FILENAME 256
#define DEFAULT_FILE_DISK "persistence_file.fisopfs"

typedef struct inode inode;

typedef struct dentry {
	char filename[MAX_FILENAME];
	inode *inode;
} dentry;

typedef struct inode_file {
	char content[MAX_CONTENT_SIZE];
} inode_file;

typedef struct inode_dir {
	dentry *entries[MAX_DENTRIES];
	int size;
} inode_dir;

typedef struct inode {
	inode_file *file;  // Inode file or null if it is a directory
	inode_dir *dir;    // Inode directory or null if it is a file
	mode_t mode;       // Type and permissions of this inodes
	nlink_t nlink;     // Number of links (2 for directory, 1 for file)
	uid_t uid;         // UID of the owner
	gid_t gid;         // GID of the group owner
	time_t atime;      // Last access time
	time_t mtime;      // Last modification time
	time_t ctime;      // Creation time
	off_t size;        // Size in bytes/chars for files or 0 for directories
} inode;

// Filesystem structure
typedef struct filesystem {
	inode *root;  // Inode root for the filesystem
} filesystem;

#endif
