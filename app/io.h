#ifndef IO_H
#define IO_H 1

#define MAX_PATH (512)

typedef struct SearchFilter {
	uint64_t max_filesize;
	uint8_t file_only;
	char match_extension[MAX_PATH];
} SearchFilter;

int file_exist(char* path);
int write_file(const char* path, const void* data, size_t size);
int read_file(const char* path, void* data, size_t size);
int copy_file(const char* path, const char* new_path);

void remove_illegal_chars(char* str);

int wait_for_partition(char* partiton);
int mount_uma();
int mount_xmc();
int mount_imc();

int mount_gro0();
int mount_grw0();

void umount_gro0();
void umount_grw0();

void mount_devices();
void umount_uma();
void umount_xmc();
void umount_imc();
void umount_devices();

uint64_t get_file_size(const char* filepath);
uint64_t get_free_space(const char* device);
int read_first_filename(char* path, char* output, size_t out_size);

int get_files_in_folder(char* folder, char* out_filenames, int* total_folders, SearchFilter* filter, size_t max_files);
void change_extension(char* path, const char* new_extension);


#endif