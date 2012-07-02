#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>

#define PAGESZ 8832L
#define KEYSZ 1024

void main() {
	unsigned char buf[PAGESZ];
	unsigned char needle[] = { 0x01, 0x86, 0x05, 0x14 };
	unsigned int offsets[PAGESZ] = {0};
	unsigned int probs[KEYSZ][256] = {{0}};
	unsigned int pn = 0;
	
	while (read(0, buf, PAGESZ) == PAGESZ) {
		unsigned char *p = buf;
		int printed4 = 0;
        	while ((p = memmem(p, PAGESZ - (p - buf), needle, sizeof(needle))) != NULL) {
        		int i;
        		unsigned long ofs = p - buf;
        		
        		printf("...P%06x...\r", pn);
        		fflush(stdout);
        		
        		/* Maybe worth doing probabilistic hashing on pages? */
			offsets[ofs]++;
			
			for (i = 0; i < KEYSZ; i++)
				if ((p - buf) + i < PAGESZ)
					probs[i][p[i]]++;
			
        		if (p != (buf + PAGESZ))
        			p++;
        	}
        	pn++;
	}
	
	int i;
	for (i = 0; i < PAGESZ; i++)
		if (offsets[i])
			printf("ofs %4d: %4d hits\n", i, offsets[i]);
	
	for (i = 0; i < KEYSZ; i++) {
		int c;
		int bn;
		int tot = 0;
		
		unsigned char best[4] = {0};
		printf("most likely for position %4d: ", i);
		
		for (c = 0; c < 256; c++) {
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
}
