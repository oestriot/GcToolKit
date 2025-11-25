#ifndef AUTH_H
#define AUTH_H 1


int key_dump_network(char* ip_address, unsigned short port, char* output_file);
int key_dump(char* output_file);

void wait_for_gc_auth();

#endif //AUTH_H