#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#define PAGESZ 8832L
#define SIGPOS 0x2230
#define SIGSZ  0x1B
#define BLOCKSZ 0x100

#define XFAIL(p) do { if (p) { fprintf(stderr, "%s (%s:%d): operation failed: %s\n", __FUNCTION__, __FILE__, __LINE__, #p); abort(); } } while(0)

struct fwdtab {
	unsigned short phys;
	short confidence;
} __attribute__((packed));

static void _ensure_tab(struct fwdtab **tab, int *tabsz, int p) {
	int i;
	
	if (p < *tabsz)
		return;
	
	*tab = realloc(*tab, (p+1) * sizeof(struct fwdtab));
	XFAIL(!*tab);
	
	for (i = *tabsz; i <= p; i++) {
		(*tab)[i].phys = 0;
		(*tab)[i].confidence = -1;
	}
	*tabsz = p + 1;
}

int main(int argc, char **argv) {
	unsigned char buf[SIGSZ];
	unsigned short sigprob[SIGSZ][256];
	struct fwdtab *tab;
	int tabsz;
	off64_t blockofs = 0x0;
	off64_t maxblock;
	int fd;
	int fdtab;
	
	if (argc != 3) {
		printf("usage: %s filename blocktable\n", argv[0]);
		return 1;
	}
	
	tab = NULL;
	tabsz = 0;
	
	XFAIL((fd = open(argv[1], O_RDONLY)) < 0);
	XFAIL((fdtab = creat(argv[2], 0755)) < 0);
	
	maxblock = lseek64(fd, 0, SEEK_END);
	maxblock /= PAGESZ;
	
	for (blockofs = 0; blockofs <= maxblock; blockofs++) {
		int i;
		
		lseek64(fd, blockofs * (off64_t)PAGESZ + SIGPOS, SEEK_SET);
		read(fd, buf, SIGSZ);
		
		if ((blockofs % BLOCKSZ) == 0)
			memset(sigprob, 0, SIGSZ * 256 * sizeof(short));
		
		for (i = 0; i < SIGSZ; i++)
			if (buf[i] != buf[0])
				break;
		if (i != SIGSZ) /* i.e., this block contains real data */
			for (i = 0; i < SIGSZ; i++)
				sigprob[i][buf[i]]++;
		
		if ((blockofs % BLOCKSZ) == (BLOCKSZ - 1)) {
			unsigned char sig[SIGSZ];
			int confidence = BLOCKSZ;
			unsigned short blockn;
			unsigned short genern;
			
			for (i = 0; i < SIGSZ; i++) {
				unsigned char best = 0;
				int bestn = -1;
				int j;
				
				for (j = 0; j < 256; j++) {
					if (sigprob[i][j] > bestn) {
						best = j;
						bestn = sigprob[i][j];
					}
				}
				
				if (bestn < confidence)
					confidence = bestn;
				
				sig[i] = best;
			}
			
			for (i = 0; i < SIGSZ; i++)
				if (sig[i] != sig[0])
					break;
			if (i == SIGSZ)
				confidence = 0;
			
			blockn = ~(sig[1] << 8 | sig[2]);
			genern = ~(sig[3] << 8 | sig[4]);
			
			switch (genern) {
			case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8:
				confidence += 128;
				break;
			default:
				confidence -= 128;
			}
			
			if (confidence < 0) {
				printf("XXXX: (no confidence): ");
				confidence = 0;
			} else
				printf("%04x: (confidence %3d):", ~(sig[1] << 8 | sig[2]) & 0xFFFF, confidence * 100 / 384);
			printf(" @%05x: ", blockofs / BLOCKSZ);
			for (i = 0; i < SIGSZ; i++)
				printf("%02x ", sig[i]);
			printf("\n");
			
			_ensure_tab(&tab, &tabsz, blockn);
			if (confidence > tab[blockn].confidence) {
				tab[blockn].confidence = confidence;
				tab[blockn].phys = blockofs / BLOCKSZ;
			}
		}
	}
	
	write(fdtab, tab, sizeof(*tab) * tabsz);
	
	return 0;
}
