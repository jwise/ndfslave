/* i.e., find a list of unreferenced pages */

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
	off_t ofs, npgs;
	int i;
	unsigned char *pages;
	
	if (argc < 3) {
		fprintf(stderr, "usage: %s blocktable cs\n", argv[0]);
		return 1;
	}

	XFAIL((fd = open(argv[2], O_RDONLY)) < 0);
	npgs = lseek64(fd, 0, SEEK_END);
	npgs /= PAGESZ;
	pages = malloc(npgs / 8 + 1);
	memset(pages, 0, npgs / 8 + 1);
	
	fprintf(stderr, "searching for interesting pages...\n");
	for (i = 0; i < npgs; i++) {
		unsigned char buf[PAGESZ];
		int j;
		
		if ((i % 100000) == 0)
			fprintf(stderr, "...%d / %d...\n", i, npgs);
		
		
		pread64(fd, buf, PAGESZ, (off64_t)PAGESZ * i);
		for (j = 1; j < 512; j++)
			if (buf[j] != buf[1])
				break;
		if (j == 512)
			continue;
		
		/* found an interesting page */
		pages[i / 8] |= 1 << (i % 8);
	}
	close(fd);

	
	XFAIL((fd = open(argv[1], O_RDONLY)) < 0);
	ofs = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	XFAIL((tab = malloc(ofs)) == NULL);
	read(fd, tab, ofs);
	close(fd);
	tabsz = ofs / sizeof(struct fwdtab);
	
	for (i = 0; i < tabsz; i++) {
		int j;
		
		if (!tab[i].phys)
			continue;
		
		for (j = 0; j < BLOCKSZ; j++) {
			int pn = tab[i].phys * BLOCKSZ + j;
			pages[pn / 8] &= ~(1 << (pn % 8));
		}
	}
	
	printf("Interesting pages:\n");
	for (i = 0; i < npgs; i++) {
		if (pages[i/8] & (1 << (i % 8))) {
			printf("  %5x\n", i);
		}
	}
	
	return 0;
}