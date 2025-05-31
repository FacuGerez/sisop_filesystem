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
	char contenido[CONTENT_SIZE];
} inodo_file;

typedef struct inodo_dir {
	dentry *entries[MAX_DENTRIES];  // Entradas del directorio
	int size;                       // Número de entradas en el directorio
} inodo_dir;

typedef struct inodo {
	inodo_file *file;  // Contenido del archivo o null si es un directorio
	inodo_dir *dir;    // Contenido del directorio o null si es un archivo
	int size;          // Tamaño del contenido
	int nlinks;        // Número de enlaces duros
	int atime;         // Último acceso
	int mtime;         // Última modificación
	int ctime;         // Último cambio de metadatos
	                   // .....
} inodo;
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
	inodo *root;  // Inodo raíz del sistema de archivos
} filesystem;


// Variables globales
char *filedisk = DEFAULT_FILE_DISK;
filesystem fs;
