// Copies a file from the file system to the current directory in Linux.
// If the specified file is not found in the root directory of the file system, output the message "File not found" and exit.
//
// Your program for part III will be invoked as follows: ./diskget disk.IMA ANS1.PDF
// ANS1.PDF should be copied to your current Linux directory, and you should be able to read the content of ANS1.PDF.

#include <stdio.h>

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		fprintf(stderr, "Usage: diskget <file system image> <file name>\n");
		return -1;
	}
	
	return 0;
}