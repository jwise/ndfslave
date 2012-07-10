#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#define XFAIL(p) do { if (p) { fprintf(stderr, "%s (%s:%d): operation failed: %s\n", __FUNCTION__, __FILE__, __LINE__, #p); abort(); } } while(0)


int hex2int(unsigned char c) {
	switch (c) {
		case '0': case '1':
		case '2': case '3':
		case '4': case '5':
		case '6': case '7':
		case '8': case '9':
			return c - '0';
		case 'a': case 'b':
		case 'c': case 'd':
		case 'e': case 'f':
			return c - 'a' + 0xA;
		case 'A': case 'B':
		case 'C': case 'D':
		case 'E': case 'F':
			return c - 'A' + 0xA;
		default:
			fprintf(stderr, "bad hex nybble '%c'?\n", c);
			abort();
	}
}

int main(int argc, unsigned char **argv) {
	int i;
	unsigned char *key;
	size_t keylen;
	int fd;
	size_t keyp;
	
	if (argc != 2) {
		fprintf(stderr, "usage: %s keyfile\n", argv[0]);
		return 1;
	}
	
	XFAIL((fd = open(argv[1], O_RDONLY)) < 0);
	keylen = lseek64(fd, 0, SEEK_END);
	key = mmap(NULL, keylen, PROT_READ, MAP_SHARED, fd, 0);
	XFAIL(key == MAP_FAILED);

	unsigned char buf[16384];
	while ((i = read(0, buf, sizeof(buf)))) {
		int j;
		for (j = 0; j < i; j++)
			buf[j] ^= key[(j + keyp) % keylen] ^ 0xFF;
		keyp += i;
		keyp %= keylen;
		write(1, buf, i);
	}
}
