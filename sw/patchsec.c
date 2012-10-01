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

int main(int argc, char **argv) {
	unsigned char buf[PGSZ];
	int pgn = 0;
	
	while (read(0, buf, PGSZ) == PGSZ) {
		int blk;
		unsigned int *fatp;
		struct vote votes[10] = {0};
		unsigned int voted;
		
		for (blk = 0; blk < NECCBLKS; blk++) {
			int ofs;
			
			fatp = (unsigned int *)(buf + blk * (ECCBLKSZ + ECCOOBSZ));
			for (ofs = 0; ofs < (ECCBLKSZ / 4); ofs++) {
				int fat = fatp[ofs];
				/* FAT1 at 0x30FB */
				int secofs = fat / (512 / 4) + 0x21F6;
				
				secofs /= NECCBLKS * (ECCBLKSZ / 512);
				secofs -= 0x200 /* remember, we're in the second block */;
				
				if (((fat & 0x0FFFFFF0) != 0x0FFFFFF0) && ((fat & 0x0F) != 0))
					vote(votes, sizeof(votes)/sizeof(votes[0]), secofs);
			}
		}
		
		voted = votes[0].value;
		
		if (voted > 0x1000)
			continue;
		
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
		
		printf(" (pgn %03x)\n", pgn);
		
		pgn++;
	}
	
	return 0;
}
