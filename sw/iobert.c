#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/time.h>

typedef void * dmgr_t;

extern int DmgrOpen(dmgr_t *p, char *n);
extern int DeppEnable(dmgr_t *p);
extern int DeppSetTimeout(dmgr_t *p, unsigned int ns, unsigned int *rns);
extern int DeppGetReg(dmgr_t *p, unsigned char reg, unsigned char *data, int bs);
extern int DeppPutReg(dmgr_t *p, unsigned char reg, unsigned char  data, int bs);
extern int DeppGetRegRepeat(dmgr_t *p, unsigned char reg, unsigned char *data, int n, int bs);
extern int DeppPutRegRepeat(dmgr_t *p, unsigned char reg, unsigned char *data, int n, int bs);
extern int DeppDisable(dmgr_t *p);
extern int DmgrClose(dmgr_t *p);

#define XFAIL(p) do { if (p) { fprintf(stderr, "%s (%s:%d): operation failed: %s\n", __FUNCTION__, __FILE__, __LINE__, #p); abort(); } } while(0)

dmgr_t hif;

void ndf_init(char *dev) {
	unsigned int n;
	XFAIL(!DmgrOpen(&hif, dev));
	XFAIL(!DeppEnable(hif));
	XFAIL(!DeppSetTimeout(hif, 10000000 /* 10ms */, &n));
}

void ndf_cmd(unsigned char c) {
	XFAIL(!DeppPutReg(hif, 'C', c, 0));
}

void ndf_adr(unsigned char c) {
	XFAIL(!DeppPutReg(hif, 'A', c, 0));
}

void ndf_adr_many(unsigned char *p, int n) {
	XFAIL(!DeppPutRegRepeat(hif, 'A', p, n, 0));
}

void ndf_wait() {
	unsigned char c;
	XFAIL(!DeppGetReg(hif, 'B', &c, 0));
}

unsigned char ndf_read() {
	unsigned char c;
	XFAIL(!DeppGetReg(hif, 'D', &c, 0));
	return c;
}

void ndf_read_many(unsigned char *p, int n) {
	XFAIL(!DeppGetRegRepeat(hif, 'D', p, n, 0));
}

void ndf_ce(unsigned char c) {
	XFAIL(!DeppPutReg(hif, 'E', ~c, 0));
}

void ndf_cmd_read_page(unsigned int adr) {
	unsigned char adrp[5];
	ndf_cmd(0x00);
	
	adrp[0] = 0x00;
	adrp[1] = 0x00;
	adrp[2] = adr & 0xFF;
	adrp[3] = adr >> 8;
	adrp[4] = adr >> 16;
	
	ndf_adr_many(adrp, 5);
	
	ndf_cmd(0x30);
}

int main(int argc, char **argv) {
	int ce;
	int iter = 1;
	
	if (argc != 2) {
		fprintf(stderr, "usage: %s devicepath\n", argv[0]);
		abort();
	}
	ndf_init(argv[1]);
	
	ndf_ce(3);
	
	printf("resetting NAND device...\n");
	ndf_cmd(0xFF); /* reset */
	ndf_wait();

	struct timeval tv1, tv2;
	
	gettimeofday(&tv1, NULL);
	
#define IOBUFSIZ 8192
	
	while (iter++) {
		gettimeofday(&tv2, NULL);
		
		long usec = (tv2.tv_sec - tv1.tv_sec) * 1000000L + (tv2.tv_usec - tv1.tv_usec);
		float bps = (float)(iter * IOBUFSIZ * 2 * 1000000L)/(float)usec;
		if ((iter % 10) == 0)
			printf("iter %d (%.2f bps, %d bytes xferred)...\r",iter, bps, (iter - 1) * IOBUFSIZ * 2);
		
		for (ce = 1; ce < 3; ce++) {
		
			unsigned char buf[IOBUFSIZ];
			unsigned char good[6] = {'J', 'E', 'D', 'E', 'C', 0x01};
			int i;
			
			ndf_ce(ce);
			
			fflush(stdout);
			ndf_cmd(0x90); /* ID */
			ndf_adr(0x40);
			ndf_wait();
			ndf_read_many(buf, sizeof(buf));
			for (i = 0; i < sizeof(buf); i++)
				if (buf[i] != good[i % sizeof(good)])
					printf("ce%d mismatch: expected %02x, got %02x at ofs %d\n", ce, good[i % sizeof(good)], buf[i], i);
		}
	}
	
	return 0;
}
