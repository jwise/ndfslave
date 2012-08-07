#define _LARGEFILE64_SOURCE

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define PAGESZ 8832L
#define CMDOFS 8752L
#define BLOCKSZ 0x100L

#define XFAIL(p) do { if (p) { fprintf(stderr, "%s (%s:%d): operation failed: %s\n", __FUNCTION__, __FILE__, __LINE__, #p); abort(); } } while(0)

int main(int argc, char **argv) {
	int fd;
	off_t ofs;
	int i;
	
	if (argc < 2) {
		fprintf(stderr, "usage: %s cs\n", argv[0]);
		return 1;
	}
	
	XFAIL((fd = open(argv[1], O_RDONLY)) < 0);
	ofs = lseek64(fd, 0, SEEK_END);
	ofs /= PAGESZ * BLOCKSZ;
	
	for (i = 0; i < ofs; i++) {
		unsigned char buf[PAGESZ];
		
		pread64(fd, buf, PAGESZ, (off64_t)PAGESZ * (off64_t)BLOCKSZ * (off64_t)i);
		
		printf("log %02x%02x, phys %04x, command %02x, generation %02x%02x\n",
			~buf[CMDOFS+1] & 0xFF, ~buf[CMDOFS+2] & 0xFF,
			i,
			buf[CMDOFS],
			~buf[CMDOFS+3] & 0xFF, ~buf[CMDOFS+4] & 0xFF);
	}
	
	return 0;
}