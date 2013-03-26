#ifndef DISKINFO_H_INCLUDED
#define DISKINFO_H_INCLUDED

void get_os_name(char *os_name, char *mmap);
int get_total_sectors(char *mmap);
int get_bytes_per_sector(char *mmap);
int get_sectors_per_cluster(char *mmap);
int get_total_fats(char *mmap);
int get_sectors_per_fat(char *mmap);
int get_free_size(char *mmap);
int get_total_files_in_root(char *mmap);
int get_max_root_directory_entries(char *mmap);
int get_two_byte_value(char *mmap, int offset);

#endif