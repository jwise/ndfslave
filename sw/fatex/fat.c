#define _LARGEFILE64_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include "dumpio.h"
#include "fat.h"

static struct dumpio *_io;

enum {
	FL_FILE = 1,
	FL_LIST = 2,
	FL_EXTRACT = 4,
	FL_VERBOSE = 8,
	FL_CHDIR = 16,
};

#define SHIFT do { argv[1] = argv[0]; argc--; argv++; } while(0)
#define XFAIL(p) do { if (p) { fprintf(stderr, "%s (%s:%d): operation failed: %s\n", __FUNCTION__, __FILE__, __LINE__, #p); abort(); } } while(0)

int flags = 0;

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s mode ...\n", argv[0]);
		return 1;
	}
	
	while (*argv[1]) {
		switch(*argv[1]) {
		case 't':
			flags |= FL_LIST;
			break;
		case 'x':
			flags |= FL_EXTRACT;
			break;
		case '-':
			break;
		case 'f':
			flags |= FL_FILE;
			break;
		case 'v':
			flags |= FL_VERBOSE;
			break;
		default:
			fprintf(stderr, "%s: unknown flag '%c'\n", argv[0], *argv[1]);
			break;
		}
		argv[1]++;
	}
	SHIFT;
	
	if (!FL_FILE) {
		fprintf(stderr, "%s: 'f' required flag\n", argv[0]);
		return 1;
	}
	
	if (argc < 2) {
		fprintf(stderr, "%s: no file arg?\n", argv[0]);
		return 1;
	}
	
	XFAIL((_io = dumpio_init(argv[1])) == NULL);
	SHIFT;
	
	if (argc > 1 && argv[1][0] == '-') {
		argv[1]++;
		while (*argv[1]) {
			switch (*argv[1]) {
			case 'C':
				flags |= FL_CHDIR;
				break;
			default:
				fprintf(stderr, "%s: unknown flag '%c'\n", argv[0], *argv[1]);
				break;
			}
			argv[1]++;
		}
		SHIFT;
	}
	if (flags & FL_CHDIR) {
		if (argc < 2) {
			fprintf(stderr, "%s: no dir arg?\n", argv[0]);
			return 1;
		}
		if (chdir(argv[1]) < 0) {
			fprintf(stderr, "%s: failed to chdir(%s): %s\n", argv[0], argv[1], strerror(errno));
			return 2;
		}
		SHIFT;
	}
	
	/* off_t dumpio_pread(io, buf, sz, off) */
	/* off_t dumpio_size */
	
	struct fat32_handle h;
	int p = fat32_find_partition(_io);
	XFAIL(p == -1);
	
	XFAIL(fat32_open(&h, _io, p));
	
	struct fat32_file fd;
	char **paths = NULL;
	int npaths = 0;
	
	fat32_open_root(&h, &fd);
	
	void recurs(struct fat32_handle *h, struct fat32_file *fd, char ***paths, int *npaths, int lvl);
	recurs(&h, &fd, &paths, &npaths, 0);
	
	return 0;
}

void recurs(struct fat32_handle *h, struct fat32_file *fd, char ***paths, int *npaths, int lvl) {
	struct fat32_file fd2;
	struct fat32_dirent de;
	
	while (fat32_readdir(fd, &de) == 0) {
		int i;
		
		if (flags & FL_VERBOSE)
			printf("%crwx\t@%08x\tsz %10d\t", de.attrib & FAT32_ATTRIB_DIRECTORY ? 'd' : '-', de.cluster, de.size == 0xffffffff ? 0 : de.size);
		for (i = 0; i < lvl; i++)
			printf("%s/", (*paths)[i]);
		printf("%s%s\n", de.name, de.attrib & FAT32_ATTRIB_DIRECTORY ? "/" : "");
			
		if (de.attrib & FAT32_ATTRIB_DIRECTORY) {
			fat32_open_by_de(h, &fd2, &de);
			if (lvl+1 > *npaths) {
				*paths = realloc(*paths, (*npaths+1) * 2 * sizeof (char **));
				*npaths = (*npaths+1) * 2;
			}
			(*paths)[lvl] = de.name;
			
			if (flags & FL_EXTRACT) {
				mkdir(de.name, 0755);
				if (chdir(de.name) < 0) {
					perror("chdir");
					continue;
				}
			}
			
			recurs(h, &fd2, paths, npaths, lvl+1);
			
			if (flags & FL_EXTRACT)
				chdir("..");
		} else if (flags & FL_EXTRACT) {
			char buf[8192];
			int rfd;
			
			fat32_open_by_de(h, &fd2, &de);
			rfd = creat(de.name, 0644);
			if (rfd < 0) {
				perror("creat");
				continue;
			}
			/* :opens_boxes -3
			   :fu -100 */
			while (fd2.pos != fd2.len) {
				int thislen = (8192 > (fd2.len - fd2.pos)) ? fd2.len - fd2.pos : 8192;
				
				if (fat32_read(&fd2, buf, thislen) < thislen) {
					fprintf(stderr, "EXTRACT ERROR: short read: %d bytes read out of %d\n", fd2.pos, fd2.len); 
					break;
				}
				write(rfd, buf, thislen);
			}
			close(rfd);
		}
	}
}
