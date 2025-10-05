#ifndef IME_H
#define IME_H 1

int open_ime_short(char* title, unsigned short* number);
int open_ime(char* title, char* text, int max_len);

#endif