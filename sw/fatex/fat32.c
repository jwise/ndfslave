#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fat.h"

#define FAT32_FAT0_SECTOR(h) ((h)->start + (h)->reserved_sector_count)
#define FAT32_FAT1_SECTOR(h) ((h)->start + (h)->reserved_sector_count + (h)->sectors_per_fat)
#define FAT32_ROOT_SECTOR(h) ((h)->start + (h)->reserved_sector_count + (h)->sectors_per_fat * (h)->num_fats)
#define FAT32_FIRST_CLUSTER(h) (FAT32_ROOT_SECTOR(h))
#define FAT32_CLUSTER(h, n) (FAT32_FIRST_CLUSTER(h) + (n - 2) * (h)->sectors_per_cluster)
#define FAT32_CLUSTER_IS_EOF(c) ((c) == 0xFFFFFF8 || (c) == 0xFFFFFF9 || (c) == 0xFFFFFFA || \
                                 (c) == 0xFFFFFFB || (c) == 0xFFFFFFC || (c) == 0xFFFFFFD || \
                                 (c) == 0xFFFFFFE || (c) == 0xFFFFFFF)
#define FAT32_BYTES_PER_CLUSTER(h) ((h)->sectors_per_cluster * 512)

int _read_sector(struct dumpio *io, unsigned int sector, void *buf)
{
	off_t b = ((off_t) sector) * 512LL;
	
	return 0 - (dumpio_pread(io, buf, 512, b) < 512);
}

int fat32_find_partition(struct dumpio *io)
{
	static unsigned char buf[512];
	struct ptab *table;
	int i;
	
	if (_read_sector(io, 0, buf))
	{
		puts("failed to read sector 0!");
		return -1;
	}
	
	table = (struct ptab *)&(buf[446]);
	for (i = 0; i < 4; i++)
	{
		if (table[i].type == 0x0C /* FAT32-LBA */)
		{
			return table[i].lba[0] |
			       table[i].lba[1] << 8 |
			       table[i].lba[2] << 16 |
			       table[i].lba[3] << 24;
		}
	}
	
	return -1;
}

int fat32_open(struct fat32_handle *h, struct dumpio *io, int start)
{
	static unsigned char buf[512];
	struct fat32_bootsect *bs = (struct fat32_bootsect *)buf;
	
	h->start = start;
	h->io = io;
	
	if (_read_sector(h->io, start, buf))
	{
		printf("failed to read FAT32 boot sector!\r\n");
		return -1;
	}
	
	h->bytes_per_sector = bs->bytes_per_sector[0] |
	                      (bs->bytes_per_sector[1] << 8);
	h->sectors_per_cluster = bs->sectors_per_cluster;
	h->reserved_sector_count = bs->reserved_sector_count[0] |
	                           (bs->reserved_sector_count[1] << 8);
	h->num_fats = bs->num_fats;
	h->root_cluster = bs->root_cluster[0] |
	                  (bs->root_cluster[1] << 8) |
	                  (bs->root_cluster[2] << 16) |
	                  (bs->root_cluster[3] << 24);
	h->sectors_per_fat = bs->sectors_per_fat[0] |
	                     (bs->sectors_per_fat[1] << 8) |
	                     (bs->sectors_per_fat[2] << 16) |
	                     (bs->sectors_per_fat[3] << 24);
	
	if (h->bytes_per_sector != 512)
	{
        	printf("FAT32: invalid number of bytes per sector\r\n");
        	return -1;
	}
	
	h->cache_cluster = -1;
	h->cache_data = malloc(h->sectors_per_cluster * 512);
	
	return 0;
}

int fat32_readdir(struct fat32_file *fd, struct fat32_dirent *de)
{
	union fat32_dirent_raw der;
	int has_lfn = 0;
	unsigned char lfn_checksum;
	
	while (fat32_read(fd, &der, sizeof(der)) == sizeof(der))
	{
		if (der.name[0] == 0x00 || der.name[0] == 0x05 || der.name[0] == 0xE5 || der.name[0] == '.')
		{
			has_lfn = 0;
			continue;
		}
		
		if ((der.attrib & FAT32_ATTRIB_LFN) == FAT32_ATTRIB_LFN)
		{
			int seq;
			
			seq = (der.lfn_seq & 0x3F) - 1;
			if (seq >= FAT32_NAMEENTS_MAX)
			{
				printf("FAT32: excessively large LFN sequence 0x%02x\n", der.lfn_seq);
				continue;
			}
			
			/* XXX check for oversized unicode */
			de->name[seq * 13 +  0] = der.lfn_name0[0];
			de->name[seq * 13 +  1] = der.lfn_name0[2];
			de->name[seq * 13 +  2] = der.lfn_name0[4];
			de->name[seq * 13 +  3] = der.lfn_name0[6];
			de->name[seq * 13 +  4] = der.lfn_name0[8];
			de->name[seq * 13 +  5] = der.lfn_name1[0];
			de->name[seq * 13 +  6] = der.lfn_name1[2];
			de->name[seq * 13 +  7] = der.lfn_name1[4];
			de->name[seq * 13 +  8] = der.lfn_name1[6];
			de->name[seq * 13 +  9] = der.lfn_name1[8];
			de->name[seq * 13 + 10] = der.lfn_name1[10];
			de->name[seq * 13 + 11] = der.lfn_name2[0];
			de->name[seq * 13 + 12] = der.lfn_name2[2];
			
			if (der.lfn_seq & 0x40)
				de->name[seq * 13 + 13] = 0;
			
			has_lfn = 1;
			lfn_checksum = der.lfn_checksum;
			continue;
		}
		
		if (has_lfn)
		{
			/* Verify the LFN checksum. */
			unsigned char sum = 0;
			int i;
			
			for (i = 0; i < 11; i++)
				sum = ((sum & 1) << 7) + (sum >> 1) + der.name[i];
			
			if (sum != lfn_checksum)
			{
				printf("FAT32: LFN checksum mismatch (expected %02x, got %02x)\n", lfn_checksum, sum);
				has_lfn = 0;
			}
		}
		
		if (!has_lfn)
		{
			int i;
			
			for (i = 0; i < 8; i++)
			{
				if (der.name[i] == ' ')
					break;
				de->name[i] = der.name[i];
			}
			if (der.ext[0] != ' ')
			{
				de->name[i++] = '.';
				de->name[i++] = der.ext[0];
				de->name[i++] = der.ext[1];
				de->name[i++] = der.ext[2];
			}
			de->name[i++] = 0;
		}
		
		de->attrib = der.attrib;
		de->size = der.size[0] | ((unsigned int)der.size[1] << 8) |
		           ((unsigned int)der.size[2] << 16) | ((unsigned int)der.size[3] << 24);
		if (der.attrib & FAT32_ATTRIB_DIRECTORY)
			de->size = 0xFFFFFFFF;
		de->cluster = der.clust_lo[0] | ((unsigned int)der.clust_lo[1] << 8) |
	                      ((unsigned int)der.clust_hi[0] << 16) | ((unsigned int)der.clust_hi[1] << 24);
		return 0;
	}
	
	return -1;
}

