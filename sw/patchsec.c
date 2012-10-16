#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/time.h>

#define XFAIL(p) do { if (p) { fprintf(stderr, "%s (%s:%d): operation failed: %s\n", __FUNCTION__, __FILE__, __LINE__, #p); abort(); } } while(0)

#define ECCBLKSZ 0x400
#define ECCOOBSZ 0x46
#define NECCBLKS 8
#define BLKOOBSZ 0x50
#define PGSZ ((ECCBLKSZ+ECCOOBSZ)*8+BLKOOBSZ)

#define GREEN "\033[01;32m"
#define NORMAL "\033[0m"

struct vote {
	unsigned int value;
	int votes;
};

int vcomp(const void *a, const void *b) {
	const struct vote *_a = a;
	const struct vote *_b = b;
	return _a->votes - _b->votes;
}

void vote(struct vote *votes, int nvotes, int value) {
	int i;
	
	for (i = 0; i < nvotes; i++) {
		if (votes[i].votes == 0)
			break;
		if (votes[i].value == value) {
			votes[i].votes++;
			qsort(votes, nvotes, sizeof(struct vote), vcomp);
		}
	}
	
	if (i == nvotes)
		return;
	
	votes[i].value = value;
	votes[i].votes = 1;
}

#define NPATCHES 256
struct patch {
	int sector;
	int pg;
	int confidence;
	int fd;
};

int main(int argc, char **argv) {
	unsigned char buf[PGSZ];
	struct patch patches[NPATCHES] = {{0}};
	int pgn = 0;
	int fd;
	int ofd = -1;
	
	if (argc != 2 && argc != 3) {
		fprintf(stderr, "usage: %s patchblock [patchlist]\n", argv[0]);
		return 1;
	}
	XFAIL((fd = open(argv[1], O_RDONLY)) < 0);
	if (argc == 3)
		XFAIL((ofd = creat(argv[2], 0644)) < 0);
	
	while (read(fd, buf, PGSZ) == PGSZ) {
		int blk;
		unsigned int *fatp;
		struct vote votes[10] = {0};
		unsigned int voted;
		
		for (blk = 0; blk < NECCBLKS; blk++) {
			int ofs;
			
			fatp = (unsigned int *)(buf + blk * (ECCBLKSZ + ECCOOBSZ));
			for (ofs = 0; ofs < (ECCBLKSZ / 4); ofs++) {
				int fat = fatp[ofs];
				int secofs = (fat - 1) / (512 / 4);
				
				/* Compensate for position from start of page. */
				if (ofs >= (512 / 4))
					secofs--;
				secofs -= blk * 2;
				
				if (((fat & 0x0FFFFFF0) != 0x0FFFFFF0) && ((fat & 0x0F) != 0))
					vote(votes, sizeof(votes)/sizeof(votes[0]), secofs);
			}
		}
		
		voted = votes[0].value;
		
		if (voted > 0x1000) {
			pgn++;
			continue;
		}
		
		if (votes[0].votes < 200) {
			pgn++;
			continue;
		}
		
#if 0
		{
			int i;
			
			for (i = 0x2230; i < 0x224A; i++)
				printf("%02x ", (unsigned char)~buf[i]);
		}
		
		{
			int pg = voted;

			int logpg = pg;
				
			pg = (pg >> 1) + ((pg & 1) << 8);

			int cs;
			cs = pg & 1;
			pg = pg >> 1;
			
			printf("-> fat0 logpg " GREEN "%03x" NORMAL " (%5d votes); cs %d pg %02x", logpg, votes[0].votes, cs, pg);
		}
#endif
		printf("FAT sector offset %03x (F0 %04x, F1 %04x) (%5d votes) (representative FAT %08x)", voted, voted + 0x21F6, voted + 0x30FB, votes[0].votes, fatp[0]);
		printf(" (pgn %03x @ %s)\n", pgn, argv[1]);
		
		{
			int fat0ad = voted + 0x21F6;
			int fat1ad = voted + 0x30FB;
			int isfat0 = (fat0ad & 0xF) == 0x0;
			int isfat1 = (fat1ad & 0xF) == 0x0;
			if (isfat0 || isfat1) {
				int sector = isfat0 ? fat0ad : fat1ad;
				int i;
				
				for (i = 0; i < NPATCHES; i++)
					if (patches[i].sector == 0 || patches[i].sector == sector)
						break;
				if (i == NPATCHES) 
					printf("*** too many patches!\n");
				else if (votes[0].votes >= patches[i].confidence) {
					patches[i].fd = -1;
					patches[i].confidence = votes[0].votes;
					patches[i].sector = sector;
					patches[i].pg = pgn;
				}
			}
		}

		pgn++;
	}
	
	if (ofd >= 0)
		write(ofd, patches, sizeof(patches));
	
	return 0;
}
