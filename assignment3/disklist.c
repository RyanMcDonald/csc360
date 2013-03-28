// Program displays the contents of the root directory in the file system.
// Your program for part II will be invoked as follows: ./disklist disk.IMA
//
// The directory listing should be formatted as follows:
// 1. The first column will contain:
// 		(a) F for regular files, or
// 		(b) D for directories;
// 		followed by a single space
// 2. then 10 characters to show the file size in bytes, followed by a single space
// 3. then 20 characters for the file name, followed by a single space
// 4. then the file creation date and creation time.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>  // mmap
#include <fcntl.h>  // open
#include <sys/stat.h> // fstat
#include <string.h> // strcat
#include <ctype.h> // isspace
#include "disklist.h"

typedef struct {
	char *file_name;
	int file_size;
	char *file_type;
	
} file_struct;

typedef file_struct * file_struct_pointer;

file_struct_pointer *root_files;
	
int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		fprintf(stderr, "Usage: disklist <file system image>\n");
		return -1;
	}
	
	char *file_system_image = argv[1];
	
	int fd;
	struct stat file_stats;
	char *map;
	
	if ((fd = open(file_system_image, O_RDONLY))) {
		// Return information about the file and store it in file_stats
		fstat(fd, &file_stats);
		
		// void * mmap(void * addr, size_t length, int prot, int flags, int fd, off_t offset);
		map = mmap(NULL, file_stats.st_size, PROT_READ, MAP_SHARED, fd, 0);
		
		// get the total number of files in the root directory
		int number_files_in_root = get_number_files_in_root(map);
		
		root_files = malloc(number_files_in_root * sizeof(file_struct_pointer));
		
		get_files_in_root(map, number_files_in_root);
		
		int i;
		for (i = 0; i < number_files_in_root; i ++) {
			printf("%1s %20s\n", root_files[i]->file_type, root_files[i]->file_name);
		}
		
	} else {
		perror("Error opening file for reading");
		exit(EXIT_FAILURE);
	}
	return 0;
}

int get_bytes_per_sector(char *mmap) {
	return get_two_byte_value(mmap, 11);
}

int get_number_files_in_root(char *mmap) {
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

void get_files_in_root(char *mmap, int num_files_in_root) {
	int bytes_per_sector = get_bytes_per_sector(mmap);
	int i;
	int index = 0;
	for (i = 19; i <= 32; i ++) {
		// Directory entries are 32 bytes long
		int j = 0;
		for (j = 0; j < 16; j++) {
			int offset = (i * bytes_per_sector) + (j * 32);
			int attributeValue = mmap[offset + 11];
			
			// If the first byte of the Filename field is 0x00, then this directory entry is free and all the
			// remaining directory entries in this directory are also free.
			if (mmap[(i * bytes_per_sector) + (j * 32)] == 0x00) {
				return;
			}
			
			if ((attributeValue & 0x0F) != 0x0F && (attributeValue & 0x08) != 0x08 && (attributeValue & 0x10) != 0x10) {
				root_files[index] = malloc(sizeof(file_struct));
				root_files[index]->file_name = malloc(sizeof(char) * 11);
				root_files[index]->file_type = malloc(sizeof(char));
				get_file_name(mmap, root_files[index]->file_name, offset);
				get_file_type(mmap, root_files[index]->file_type, offset);
				
				index ++;
			}
		}
	}
}

void get_file_name(char *mmap, char *file_name, int offset) {
	int i;
	char *temp_file_name = malloc(sizeof(char) * 8);
	char *temp_file_extension = malloc(sizeof(char) * 3);
	for(i = 0; i < 8; i ++) {
		if (!isspace(mmap[offset + i])) {
			temp_file_name[i] = mmap[offset + i];
		} else {
			//temp_file_name[i] = '\0';
			break;
		}
	}
	
	int j;
	for (j = 0; j < 3; j ++) {
		if (!isspace(mmap[offset + 8 + j])) {
			temp_file_extension[j] = mmap[offset + 8 + j];
		} else {
			//temp_file_extension[j] = '\0';
			break;
		}
	}
	
	//file_name = malloc(sizeof(char) * 11);
	strcpy(file_name, temp_file_name);
	strcat(file_name, ".");
	strcat(file_name, temp_file_extension);
	free(temp_file_name);
	free(temp_file_extension);
}

void get_file_type(char *mmap, char *file_type, int offset) {
	int attributeValue = mmap[offset + 11];
	
	//file_type = malloc(sizeof(char));
	file_type[0] = 'F';
	if (attributeValue == 0x10) {
		file_type[0] = 'D';
	}
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