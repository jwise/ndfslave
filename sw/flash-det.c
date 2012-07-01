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


void probe_onfi(unsigned char adr) {
	unsigned char buf[512];
	int i;

	printf("Reading ONFI-like device ID table (cmd 0xEC, adr 0x%02x): ", adr);
	ndf_cmd(0xEC); /* extended ID */
	ndf_adr(adr);
	ndf_wait();
	ndf_read_many(buf, 512);
	for (i = 0; i < 512; i++)
		printf("'%c' ", buf[i]);
	printf("\n");
}

void decode_id_buf(unsigned char *buf) {
	printf("%02x:\n", buf[2]);
	switch (buf[2] & 0x3) {
	case 0x00:
		printf("    (single-device package)\n");
		break;
	case 0x01:
		printf("    (dual-device package)\n");
		break;
	case 0x02:
		printf("    (quad-device package)\n");
		break;
	case 0x03:
		printf("    (8-device package)\n");
		break;
	}
	
	switch (buf[2] & 0xC) {
	case 0x00:
		printf("    (2-level cell)\n");
		break;
	case 0x04:
		printf("    (4-level cell)\n");
		break;
	case 0x08:
		printf("    (8-level cell)\n");
		break;
	case 0x0C:
		printf("    (16-level cell -- GET YO ECC ON!)\n");
		break;
	}
	
	switch (buf[2] & 0x30) {
	case 0x00:
		printf("    (one simulataneously programmed page)\n");
		break;
	case 0x10:
		printf("    (two simultaneously programmed pages)\n");
		break;
	case 0x20:
		printf("    (four simultaneously programmed pages)\n");
		break;
	case 0x30:
		printf("    (eight simultaneously programmed pages)\n");
		break;
	}
	
	switch (buf[2] & 0x40) {
	case 0x00:
		printf("    (multi-chip program interleaving not supported)\n");
		break;
	case 0x40:
		printf("    (multi-chip program interleaving supported)\n");
		break;
	}
	
	switch (buf[2] & 0x80) {
	case 0x00:
		printf("    (cache program not supported)\n");
		break;
	case 0x80:
		printf("    (cache program supported)\n");
		break;
	}
	
	printf("%02x:\n", buf[3]);
	printf("    (%d-byte page size)\n", 2048 << (buf[3] & 0x3));
	switch (buf[3] & 0x4C) {
	case 0x04:
		printf("    (128-byte OOB)\n");
		break;
	case 0x08:
		printf("    (218-byte OOB)\n");
		break;
	case 0x0C:
		printf("    (400-byte OOB)\n");
		break;
	case 0x40:
		printf("    (436-byte OOB)\n");
		break;
	case 0x44:
		printf("    (640-byte OOB)\n");
		break;
	default:
		printf("    (unknown OOB 0x%02x)\n", buf[3] & 0x4C);
	}
	
	switch (buf[3] & 0xB0) {
	case 0x00:
		printf("    (128KB block size)\n");
		break;
	case 0x10:
		printf("    (256KB block size)\n");
		break;
	case 0x20:
		printf("    (512KB block size)\n");
		break;
	case 0x30:
		printf("    (1MB block size)\n");
		break;
	default:
		printf("    (unknown block size 0x%02x)\n", buf[3] & 0xB0);
	}
	
	printf("%02x:\n", buf[4]);
	if (buf[4] & 0x83)
		printf("    (reserved bits aren't: 0x%02x)\n", buf[4] & 0x83);
	
	printf("    (plane %d)\n", 1 << ((buf[4] & 0x0C) >> 2));
	
	switch (buf[4] & 0x70) {
	case 0x00:
		printf("    (1b / 512B ECC)\n");
		break;
	case 0x10:
		printf("    (2b / 512B ECC)\n");
		break;
	case 0x20:
		printf("    (4b / 512B ECC)\n");
		break;
	case 0x30:
		printf("    (8b / 512B ECC)\n");
		break;
	case 0x40:
		printf("    (16b / 512B ECC)\n");
		break;
	case 0x50:
		printf("    (24b / 1KB ECC)\n");
		break;
	case 0x60:
		printf("    (40b / 1KB ECC)\n");
		break;
	default:
		printf("    (unknown ECC level 0x%02x)\n", buf[4] & 0x70);
	}
	
	printf("%02x:\n", buf[5]);
	if (buf[5] & 0x38)
		printf("    (reserved bits aren't: 0x%02x)\n", buf[5] & 0x38);
	switch (buf[5] & 0x7) {
	case 0x00:
		printf("    (50nm flash)\n");
		break;
	case 0x01:
		printf("    (40nm flash)\n");
		break;
	case 0x02:
		printf("    (30nm flash)\n");
		break;
	case 0x03:
		printf("    (20nm flash)\n");
		break;
	default:
		printf("    (unknown flash process size 0x%02x)\n", buf[5] & 0x7);
	}
	if (buf[5] & 0x40)
		printf("    (EDO supported)\n");
	else
		printf("    (EDO not supported)\n");
	if (buf[5] & 0x80)
		printf("    (conventional interface)\n");
	else
		printf("    (toggle mode interface)\n");
}

