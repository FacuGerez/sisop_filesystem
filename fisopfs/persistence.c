#include "persistence.h"

#include <stdlib.h>
#include <string.h>

#define FILE_INDICATOR 'F'
#define DIR_INDICATOR 'D'

void
fread_checked(void *ptr, size_t size, size_t count, FILE *stream)
{
	// This should not fail. Added mostly to silence
	// compiler warnings.
	if (fread(ptr, size, count, stream) != count) {
		fprintf(stderr,
		        "Deserialization error: failed to read %zu bytes\n",
		        size * count);
		exit(EXIT_FAILURE);
	}
}

// Serializes the inode in a recursive depth-first
// manner and saves it into the file.
// NOTE: This simplifies and centralizes code, but
// if stack overflows are a problem, switch to an
// iterative/breath-first serialization.
void
serialize_inode(FILE *file, const inode *node)
{
	// This should never fail, so no fallback
	// case is added.
	fputc(node->file != NULL ? FILE_INDICATOR : DIR_INDICATOR, file);

	// Shared fields.
	fwrite(&node->mode, sizeof(mode_t), 1, file);
	fwrite(&node->nlink, sizeof(nlink_t), 1, file);
	fwrite(&node->uid, sizeof(uid_t), 1, file);
	fwrite(&node->gid, sizeof(gid_t), 1, file);
	fwrite(&node->atime, sizeof(time_t), 1, file);
	fwrite(&node->mtime, sizeof(time_t), 1, file);
	fwrite(&node->ctime, sizeof(time_t), 1, file);
	fwrite(&node->size, sizeof(off_t), 1, file);

	if (node->file != NULL) {
		fwrite(node->file->content, 1, CONTENT_SIZE, file);
	} else {
		fwrite(&node->dir->size, sizeof(int), 1, file);
		for (int i = 0; i < node->dir->size; ++i) {
			const dentry *entry = node->dir->entries[i];
			size_t len = strlen(entry->filename);
			fwrite(&len, sizeof(len), 1, file);
			fwrite(entry->filename, 1, len, file);
			serialize_inode(file, entry->inode);
		}
	}
}

// Deserializes the inode from the file.
inode *
deserialize_inode(FILE *file)
{
	const char type = (char) fgetc(file);

	inode *node = malloc(sizeof(inode));

	// Shared fields.
	fread_checked(&node->mode, sizeof(mode_t), 1, file);
	fread_checked(&node->nlink, sizeof(nlink_t), 1, file);
	fread_checked(&node->uid, sizeof(uid_t), 1, file);
	fread_checked(&node->gid, sizeof(uid_t), 1, file);
	fread_checked(&node->atime, sizeof(time_t), 1, file);
	fread_checked(&node->mtime, sizeof(time_t), 1, file);
	fread_checked(&node->ctime, sizeof(time_t), 1, file);
	fread_checked(&node->size, sizeof(off_t), 1, file);

	// This should never fail, so no fallback
	// case is added.
	if (type == FILE_INDICATOR) {
		node->file = malloc(sizeof(inode_file));
		node->dir = NULL;
		fread_checked(node->file->content, 1, CONTENT_SIZE, file);
	} else {
		node->file = NULL;
		node->dir = malloc(sizeof(inode_dir));
		fread_checked(&node->dir->size, sizeof(int), 1, file);

		for (int i = 0; i < node->dir->size; ++i) {
			dentry *entry = malloc(sizeof(dentry));
			size_t len;
			fread_checked(&len, sizeof(len), 1, file);
			fread_checked(entry->filename, 1, len, file);
			entry->filename[len] = '\0';
			entry->inode = deserialize_inode(file);
			node->dir->entries[i] = entry;
		}
	}

	return node;
}
