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

int main(int argc, char **argv) {
	int ce;
	
	if (argc != 3) {
		fprintf(stderr, "usage: %s devicepath filename\n", argv[0]);
		abort();
	}
	ndf_init(argv[1]);
	
	ndf_ce(3);
	
	printf("resetting NAND device...\n");
	ndf_cmd(0xFF); /* reset */
	ndf_wait();
	
	for (ce = 1; ce < 3; ce++) {
		ndf_ce(ce);
		
		printf("ce%d: reading device ID: ", ce);
		ndf_cmd(0x90); /* ID */
		ndf_adr(0x00);
		printf("%02x ", ndf_read());
		printf("%02x ", ndf_read());
		printf("%02x ", ndf_read());
		printf("%02x ", ndf_read());
		printf("%02x ", ndf_read());
		printf("%02x ", ndf_read());
		printf("\n");
		
		printf("ce%d: reading JEDEC ID: ", ce);
		ndf_cmd(0x90); /* ID */
		ndf_adr(0x40);
		printf("'%c', ", ndf_read());
		printf("'%c', ", ndf_read());
		printf("'%c', ", ndf_read());
		printf("'%c', ", ndf_read());
		printf("'%c', ", ndf_read());
		printf("0x%02x ", ndf_read());
		printf("\n");
	}
	
	struct timeval tv1, tv2;
	
	gettimeofday(&tv1, NULL);
	
	int fd;
	
	fd = creat(argv[2], 0644);
	
	int adr;
#define BYTES_PER_ITER (2L*(8192L+640L))
#define TOTAL 0x100000L
	for (adr = 0; adr < TOTAL; adr++) {
		gettimeofday(&tv2, NULL);
		
		long usec = (tv2.tv_sec - tv1.tv_sec) * 1000000L + (tv2.tv_usec - tv1.tv_usec);
		long remaining = (TOTAL - adr) * (long long)BYTES_PER_ITER;
		float bps = (float)(adr*BYTES_PER_ITER*1000000.0)/(float)usec;
		long secrem = remaining / ((long long)bps + 1);
		
		ndf_ce(3);
		printf("reading page %d / %d (%.2f Bps; %dh%02dm%02ds left)...          \r", adr, TOTAL,
			bps, secrem / 3600, (secrem % 3600) / 60, secrem % 60);
		fflush(stdout);
		unsigned char c;
		int i;
		ndf_cmd(0x00);
		ndf_adr(0x00); /* col */
		ndf_adr(0x00);
		ndf_adr(adr & 0xFF); /* row */
		ndf_adr(adr >> 8);
		ndf_adr(adr >> 16);
	
		ndf_cmd(0x30);
		ndf_wait();
		
		for (ce = 1; ce < 3; ce++) {
			ndf_ce(ce);
			
			unsigned char buf[1024];
			memset(buf, 0xAA, 1024);
			for (i = 0; i < (8192 / 512); i++) {
				ndf_read_many(buf, 512);
				write(fd, buf, 512);
			}
			
			ndf_read_many(buf, 640);
			write(fd, buf, 640);
		}
	}
	
	return 0;
}
