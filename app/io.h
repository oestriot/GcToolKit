#ifndef IO_H
#define IO_H 1

#include <GcToolKit.h>

typedef struct SearchFilter {
	uint64_t max_filesize;
	uint8_t file_only;
	char match_extension[MAX_PATH];
} SearchFilter;

int file_exist(const char* path);
int write_file(const char* path, const void* data, size_t size);
int read_file(const char* path, void* data, size_t size);
int copy_file(const char* path, const char* new_path);
int extract_dirname(const char* path, char* dir_name, size_t dir_name_length);
void make_directories(const char* path);
void make_directories_excluding_last(const char* filepath);
void change_extension(char* path, size_t path_length, const char* new_extension);

void remove_illegal_chars(char* str);

int wait_for_partition(char* partiton);
int mount_uma();
int mount_xmc();
int mount_imc();
int mount_ux0();

int mount_gro0();
int mount_grw0();

void umount_gro0();
void umount_grw0();

void mount_devices();
void umount_uma();
void umount_xmc();
void umount_imc();
void umount_ux0();
void umount_devices();

uint64_t get_file_size(const char* filepath);
uint64_t get_free_space(const char* device);
int read_first_filename(char* path, char* output, size_t out_size);

int get_files_in_folder(char* folder, char* out_filenames, int* total_folders, SearchFilter* filter, size_t max_files);


#endif