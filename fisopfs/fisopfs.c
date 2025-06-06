#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <string.h>

#include "defs.h"
#include "operations.h"

// Variables globales
char *filedisk = DEFAULT_FILE_DISK;
filesystem fs;

static struct fuse_operations operations = {
	.init = filesystem_init,
	.getattr = filesystem_getattr,
	.mkdir = filesystem_mkdir,
	.readdir = filesystem_readdir,
	//.unlink = filesystem_unlink,
	.rmdir = filesystem_rmdir,
	//.create = filesystem_create,
	//.open = filesystem_open,
	//.write = filesystem_write,
	//.read = filesystem_read,
	.destroy = filesystem_destroy,  // Called on flush.
};

int
main(int argc, char *argv[])
{
	for (int i = 1; i < argc - 1; i++) {
		if (strcmp(argv[i], "--filedisk") == 0) {
			filedisk = argv[i + 1];

			// We remove the argument so that fuse doesn't use our
			// argument or name as folder.
			// Equivalent to a pop.
			for (int j = i; j < argc - 1; j++) {
				argv[j] = argv[j + 2];
			}

			argc = argc - 2;
			break;
		}
	}

	return fuse_main(argc, argv, &operations, NULL);
}
