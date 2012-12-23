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

#ifdef DUMPIO_DEBUG
#  define DEBUG(...) fprintf(stderr, __VA_ARGS)
#else
#  define DEBUG(...)
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
		DEBUG("*** no mapping for block %04x\n", block);
		memset(buf, 0, size);
		return size;
	}
	DEBUG("  rq for block %04x, phys %04x, confidence %d\n", block, io->tab[block].phys, io->tab[block].confidence);
	
	phys = io->tab[block].phys;
	sec = offset % (off_t)(SECBLOCK * SECCNT) / (off_t)SECBLOCK;
	secofs = offset % (off_t)SECBLOCK;
	
	for (i = 0; i < NPATCHES; i++)
		if (io->patches[i].sector && ((io->patches[i].sector / 0x10) == (offset / (512 * 0x10)))) {
			/* we have a patch... */
			DEBUG("  applying patch from fd %d, pg %x for sector %x\n", io->patches[i].fd, io->patches[i].pg, io->patches[i].sector);
			return pread64(io->patches[i].fd, buf, size,
			               io->patches[i].pg * PAGESZ +
			               sec * (SECBLOCK + ECCSZ) +
			               secofs);
		}

	pg = offset % (off_t)(SECBLOCK * SECCNT * BLOCKSZ) / (off_t)(SECBLOCK * SECCNT);
	
	pg = (pg >> 1) + ((pg & 1) << 8);

	cs = pg & 1;
	pg = pg >> 1;
	
	DEBUG("    offset %08llx -> virtblock %04x, cs %d, pg %02x, sec %d, secofs %02x\n", offset, block, cs, pg, sec, secofs);

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
	
	DEBUG("read request: offset %08llx, size %08lx\n", offset, size);
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

static char *_fgetln(FILE *fp) {
	size_t sz;
	char *l;
	char *l2;
	
	l = fgetln(fp, &sz);
	if (!l)
		return NULL;
	
	XFAIL((l2 = malloc(sz + 1)) == NULL);
	strncpy(l2, l, sz);
	l2[sz] = 0;
	
	l = strchr(l2, '\n');
	if (l)
		*l = 0;
	return l2;
}

struct dumpio *dumpio_init(char *conf) {
	int fd;
	off_t ofs;
	struct dumpio *io;
	FILE *fp;
	char *namebuf;
	
	XFAIL((fp = fopen(conf, "r")) == NULL);
	XFAIL((io = malloc(sizeof *io)) == NULL);
	memset(io, 0, sizeof *io);
	
	XFAIL((namebuf = _fgetln(fp)) == NULL && "blocktable");
	XFAIL((fd = open(namebuf, O_RDONLY)) < 0);
	ofs = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	XFAIL((io->tab = malloc(ofs)) == NULL);
	read(fd, io->tab, ofs);
	close(fd);
	io->tabsz = ofs / sizeof(struct fwdtab);
	free(namebuf);
	
	XFAIL((namebuf = _fgetln(fp)) == NULL && "cs0");
	XFAIL((io->fd0 = open(namebuf, O_RDONLY)) < 0);
	free(namebuf);
	XFAIL((namebuf = _fgetln(fp)) == NULL && "cs1");
	XFAIL((io->fd1 = open(namebuf, O_RDONLY)) < 0);
	free(namebuf);
	
	while ((namebuf = _fgetln(fp)) != NULL) {
		/* load patch files */
		int pfilfd, plisfd;
		int i;
		struct patch thispatches[NPATCHES] = {{0}};
		
		XFAIL((pfilfd = open(namebuf, O_RDONLY)) < 0);
		free(namebuf);
		XFAIL((namebuf = _fgetln(fp)) == NULL && "patchlist");
		XFAIL((plisfd = open(namebuf, O_RDONLY)) < 0);
		free(namebuf);
		
		XFAIL(read(plisfd, thispatches, sizeof(thispatches)) != sizeof(thispatches));
		for (i = 0; i < NPATCHES; i++) {
			int j;
			
			if (thispatches[i].sector == 0)
				break;
			
			for (j = 0; j < NPATCHES * 2; j++)
				if (io->patches[j].sector == 0 || io->patches[j].sector == thispatches[i].sector)
					break;
			if (j == NPATCHES * 2) {
				fprintf(stderr, "*** patch table full!\n");
				break;
			}
			if (thispatches[i].confidence > io->patches[j].confidence) {
				DEBUG("installing patch: sector %x -> fd %d, pg %x\n", thispatches[i].sector, pfilfd, thispatches[i].pg);
				io->patches[j].sector = thispatches[i].sector;
				io->patches[j].pg = thispatches[i].pg;
				io->patches[j].confidence = thispatches[i].confidence;
				io->patches[j].fd = pfilfd;
			}
		}
		
		close(plisfd);
	}
	
	fclose(fp);
	
	return io;
}
