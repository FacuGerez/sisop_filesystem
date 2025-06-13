#pragma once

#define PARENT_INODE_NOT_DIRECTORY                                             \
	"Error: parent directory is not a directory.\n"
#define PARENT_DIRECTORY_NOT_FOUND "Error: parent directory not found.\n"
#define DIRECTORY_NOT_FOUND "Error: directory not found: %s\n"
#define PARENT_DIRECTORY_FULL                                                  \
	"Error: parent directory is full, cannot create a new directory.\n"
#define DIRECTORY_ALREADY_EXISTS "Error: directory '%s' already exists\n"
#define DIRECTORY_NOT_EMPTY "Error: directory is not empty: %s\n"
#define FILE_ALREADY_EXISTS "Error: file '%s' already exists\n"
#define INODE_NOT_FOUND "Error: directory or file not found for path: %s\n"
#define OFFSET_OUT_OF_BOUNDS "Error: offset out of bounds.\n"
#define INODE_NOT_FILE "Error: inode is not a file.\n"
#define WRITE_EXCEEDS_SIZE "Error: write exceeds file size limit.\n"
