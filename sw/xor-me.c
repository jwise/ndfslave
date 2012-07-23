#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#define XFAIL(p) do { if (p) { fprintf(stderr, "%s (%s:%d): operation failed: %s\n", __FUNCTION__, __FILE__, __LINE__, #p); abort(); } } while(0)

int main(int argc, unsigned char **argv) {
	int i;
	unsigned char *key;
	size_t keylen;
	int fd;
	size_t keyp = 0;
	
	if (argc != 2) {
		fprintf(stderr, "usage: %s keyfile\n", argv[0]);
		return 1;
	}
	
	XFAIL((fd = open(argv[1], O_RDONLY)) < 0);
	keylen = lseek64(fd, 0, SEEK_END);
	XFAIL((key = malloc(keylen)) == NULL);
	lseek64(fd, 0, SEEK_SET);
	XFAIL(read(fd, key, keylen) != keylen);

	unsigned char buf[16384];
	while ((i = read(0, buf, sizeof(buf)))) {
		int j;
		for (j = 0; j < i; j++)
			buf[j] ^= key[(j + keyp) % keylen];
		keyp += i;
		keyp %= keylen;
		write(1, buf, i);
	}
}
