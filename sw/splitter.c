#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/time.h>

#define XFAIL(p) do { if (p) { fprintf(stderr, "%s (%s:%d): operation failed: %s\n", __FUNCTION__, __FILE__, __LINE__, #p); abort(); } } while(0)

#define BSZ (8192+640)

int main(int argc, char **argv) {
	int fdi, fd0, fd1;
	int rv;
	char buf[BSZ];
	if (argc != 4) {
		fprintf(stderr, "usage: %s inp oup0 oup1\n", argv[0]);
		abort();
	}
	
	XFAIL((fdi = open(argv[1], O_RDONLY)) < 0);
	XFAIL((fd0 = creat(argv[2], 0644)) < 0);
	XFAIL((fd1 = creat(argv[3], 0644)) < 0);
	
	int cs = 0;
	while ((rv = read(fdi, buf, BSZ)) == BSZ) {
		write(cs?fd1:fd0, buf, BSZ);
		cs = !cs;
	}
	
	printf("final read: %d/%d\n", rv, BSZ);
}