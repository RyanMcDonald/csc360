#ifndef DISKLIST_H_INCLUDED
#define DISKLIST_H_INCLUDED

int get_bytes_per_sector(char *mmap);
int get_number_files_in_root(char *mmap);
void get_files_in_root(char *mmap, int num_files_in_root);
void get_file_type(char *mmap, char *file_type, int offset);
void get_file_name(char *mmap, char *file_name, int offset);
int get_file_size(char *mmap, int offset);
int get_two_byte_value(char *mmap, int offset);
int get_four_byte_value(char *mmap, int offset);

#endif