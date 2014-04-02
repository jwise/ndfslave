#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define PAGESZ 8832LL
#define KEYSZ 1024

int main(int argc, char **argv) {
	unsigned char buf[PAGESZ];
	off64_t pn = 0;
	off64_t max = 0;
	
	if (argc != 2) {
		fprintf(stderr, "usage: %s pageno\n", argv[0]);
		return 1;
	}
	
	max = lseek64(0, pn, SEEK_END);
	
	pn = strtoul(argv[1], NULL, 0);
	pn *= PAGESZ;
	
	if (lseek64(0, pn, SEEK_SET) != pn) {
		fprintf(stderr, "%s: hmm...\n", argv[0]);
		return 2;
	}
	
	if (pn >= max) {
		fprintf(stderr, "%s: page too big\n", argv[0]);
		return 2;
	}
	
	pn = read(0, buf, PAGESZ);
	write(1, buf, pn);
	
	return 0;
}
