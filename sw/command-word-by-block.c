#define _LARGEFILE64_SOURCE

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

struct fwdtab {
	unsigned short phys;
	short confidence;
} __attribute__((packed));

#define PAGESZ 8832L
#define CMDOFS 8752L
#define BLOCKSZ 0x100L

#define XFAIL(p) do { if (p) { fprintf(stderr, "%s (%s:%d): operation failed: %s\n", __FUNCTION__, __FILE__, __LINE__, #p); abort(); } } while(0)

int main(int argc, char **argv) {
	static struct fwdtab *tab;
	static int tabsz;
	int fd;
	off_t ofs;
	int i;
	
	if (argc < 3) {
		fprintf(stderr, "usage: %s blocktable cs\n", argv[0]);
		return 1;
	}
	
	XFAIL((fd = open(argv[1], O_RDONLY)) < 0);
	ofs = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	XFAIL((tab = malloc(ofs)) == NULL);
	read(fd, tab, ofs);
	close(fd);
	tabsz = ofs / sizeof(struct fwdtab);
	
	XFAIL((fd = open(argv[2], O_RDONLY)) < 0);
	for (i = 0; i < tabsz; i++) {
		unsigned char buf[PAGESZ];
		
		if (!tab[i].phys)
			continue;
		
		pread64(fd, buf, PAGESZ, (off64_t)PAGESZ * (off64_t)BLOCKSZ * (off64_t)tab[i].phys);
		
		printf("log %04x (conf %d), phys %04x, command %02x, generation %02x%02x\n", i, tab[i].confidence, tab[i].phys, buf[CMDOFS], ~buf[CMDOFS+3] & 0xFF, ~buf[CMDOFS+4] & 0xFF);
		
		if (((~buf[CMDOFS+1] & 0xFF) << 8 | (~buf[CMDOFS+2] & 0xFF)) != i)
			printf("  unexpected logical sector %02x%02x?\n", ~buf[CMDOFS+1] & 0xFF, ~buf[CMDOFS+2] & 0xFF);
	}
	
	return 0;
}