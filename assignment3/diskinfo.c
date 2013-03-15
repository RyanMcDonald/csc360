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

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		fprintf(stderr, "Usage: diskinfo <file system image>\n");
		return -1;
	}
	
	char *file_system_image = argv[1];
	char *os_name = "OS Name";
	char *disk_label = "Label";
	int disk_size_total = 10;
	int disk_size_free = 5;
	int num_files_in_root = 1;
	int num_fat_copies = 2;
	int sectors_per_fat = 3;
	
	printf("File system image name: %s\n", file_system_image);
	printf("OS Name: %s\n", os_name);
	printf("Label of the disk: %s\n", disk_label);
	printf("Total size of the disk: %d\n", disk_size_total);
	printf("Free size on the disk: %d\n", disk_size_free);
	printf("==========================================\n");
	printf("Number of files in the root directory: %d\n", num_files_in_root);
	printf("==========================================\n");
	printf("Number of FAT copies: %d\n", num_fat_copies);
	printf("Sectors per fat: %d\n", sectors_per_fat);

	return 0;
}