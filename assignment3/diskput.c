// Copies a file from the current Linux directory into the root directory of the file system.
// If the specified file is not found, output the message "File not found" on a single line and exit. If the
// file system does not have enough free space to store the file, output "Not enough free space in the disk image" and exit.
//
// Your program will be invoked as follows: ./diskput disk.IMA foo.txt
// Note that a correct execution should update FAT and related allocation information in disk.IMA accordingly.
// To validate, you can use diskget implemented in Part III to check if you can correctly read foo.txt from the file system.

#include <stdio.h>

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		fprintf(stderr, "Usage: diskput <file system image> <file name>\n");
		return -1;
	}
	
	return 0;
}