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

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		fprintf(stderr, "Usage: disklist <file system image>\n");
		return -1;
	}
	
	return 0;
}