#define PG_MAX 4096
#define BUF_MAX 16384
#define SEARCH_SZ 64
void probe_actual_size() {
	unsigned char buf[BUF_MAX];
	int i;
	int bign = 0;
	int npages = 0;
	
	printf("Searching for highest bits set...\n");
	for (i = 0; i < PG_MAX; i++) {
		int j;
		
		ndf_cmd_read_page(i);
		ndf_wait();
		
		ndf_read_many(buf, SEARCH_SZ);
		for (j = 0; j < SEARCH_SZ; j++)
			if (buf[j] != 0xFF)
				break;
		if (j == SEARCH_SZ) /* no data */
			continue;
		
		npages++;
		
		/* Now read the whole page. */
		ndf_read_many(buf + SEARCH_SZ, BUF_MAX - SEARCH_SZ);
		
		for (j = 0; j < BUF_MAX; j++)
			if (buf[j] != 0x00)
				if ((j + 1) > bign)
					bign = (j + 1);
	}
	
	printf("...looks like pages have %d bytes (saw %d pages with data)\n", bign, npages);
}

void detect(int ce) {
	unsigned char buf[BUF_MAX];
	int i;
	
	printf("CE: %d\n", ce);
	
	ndf_ce(ce);
	printf("Resetting...\n");
	ndf_cmd(0xFF);
	ndf_wait();

	/* Everybody should have at least the first six bytes. */
	printf("Reading device ID (cmd 0x90, adr 0x00): ");
	ndf_cmd(0x90); /* ID */
	ndf_adr(0x00);
	ndf_wait();
	ndf_read_many(buf, 6);
	for (i = 0; i < 6; i++)
		printf("%02x ", buf[i]);
	printf("\n");
	
	decode_id_buf(buf);

	/* Maybe an ONFI flash? */
	printf("Probing for ONFI flash (cmd 0x90, adr 0x20): ");
	ndf_cmd(0x90); /* ID */
	ndf_adr(0x20);
	ndf_wait();
	ndf_read_many(buf, 4);
	if (!memcmp(buf, "ONFI", 4)) {
		printf("\"ONFI\"\n");
		probe_onfi(0x40);
	} else
		printf("not an ONFI device\n");
	
	printf("Reading JEDEC ID (cmd 0x90, adr 0x40): ");
	ndf_cmd(0x90); /* ID */
	ndf_adr(0x40);
	ndf_wait();
	ndf_read_many(buf, 6);
	if (memcmp(buf, "JEDEC", 5)) 
		printf("not a JEDEC device\n");
	else {
		printf("\"JEDEC\" 0x%02x\n", buf[5]);
		switch (buf[5]) {
		case 0x01:
			printf("  (legacy asynchronous SDR)\n");
			break;
		case 0x02:
			printf("  (toggle mode DDR)\n");
			break;
		case 0x03:
			printf("  (synchronous DDR)\n");
			break;
		default:
			printf("  (unknown device protocol)\n");
			break;
		}
	}
	
	probe_actual_size();
}

int main(int argc, char **argv) {
	int ce;
	
	if (argc != 2) {
		fprintf(stderr, "usage: %s devicepath filename0 filename1\n", argv[0]);
		abort();
	}
	ndf_init(argv[1]);
	
	detect(1);
	detect(2);
	
	DeppDisable(hif);
	DmgrClose(hif);
	
	return 0;
}
