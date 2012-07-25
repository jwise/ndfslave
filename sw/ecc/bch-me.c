#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "bch.h"

#define PAGESZ 8832
#define NPPAGE 8
#define DATALEN 1024
#define ECCLEN 70

#define XFAIL(p) do { if (p) { fprintf(stderr, "%s (%s:%d): operation failed: %s\n", __FUNCTION__, __FILE__, __LINE__, #p); abort(); } } while(0)

volatile int stop = 0;

void sigint(int sig) {
	stop = 1;
}

int main(int argc, unsigned char **argv) {
	int i;
	unsigned char *key;
	unsigned char *syndromekey;
	size_t keylen;
	int fd;
	size_t keyp = 0;
	struct bch_control *bch;
	
	if (argc != 2) {
		fprintf(stderr, "usage: %s keyfile\n", argv[0]);
		return 1;
	}
	
	XFAIL((fd = open(argv[1], O_RDONLY)) < 0);
	keylen = lseek64(fd, 0, SEEK_END);
	XFAIL((key = malloc(keylen)) == NULL);
	lseek64(fd, 0, SEEK_SET);
	XFAIL(read(fd, key, keylen) != keylen);
	
	XFAIL((keylen % PAGESZ) != 0);
	keylen /= PAGESZ;
	
	fprintf(stderr, "Precomputing syndrome modifier...\n");
	
	bch = bch_init(1024, 70);
	XFAIL(!bch);
	
	XFAIL((syndromekey = malloc(keylen * ECCLEN)) == NULL);
	for (i = 0; i < keylen; i++) {
		int j;
		
		memset(syndromekey + i*ECCLEN, 0, ECCLEN);
		bch_encode(bch, key + i*PAGESZ, DATALEN, syndromekey + i*ECCLEN);
		for (j = 0; j < ECCLEN; j++)
			syndromekey[i*ECCLEN + j] ^= key[i*PAGESZ + DATALEN + j];
	}
	
	free(key);

	fprintf(stderr, "%d pages precomputed\n", keylen);
	
	signal(SIGINT, sigint);

	unsigned char buf[PAGESZ];
	keyp = 0;
	int pgs = 0;
	int pkts = 0;
	int toterrs = 0;
	int pkterrs = 0;
	int nonpgs = 0;
	while (read(0, buf, sizeof(buf)) == sizeof(buf) && !stop) {
		int subpg;
		int uncor = 0;
		
		keyp %= keylen;
		
		/* First check to see if this "can't possibly be right". */
		for (i = 0; i < 512; i++)
			if (buf[i] != buf[0])
				break;
		if (i == 512) {
			/* That's not right.  That's not even wrong. */
			keyp++;
			pgs++;
			write(1, buf, sizeof(buf));
			continue;
		}
		
		for (subpg = 0; subpg < PAGESZ / (DATALEN + ECCLEN); subpg++) {
			unsigned char *p = buf + subpg * (DATALEN + ECCLEN);
			int rv;
			unsigned int errs[ECCLEN];
			
			/* Start off by exoring in the syndrome key. */
			for (i = 0; i < ECCLEN; i++)
				p[DATALEN+i] ^= syndromekey[keyp * ECCLEN + i];
			
			rv = bch_decode(bch, p, DATALEN, p + DATALEN, NULL, NULL, errs);
			if (rv < 0)
				uncor++;
			
			if (rv > 0) {
				pkterrs++;
				toterrs += rv;
				
				for (i = 0; i < rv; i++)
					p[errs[i] >> 3] ^= (1 << (errs[i] & 7));
			}
			
			/* Put the key back, so it's safe to do this again later if we want. */
			for (i = 0; i < ECCLEN; i++)
				p[DATALEN+i] ^= syndromekey[keyp * ECCLEN + i];
			
			pkts++;
		}
		
		if (uncor != 0 && uncor != subpg)
			fprintf(stderr, "*** %d uncorrectable ECC errors at page %d\n", uncor, pgs);
		if (uncor == subpg)
			nonpgs++;
		
		keyp++;
		pgs++;
		write(1, buf, sizeof(buf));
	}
	fprintf(stderr, "Checked %d data packets (%d with errors); corrected %d total errors\n", pkts, pkterrs, toterrs);
	fprintf(stderr, "Checked %d total pages, of which %d contained fully uncorrectable data (i.e., non-standard pages)\n", pgs, nonpgs);
	fprintf(stderr, "Average BER: %0.2e\n",
		(float)toterrs /
		((float)pkts * (float)((DATALEN + ECCLEN) * 8)));
}
