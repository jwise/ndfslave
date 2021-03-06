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
	
	if (argc != 4) {
		fprintf(stderr, "usage: %s devicepath filename0 filename1\n", argv[0]);
		abort();
	}
	ndf_init(argv[1]);
	
	ndf_ce(3);
	
	printf("resetting NAND device...\n");
	ndf_cmd(0xFF); /* reset */
	ndf_wait();
	
	for (ce = 1; ce < 3; ce++) {
		unsigned char buf[6];
		int i;
		
		ndf_ce(ce);
		
		printf("ce%d: reading device ID: ", ce);
		ndf_cmd(0x90); /* ID */
		ndf_adr(0x00);
		ndf_wait();
		ndf_read_many(buf, 6);
		for (i = 0; i < 6; i++)
			printf("%02x ", buf[i]);
		printf("\n");
		
		printf("ce%d: reading JEDEC ID: ", ce);
		ndf_cmd(0x90); /* ID */
		ndf_adr(0x40);
		ndf_wait();
		ndf_read_many(buf, 6);
		for (i = 0; i < 5; i++)
			printf("'%c' ", buf[i]);
		printf("0x%02x ", buf[5]);
		printf("\n");
		
		XFAIL(memcmp(buf, "JEDEC", 5));
	}
	
	int fd0, fd1;
	
	XFAIL((fd0 = creat(argv[2], 0644)) < 0);
	XFAIL((fd1 = creat(argv[3], 0644)) < 0);
	
	int adr;
#define PAGE_SZ (8832L)
#define TOTAL 0x180000L

	for (ce = 1; ce < 3; ce++) {
		struct timeval tv1, tv2;

		int fd = (ce == 1) ? fd0 : fd1;
		ndf_ce(ce);
		
		gettimeofday(&tv1, NULL);
		
		for (adr = 0; adr < TOTAL; adr++) {
			unsigned char buf[PAGE_SZ];
		
			gettimeofday(&tv2, NULL);
			
			long usec = (tv2.tv_sec - tv1.tv_sec) * 1000000L + (tv2.tv_usec - tv1.tv_usec);
			long remaining = (TOTAL - adr) * (long long)PAGE_SZ;
			float bps = (float)(adr*PAGE_SZ*1000000.0)/(float)usec;
			long secrem = remaining / ((long long)bps + 1);
			
			if ((adr % 10) == 0) {
				printf("ce%d: reading page %d / %d (%.2f Bps; %dh%02dm%02ds left)...          \r",
					ce, adr, TOTAL, bps, secrem / 3600, (secrem % 3600) / 60, secrem % 60);
				fflush(stdout);
			}
			
			ndf_cmd_read_page(adr);
			ndf_read_many(buf, sizeof(buf));
			write(fd, buf, sizeof(buf));
		}
		printf("\n");
	}
	
	DeppDisable(hif);
	DmgrClose(hif);
	
	return 0;
}
