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
				int secofs = fat / (512 / 4);
				
				if (((fat & 0x0FFFFFF0) != 0x0FFFFFF0) && (secofs != 0))
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
		
		printf("-> offsetsec %08x: ", voted);
		
		{
			int prisec = voted + 0x21F6;
			int secsec = voted + 0x30FB;
			int i;
			
			for (i = 0; i < 2; i++) {
				int sec = i ? secsec : prisec;
				
				int pg = sec / 16;
				int logpg;
				int block = pg / 0x200;
				pg = pg % 0x200;
				logpg = pg;
				
				pg = (pg >> 1) + ((pg & 1) << 8);
				
				int cs;
				cs = pg & 1;
				pg = pg >> 1;
				
				printf("%s->sec %08x; cs %d pg %03x pgad" GREEN "%04x+%03x" NORMAL "; ", i ? "fat2" : "fat1", sec, cs, pg, block, logpg);
			}
		}
		
		printf("\n");
	}
	
	return 0;
}
