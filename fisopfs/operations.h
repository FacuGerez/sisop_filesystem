#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <fuse.h>
#include <sys/types.h>

void *filesystem_init(struct fuse_conn_info *conn);

int filesystem_getattr(const char *path, struct stat *stbuf);

int filesystem_mkdir(const char *path, mode_t mode);

int filesystem_readdir(const char *path,
                       void *buf,
                       fuse_fill_dir_t filler,
                       off_t offset,
                       struct fuse_file_info *fi);

int filesystem_unlink(const char *path);

int filesystem_rmdir(const char *path);

int filesystem_utimens(const char *path, const struct timespec tv[2]);

int filesystem_create(const char *path, mode_t mode, struct fuse_file_info *fi);

int filesystem_open(const char *path, struct fuse_file_info *fi);

int filesystem_write(const char *path,
                     const char *buf,
                     size_t size,
                     off_t offset,
                     struct fuse_file_info *fi);

int filesystem_read(const char *path,
                    char *buf,
                    size_t size,
                    off_t offset,
                    struct fuse_file_info *fi);

void filesystem_destroy(void *private_data);

#endif
