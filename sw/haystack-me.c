#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#define XFAIL(p) do { if (p) { fprintf(stderr, "%s (%s:%d): operation failed: %s\n", __FUNCTION__, __FILE__, __LINE__, #p); abort(); } } while(0)

#define PAGESZ 8832L
#define KEYSZ 256
#define NOFFSETS 64
#define NPGS_PER_ITER 64
#define MAXPGS 0x90000

const unsigned char needle[] = { 0x01, 0x86, 0x05, 0x14 };


struct offset {
	int nhits;
	unsigned int page[NOFFSETS];
};

int main(int argc, char **argv) {
	int fd, i, j;
	unsigned char *buf;
	static struct offset offsets[PAGESZ] = {{0}};
	static unsigned int probs[KEYSZ][256] = {{0}};
	unsigned int pn = 0;
	unsigned int pgmax;
	
	if (argc != 2) {
		printf("usage: %s filename\n", argv[0]);
		return 1;
	}
	
	XFAIL((fd = open(argv[1], O_RDONLY)) < 0);
	pgmax = lseek64(fd, 0, SEEK_END) / PAGESZ;
	if (pgmax > MAXPGS)
		pgmax = MAXPGS;
	buf = mmap(NULL, pgmax * PAGESZ, PROT_READ, MAP_SHARED, fd, 0);
	XFAIL(buf == MAP_FAILED);
	
	for (i = 0; i < pgmax; i++) {
		unsigned char *pstart = buf + PAGESZ * i;
		unsigned char *p = pstart;
		while ((p = memmem(p, PAGESZ - (p - pstart), needle, sizeof(needle))) != NULL) {
			unsigned long ofs = p - pstart;
			
			printf("...P%06x...\r", pn);
			fflush(stdout);
			
			offsets[ofs].page[pn % NOFFSETS]++;
			offsets[ofs].nhits++;
			
			for (j = 0; j < KEYSZ; j++)
				if ((p - pstart) + j < PAGESZ)
					probs[j][p[j]]++;
			
			if (p != (pstart + PAGESZ))
				p++;
		}
		pn++;
	}
	
	printf("all row statistics ANDed with %x\n", NOFFSETS - 1);
	for (i = 0; i < PAGESZ; i++)
		for (j = 0; j < NOFFSETS; j++)
			if (offsets[i].page[j])
				printf("row...%02x, col %4d: %4d hits\n", j, i, offsets[i].page[j]);
	
	for (i = 0; i < KEYSZ; i++) {
		int c;
		int bn;
		int tot = 0;
		
		unsigned char best[4] = {0};
		printf("most likely for position %4d: ", i);
		
		for (c = 0; c < 0x100; c++) {
			for (bn = 0; bn < sizeof(best); bn++)
				if (probs[i][c] > probs[i][best[bn]]) {
					memmove(best+bn+1, best+bn, sizeof(best) - bn - 1);
					best[bn] = c;
					break;
				}
			tot += probs[i][c];
		}
		
		for (bn = 0; bn < sizeof(best); bn++)
			printf("  %6.2f%: 0x%02x  ", (float)probs[i][best[bn]] * 100.0f / (float)tot, best[bn]);
		printf("\n");
	}
	
	return 0;
}
