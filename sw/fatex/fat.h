#ifndef FAT_H
#define FAT_H

#include "dumpio.h"

struct ptab {
	unsigned char flags;
	unsigned char sh, scl, sch;
	unsigned char type;
	unsigned char eh, ecl, ech;
	unsigned char lba[4];
	unsigned char size[4];
};

struct fat32_handle {
	struct dumpio *io;
	
	unsigned int start;
	
	unsigned int bytes_per_sector;
	unsigned int sectors_per_cluster;
	unsigned int reserved_sector_count;
	unsigned int num_fats;
	unsigned int root_cluster;
	unsigned int sectors_per_fat;
};

struct fat32_bootsect {
	unsigned char jmp[3];
	unsigned char oemname[8];

	/* EBPB32 */
	/*   DOS 3.31 BPB */
	/*     DOS 2.0 BPB */
	unsigned char bytes_per_sector[2];
	unsigned char sectors_per_cluster;
	unsigned char reserved_sector_count[2];
	unsigned char num_fats;
	unsigned char max_root_dirents[2]; /* 0 on FAT32 */
	unsigned char total_sectors_old[2];
	unsigned char media_descriptor;
	unsigned char sectors_per_fat_old[2]; /* 0 on FAT32 */
	/*     End DOS 2.0 BPB */
	unsigned char sectors_per_track[2];
	unsigned char number_of_heads[2];
	unsigned char hidden_sector_count[4];
	unsigned char total_sectors_new[4];
	/*   End DOS 3.31 BPB */
	unsigned char sectors_per_fat[4];
	unsigned char drive_flags[2];
	unsigned char version[2];
	unsigned char root_cluster[4];
	unsigned char fs_info_sector[2];
	unsigned char backup_bootsect[2];
	unsigned char reserved[12];
	unsigned char drive_number;
	unsigned char signature; /* 0x29 */
	unsigned char volid[4];
	unsigned char label[11];
	unsigned char fstype[8];
};

#define FAT32_ATTRIB_LFN 0x0F
#define FAT32_ATTRIB_DIRECTORY 0x10

struct fat32_dirent {
	unsigned char name[8];
	unsigned char ext[3];
	unsigned char attrib;
	unsigned char reserved;
	unsigned char ctime[3];
	unsigned char cdate[2];
	unsigned char adate[2];
	unsigned char clust_hi[2];
	unsigned char mtime[2];
	unsigned char mdate[2];
	unsigned char clust_lo[2];
	unsigned char size[4];
};

struct fat32_file {
	struct fat32_handle *h;
	unsigned int pos;
	unsigned int start_cluster;
	unsigned int cluster;
	unsigned int len;
};

extern int fat32_find_partition(struct dumpio *dump);
extern int fat32_open(struct fat32_handle *h, struct dumpio *dump, int start);
extern void fat32_open_root(struct fat32_handle *h, struct fat32_file *fd);
extern void fat32_ls_root(struct fat32_handle *h, void (*cbfn)(struct fat32_handle *, struct fat32_dirent *, void *), void *priv);
extern int fat32_get_next_cluster(struct fat32_handle *h, int cluster);
extern void fat32_open_by_de(struct fat32_handle *h, struct fat32_file *fd, struct fat32_dirent *de);
extern int fat32_open_by_name(struct fat32_handle *h, struct fat32_file *fd, char *name);
extern int fat32_read(struct fat32_file *fd, void *buf, int len);

#endif
