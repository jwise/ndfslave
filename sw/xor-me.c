#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
	int keylen;
	
	if (argc != 2) {
		fprintf(stderr, "usage: %s byte-string\n", argv[0]);
		return 1;
	}
	
	key = strdup(argv[1]);
	for (i = 0; key[i*2]; i++) {
		unsigned char c1, c2;
		
		c1 = key[i*2];
		c2 = key[i*2+1];
		if (!c2) {
			fprintf(stderr, "%s: not an even number of nybbles?\n", argv[0]);
			return 1;
		}
		
		key[i] = hex2int(c1) << 4 | hex2int(c2);
	}
	keylen = i;
	
	unsigned char buf[4096];
	while ((i = read(0, buf, sizeof(buf)))) {
		int j;
		for (j = 0; j < i; j += 4) {
			buf[j] ^= key[j % keylen];
//			buf[j] = ((buf[j] & 0xAA) >> 1) | ((buf[j] & 0x55) << 1);
//			buf[j] = ((buf[j] & 0xCC) >> 2) | ((buf[j] & 0x33) << 2);
//			buf[j] = ((buf[j] & 0xF0) >> 4) | ((buf[j] & 0x0F) << 4);
		}
		write(1, buf, i);
	}
}