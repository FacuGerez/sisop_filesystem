#define CONTENT_SIZE 1024
#define MAX_DENTRIES 100
#define MAX_FILENAME 256
#define DEFAULT_FILE_DISK "persistence_file.fisopfs"

typedef struct dentry {
	char nombre[MAX_FILENAME];
	struct inodo_t *inodo;
} dentry_t;

typedef struct inodo_file {
	char contenido[CONTENT_SIZE];
} inodo_file_t;

typedef struct inodo_dir {
	dentry_t *entries[MAX_DENTRIES];  // Entradas del directorio
} inodo_dir_t;

typedef struct inodo {
	inodo_file_t *file;  // Contenido del archivo o null si es un directorio
	inodo_dir_t *dir;    // Contenido del directorio o null si es un archivo
	int size;            // Tamaño del contenido
	int nlinks;          // Número de enlaces duros
	int atime;           // Último acceso
	int mtime;           // Última modificación
	int ctime;           // Último cambio de metadatos
	                     // .....
} inodo_t;
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
	inodo_t *root;  // Inodo raíz del sistema de archivos
} filesystem_t;


// Variables globales
char *filedisk = DEFAULT_FILE_DISK;
filesystem_t fs;
