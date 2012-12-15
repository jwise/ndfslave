/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags --libs` lofile.c -o lofile
*/

#define FUSE_USE_VERSION 26

#define _LARGEFILE64_SOURCE

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fuse.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include "../lib/dumpio.h"

static struct dumpio *_io;

static const char *lofile_path = "/lofile";

static int lofile_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0555;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, lofile_path) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = dumpio_size(_io);
	} else
		res = -ENOENT;

	return res;
}

static int lofile_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, lofile_path + 1, NULL, 0);

	return 0;
}

static int lofile_open(const char *path, struct fuse_file_info *fi)
{
	if (strcmp(path, lofile_path) != 0)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int lofile_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi) {
	if(strcmp(path, lofile_path) != 0)
		return -ENOENT;

	return dumpio_pread(_io, buf, size, offset);
}

static struct fuse_operations lofile_oper = {
	.getattr	= lofile_getattr,
	.readdir	= lofile_readdir,
	.open		= lofile_open,
	.read		= lofile_read,
};

int main(int argc, char *argv[])
{
	if (argv < 2) {
		fprintf(stderr, "usage: %s conffile fuseoptions...\n", argv[0]);
		return 1;
	}
	_io = dumpio_init(argv[1]);
	if (!_io)
        	return 1;
	argv[1] = argv[0];
	argc--;
	argv++;
	return fuse_main(argc, argv, &lofile_oper, NULL);
}
