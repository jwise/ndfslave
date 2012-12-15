#ifndef DUMPIO_H
#define DUMPIO_H

#define _LARGEFILE64_SOURCE
#include <sys/types.h>

struct fwdtab {
	unsigned short phys;
	short confidence;
} __attribute__((packed));

#define NPATCHES 256
struct patch {
	int sector;
	int pg;
	int confidence;
	int fd;
};

#define PAGESZ 8832L
#define SECBLOCK 1024L
#define ECCSZ 70L
#define SECCNT 8
#define BLOCKSZ 0x200
#define PATCHSECSZ 512L

struct dumpio;

off_t dumpio_size(struct dumpio *io);
off_t dumpio_pread(struct dumpio *io, char *buf, size_t size, off_t offset);
struct dumpio *dumpio_init(int *argc, char **argv[]);

#endif