int fat32_get_next_cluster(struct fat32_handle *h, int cluster)
{
	static unsigned int buf[128 /* 512 / 4 */];
	
	if (_read_sector(h->io, FAT32_FAT0_SECTOR(h) + cluster / 128, (unsigned int *) buf))
	{
		puts("failed to read FAT!");
		return 0xFFFFFF7;	/* bad sector */
	}
	
	return buf[cluster % 128] & 0xFFFFFFF;
}

void fat32_open_root(struct fat32_handle *h, struct fat32_file *fd)
{
	fd->h = h;
	fd->pos = 0;
	fd->start_cluster = h->root_cluster;
	fd->cluster = fd->start_cluster;
	fd->len = 0xFFFFFFFF;
}

void fat32_open_by_de(struct fat32_handle *h, struct fat32_file *fd, struct fat32_dirent *de)
{
	fd->h = h;
	fd->pos = 0;
	fd->start_cluster = de->cluster;
	fd->cluster = fd->start_cluster;
	fd->len = de->size;
}

#if 0
struct _open_by_name_priv {
	unsigned char fnameext[12];
	struct fat32_file *fd;
	int found;
};

static void _open_by_name_callback(struct fat32_handle *h, struct fat32_dirent *de, void *_priv)
{
	struct _open_by_name_priv *priv = _priv;
	
       	if (de->name[0] == 0x00 || de->name[0] == 0x05 || de->name[0] == 0xE5 || (de->attrib & 0x48))
       		return;
	
	if (!memcmp(priv->fnameext, de->name, 11))
	{
		fat32_open_by_de(h, priv->fd, de);
		priv->found = 1;
	}
}

int fat32_open_by_name(struct fat32_handle *h, struct fat32_file *fd, char *name)
{
	struct _open_by_name_priv priv;
	
	memcpy(priv.fnameext, name, 11);	/* XXX */
	priv.fnameext[11] = 0;
	priv.found = 0;
	priv.fd = fd;
	
	fat32_ls_root(h, _open_by_name_callback, (void *)&priv);
	
	return priv.found ? 0 : -1;
}
#endif



int fat32_read(struct fat32_file *fd, void *buf, int len)
{
	int retlen = 0;

#ifdef DEBUG
	printf("FAT32: called with len: %d\r\n", len);
#endif

	len = (len > (fd->len - fd->pos)) ? (fd->len - fd->pos) : len;

#ifdef DEBUG
	printf("FAT32: length to read: %d\r\n", len);
#endif
	
	while (len && !FAT32_CLUSTER_IS_EOF(fd->cluster))
	{
		int cluslen;
		int rem = FAT32_BYTES_PER_CLUSTER(fd->h) - fd->pos % FAT32_BYTES_PER_CLUSTER(fd->h);
#ifdef DEBUG
		printf("FAT32: bytes remaining: %d\r\n", len);
		printf("FAT32: reading from cluster %d\r\n", fd->cluster);
#endif
		
		cluslen = (rem > len) ? len : rem;
		
		if (fd->h->cache_cluster != fd->cluster)
		{
			if (dumpio_pread(fd->h->io, fd->h->cache_data, FAT32_BYTES_PER_CLUSTER(fd->h), FAT32_CLUSTER(fd->h, fd->cluster) * 512LL) < FAT32_BYTES_PER_CLUSTER(fd->h))
				return -1;
			fd->h->cache_cluster = fd->cluster;
		}
		memcpy(buf, fd->h->cache_data + fd->pos % FAT32_BYTES_PER_CLUSTER(fd->h), cluslen);
		
		retlen += cluslen;
		buf += cluslen;
		fd->pos += cluslen;
		len -= cluslen;
		
		if ((fd->pos % FAT32_BYTES_PER_CLUSTER(fd->h)) == 0)
			fd->cluster = fat32_get_next_cluster(fd->h, fd->cluster);
	}
#ifdef DEBUG
	printf("FAT32: done! found eof? %d\r\n", FAT32_CLUSTER_IS_EOF(fd->cluster));
	printf("FAT32: cluster: %x\r\n", fd->cluster);
	printf("FAT32: final bytes remaining: %d\r\n", len);
#endif
	
	return retlen;
}
