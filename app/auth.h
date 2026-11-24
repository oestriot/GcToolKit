#ifndef AUTH_H
#define AUTH_H 1


int do_dump_cmd56_keys(char* output_file);
int do_dump_npdrm_keys(char* output_file);

void wait_for_gc_auth();

#endif //AUTH_H