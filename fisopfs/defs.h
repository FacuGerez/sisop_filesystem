#ifndef FS_DEFS_H
#define FS_DEFS_H

#include <sys/types.h>

#define CONTENT_SIZE 1024
#define MAX_DENTRIES 128
#define MAX_FILENAME 256
#define DEFAULT_FILE_DISK "persistence_file.fisopfs"

typedef struct inode inode;

typedef struct dentry {
	char filename[MAX_FILENAME];
	inode *inode;
} dentry;

typedef struct inode_file {
	char content[CONTENT_SIZE];
} inode_file;

typedef struct inode_dir {
	dentry *dentries[MAX_DENTRIES];  // Entradas del directorio
	int size;                        // Número de entradas en el directorio
} inode_dir;

typedef struct inode {
	inode_file *file;  // Contenido del archivo o null si es un directorio
	inode_dir *dir;    // Contenido del directorio o null si es un archivo
	                   // .....
	// 👇 Campos necesarios para getattr:
	mode_t mode;    // Tipo y permisos
	nlink_t nlink;  // Número de enlaces (2 para directorio, 1 para archivo)
	uid_t uid;      // UID del dueño (usualmente getuid())
	gid_t gid;      // GID del grupo (usualmente getgid())
	time_t atime;   // Último acceso
	time_t mtime;   // Última modificación
	time_t ctime;   // Creación
	off_t size;     // Tamaño en bytes (para archivos) o 0 para directorios
} inode;

/*
typedef struct superblock {
    int inodes_count; // Número total de inodos
    int blocks_count; // Número total de bloques
    int free_inodes; // Inodos libres
    int free_blocks; // Bloques libres
} superblock_t;
*/

// El sistema de archivos desde la raiz
typedef struct filesystem {
	inode *root;  // Inodo raíz del sistema de archivos
} filesystem;

#endif
