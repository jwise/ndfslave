#define _LARGEFILE64_SOURCE

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "dumpio.h"

#ifdef __APPLE__
#define pread64 pread
#endif

struct dumpio {
	struct fwdtab *tab;
	int tabsz;
	int fd0, fd1;
	struct patch patches[NPATCHES * 2];
};

#define PAGESZ 8832L
#define SECBLOCK 1024L
#define ECCSZ 70L
#define SECCNT 8
#define BLOCKSZ 0x200
#define PATCHSECSZ 512L

#define XFAIL(p) do { if (p) { fprintf(stderr, "%s (%s:%d): operation failed: %s\n", __FUNCTION__, __FILE__, __LINE__, #p); abort(); } } while(0)

static int _read_a_little(struct dumpio *io, char *buf, size_t size, off_t offset)
{
	int block;
	int phys, cs, pg, sec, secofs;
	int i;

	if (offset >= io->tabsz * SECBLOCK * SECCNT * BLOCKSZ)
		return 0;
	
	if ((size + offset % SECBLOCK) > SECBLOCK)
		size = SECBLOCK - (offset % SECBLOCK);
	
	block = offset / (off_t)(SECBLOCK * SECCNT * BLOCKSZ);
	if (io->tab[block].confidence < 0) {
		fprintf(stderr, "*** no mapping for block %04x\n", block);
		memset(buf, 0, size);
		return size;
	}
	fprintf(stderr, "  rq for block %04x, phys %04x, confidence %d\n", block, io->tab[block].phys, io->tab[block].confidence);
	
	phys = io->tab[block].phys;
	sec = offset % (off_t)(SECBLOCK * SECCNT) / (off_t)SECBLOCK;
	secofs = offset % (off_t)SECBLOCK;
	
	for (i = 0; i < NPATCHES; i++)
		if (io->patches[i].sector && ((io->patches[i].sector / 0x10) == (offset / (512 * 0x10)))) {
			/* we have a patch... */
			fprintf(stderr, "  applying patch from fd %d, pg %x for sector %x\n", io->patches[i].fd, io->patches[i].pg, io->patches[i].sector);
			return pread64(io->patches[i].fd, buf, size,
			               io->patches[i].pg * PAGESZ +
			               sec * (SECBLOCK + ECCSZ) +
			               secofs);
		}

	pg = offset % (off_t)(SECBLOCK * SECCNT * BLOCKSZ) / (off_t)(SECBLOCK * SECCNT);
	
	pg = (pg >> 1) + ((pg & 1) << 8);

	cs = pg & 1;
	pg = pg >> 1;
	
	fprintf(stderr, "    offset %08llx -> virtblock %04x, cs %d, pg %02x, sec %d, secofs %02x\n", offset, block, cs, pg, sec, secofs);

	return pread64(cs ? io->fd1 : io->fd0,
	               buf, size,
    	               (phys * (BLOCKSZ / 2) + pg) * PAGESZ +
	               sec * (SECBLOCK + ECCSZ) +
	               secofs);
}

off_t dumpio_size(struct dumpio *io) {
	return io->tabsz * SECBLOCK * SECCNT * BLOCKSZ;
}

off_t dumpio_pread(struct dumpio *io, char *buf, size_t size, off_t offset) {
	size_t retsz = 0;
	
	printf("read request: offset %08llx, size %08lx\n", offset, size);
	while (size) {
		int rv;
		
		rv = _read_a_little(io, buf, size, offset);
		if (rv == 0)
			break;
		size -= rv;
		buf += rv;
		retsz += rv;
		offset += rv;
	}
	
	return retsz;
}

#define SHIFT do { (*argc)--; (*argv)[1] = (*argv)[0]; (*argv)++; } while(0)

struct dumpio *dumpio_init(int *argc, char **argv[]) {
	int fd;
	off_t ofs;
	struct dumpio *io;
	
	XFAIL((io = malloc(sizeof *io)) == NULL);
	
	if (*argc < 4) {
		fprintf(stderr, "usage: %s blocktable cs0 cs1 [+ patchfile patchlist]* fuseoptions...\n", (*argv)[0]);
		return NULL;
	}
	
	XFAIL((fd = open((*argv)[1], O_RDONLY)) < 0);
	ofs = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	XFAIL((io->tab = malloc(ofs)) == NULL);
	read(fd, io->tab, ofs);
	close(fd);
	io->tabsz = ofs / sizeof(struct fwdtab);
	
	XFAIL((io->fd0 = open((*argv)[2], O_RDONLY)) < 0);
	XFAIL((io->fd1 = open((*argv)[3], O_RDONLY)) < 0);
	
	SHIFT; SHIFT; SHIFT;
	while (*argc >= 4 && !strcmp((*argv)[1], "+")) {
		/* load patch files */
		int pfilfd, plisfd;
		int i;
		struct patch thispatches[NPATCHES] = {{0}};
		
		XFAIL((pfilfd = open((*argv)[2], O_RDONLY)) < 0);
		XFAIL((plisfd = open((*argv)[3], O_RDONLY)) < 0);
		
		XFAIL(read(plisfd, thispatches, sizeof(thispatches)) != sizeof(thispatches));
		for (i = 0; i < NPATCHES; i++) {
			int j;
			
			if (thispatches[i].sector == 0)
				break;
			
			for (j = 0; j < NPATCHES * 2; j++)
				if (io->patches[j].sector == 0 || io->patches[j].sector == thispatches[i].sector)
					break;
			if (j == NPATCHES * 2) {
				printf("*** patch table full!\n");
				break;
			}
			if (thispatches[i].confidence > io->patches[j].confidence) {
				printf("installing patch: sector %x -> fd %d, pg %x\n", thispatches[i].sector, pfilfd, thispatches[i].pg);
				io->patches[j].sector = thispatches[i].sector;
				io->patches[j].pg = thispatches[i].pg;
				io->patches[j].confidence = thispatches[i].confidence;
				io->patches[j].fd = pfilfd;
			}
		}
		
		close(plisfd);
		
		SHIFT; SHIFT; SHIFT;
	}
	
	return io;
}
