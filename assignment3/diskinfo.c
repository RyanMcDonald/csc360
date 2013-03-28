// Your program for part I will be invoked as follows: ./diskinfo disk.IMA
// Output should include the following information:
// OS Name:
// Label of the disk:
// Total size of the disk:
// Free size of the disk:
// ==============
// The number of files in the root directory (not including subdirectories):
// =============
// Number of FAT copies:
// Sectors per FAT:

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>  // mmap
#include <fcntl.h>  // open
#include <sys/stat.h> // fstat
#include "diskinfo.h"

int main(int argc, char *argv[]) {
	if(argc != 2)
	{
		fprintf(stderr, "Usage: diskinfo <file system image>\n");
		return -1;
	}
	
	char *file_system_image = argv[1];
	char *os_name = malloc(sizeof(char) * 8);
	char *disk_label = malloc(sizeof(char) * 11);
	int disk_size_total = 0;
	int disk_size_free = 0;
	int num_files_in_root = 0;
	int num_fat_copies = 0;
	int sectors_per_fat = 0;
	
	int fd;
	struct stat file_stats;
	char *map;
	
	if ((fd = open(file_system_image, O_RDONLY))) {
		// Return information about the file and store it in file_stats
		fstat(fd, &file_stats);

		// void * mmap(void * addr, size_t length, int prot, int flags, int fd, off_t offset);
		map = mmap(NULL, file_stats.st_size, PROT_READ, MAP_SHARED, fd, 0);
		
		get_os_name(os_name, map);
		get_disk_label(disk_label, map);
		disk_size_total = get_total_size(map);
		num_fat_copies = get_total_fats(map);
		sectors_per_fat = get_sectors_per_fat(map);
		disk_size_free = get_free_size(map);
		num_files_in_root = get_total_files_in_root(map);
	} else {
		perror("Error opening file for reading");
		exit(EXIT_FAILURE);
	}
	
	printf("OS Name: %s\n", os_name);
	printf("Label of the disk: %s\n", disk_label);
	printf("Total size of the disk: %d\n", disk_size_total);
	printf("Free size on the disk: %d\n", disk_size_free);
	printf("==========================================\n");
	printf("Number of files in the root directory: %d\n", num_files_in_root);
	printf("==========================================\n");
	printf("Number of FAT copies: %d\n", num_fat_copies);
	printf("Sectors per FAT: %d\n", sectors_per_fat);
	
	free(os_name);
	close(fd);
	return 0;
}

void get_os_name(char *os_name, char *mmap) {	
	int i;
	for(i = 0; i < 8; i++) {
		os_name[i] = mmap[3+i];
	}
}

void get_disk_label(char *disk_label, char *mmap) {
	int bytes_per_sector = get_bytes_per_sector(mmap);
	int i;
	// Directory entries are 32 bytes long
	for (i = 19; i <= 32; i ++) {
		int j = 0;
		for (j = 0; j < 16; j++) {
			int attributeValue = mmap[(i * bytes_per_sector) + (j * 32) + 11];
			if ((attributeValue & 0x08) == 0x08 && (attributeValue & 0x0F) != 0x0F) {
				int k;
				for(k = 0; k < 11; k++) {
					disk_label[k] = mmap[(i * bytes_per_sector) + (j * 32) + k];
				}
				
				return;
			}
		}
	}
	
}

int get_total_sectors(char *mmap) {
	return get_two_byte_value(mmap, 19);
}

int get_bytes_per_sector(char *mmap) {
	return get_two_byte_value(mmap, 11);
}

int get_sectors_per_cluster(char *mmap) {
	return mmap[13];
}

int get_total_fats(char *mmap) {
	// Number of FATs starts at byte 16 of the boot sector and is 1 byte in length
	return mmap[16];
}

int get_sectors_per_fat(char *mmap) {
	return get_two_byte_value(mmap, 22);
}

int get_total_size(char *mmap) {
	return get_total_sectors(mmap) * get_bytes_per_sector(mmap);
}

int get_free_size(char *mmap) {
	// Logical index of data area is 2-2848
	// Physical index of data area is 33-2879
	// Count the number of sectors in use and subtract that amount from the total sectors to get free space
	int bytes_per_sector = get_bytes_per_sector(mmap);
	int free_sectors = 0;
	int i;
	for (i = 2; i <= 2848; i ++) {
		// Directory entries are 32 bytes long
		int *tmp1 = malloc(sizeof(int));
		int *tmp2 = malloc(sizeof(int));
		int result = 0;
		
		// If the logical number is even a + b << 8
		if (i % 2 == 0) {
			*tmp1 = (unsigned char) mmap[512 + (3*i)/2];
			*tmp2 = (unsigned char) mmap[513 + (3*i)/2];
			*tmp2 = *tmp2 & 0x0F; // get the low 4 bits
			
			result = *tmp1 + (*tmp2 << 8);
			
		
		// If the logical number is odd a >> 4 + b << 4
		} else {
			*tmp1 = (unsigned char) mmap[512 + (3*i)/2];
			*tmp1 = *tmp1 & 0xF0; // get the high 4 bits
			*tmp2 = (unsigned char) mmap[513 + (3*i)/2];
			
			result = (*tmp1 >> 4) + (*tmp2 << 4);
		}
		
		// If the value is 0x00, that sector is free
		if (result == 0x00) {
			free_sectors ++;
		}
	}

	printf("Free sectors: %d\n", free_sectors);
	return free_sectors * bytes_per_sector;
}

int get_total_files_in_root(char *mmap) {
	int bytes_per_sector = get_bytes_per_sector(mmap);
	int files = 0;
	int i;
	for (i = 19; i <= 32; i ++) {
		// Directory entries are 32 bytes long
		int j = 0;
		for (j = 0; j < 16; j++) {
			int attributeValue = mmap[(i * bytes_per_sector) + (j * 32) + 11];
			
			// If the first byte of the Filename field is 0x00, then this directory entry is free and all the
			// remaining directory entries in this directory are also free.
			if (mmap[(i * bytes_per_sector) + (j * 32)] == 0x00) {
				return files;
			}
			
			if ((attributeValue & 0x0F) != 0x0F && (attributeValue & 0x08) != 0x08 && (attributeValue & 0x10) != 0x10) {
				files ++;
			}
		}
	}
	
	return files;
}

int get_max_root_directory_entries(char *mmap) {
	return get_two_byte_value(mmap, 17);
}

int get_two_byte_value(char *mmap, int offset) {
	int *tmp1 = malloc(sizeof(int));
	int *tmp2 = malloc(sizeof(int));
	int retVal;
	
	// Beginning of data area starts at byte 33 of the boot sector and is 2 bytes in length
	* tmp1 = (unsigned char) mmap[offset];
	* tmp2 = (unsigned char) mmap[offset + 1];
	
	// Switch to Big Endian format
	retVal = *tmp1 + ((*tmp2) << 8);
	
	free(tmp1);
	free(tmp2);
	
	return retVal;
}