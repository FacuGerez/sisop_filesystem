#define CONTENT_SIZE 1024
#define MAX_DENTRIES 128
#define MAX_FILENAME 256
#define DEFAULT_FILE_DISK "persistence_file.fisopfs"

typedef struct inodo inodo;

typedef struct dentry {
	char nombre[MAX_FILENAME];
	inodo *inodo;
} dentry;

typedef struct inodo_file {
	char content[CONTENT_SIZE];
} inodo_file;

typedef struct inodo_dir {
	dentry *entries[MAX_DENTRIES];  // Entry for the directory
	int size;                       // Number of entries in the directory
} inodo_dir;

typedef struct inodo {
	inodo_file *file;  // inode file or null if it is a directory
	inodo_dir *dir;    // inode directory or null if it is a file
	// 👇 metadata needed
	mode_t mode;    // Type and permissions
	nlink_t nlink;  // Number of links (2 for directory, 1 for file)
	uid_t uid;      // UID of the owner
	gid_t gid;      // GID of the group
	time_t atime;   // Last access
	time_t mtime;   // Last modification
	time_t ctime;   // Creation
	off_t size;     // Size in bytes/chars (for files) or 0 for directories
} inodo;

// filesystem structure
typedef struct filesystem {
	inodo *root;  // Inode root for the filesystem
} filesystem;


// Global variables
char *filedisk = DEFAULT_FILE_DISK;
filesystem fs;
