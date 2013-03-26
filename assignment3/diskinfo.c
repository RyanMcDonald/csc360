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
	char *os_name = malloc(sizeof(char)*8);
	char *disk_label = "";
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
		
		disk_size_total = get_total_sectors(map) * get_bytes_per_sector(map);
		num_fat_copies = get_total_fats(map);
		sectors_per_fat = get_sectors_per_fat(map);
		//disk_size_free = get_free_size(map);
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

int get_free_size(char *mmap) {
	// Loop through fat table entries
	// Read attribute of each file. Fat entry value 0x00 = Unused.
	int retVal = 0;
	return retVal;
}

int get_total_files_in_root(char *mmap) {
	int files = 0;
	
	int i;
	int bytes_per_sector = get_bytes_per_sector(mmap);
	printf("Starting offset: %d\n", 19 * bytes_per_sector);
	printf("Increment: %d\n", get_sectors_per_cluster(mmap) * bytes_per_sector);
	printf("Max root directory entries: %d\n", get_max_root_directory_entries(mmap));
	printf("Halt condition: %d\n", get_max_root_directory_entries(mmap) * bytes_per_sector);
	for (i = 19 * bytes_per_sector; i < get_max_root_directory_entries(mmap) * bytes_per_sector; i + (get_sectors_per_cluster(mmap) * bytes_per_sector)) {
		printf("Value of file: %d\n", mmap[i]);
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
	* tmp1 = mmap[offset];
	* tmp2 = mmap[offset + 1];
	
	// Switch to Big Endian format
	retVal = *tmp1 + ((*tmp2) << 8);
	
	free(tmp1);
	free(tmp2);
	
	return retVal;
}