#include "bch.h"
#include <stdio.h>

#define XFAIL(p) do { if (p) { fprintf(stderr, "%s (%s:%d): operation failed: %s\n", __FUNCTION__, __FILE__, __LINE__, #p); abort(); } } while(0)

void main() {
	struct bch_control *bch;
	unsigned char databuf[1024];
	unsigned char eccbuf[70] = {0};
	
	bch = bch_init(1024, 70);
	XFAIL(!bch);
	
	fprintf(stderr, "initialized bch code with m=%d, n=%d, t=%d\n", bch->m, bch->n, bch->t);
	
	read(0, databuf, 1024);
	bch_encode(bch, databuf, 1024, eccbuf);
	write(1, eccbuf, 70);
}
