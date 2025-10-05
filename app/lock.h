#ifndef LOCK_H
#define LOCK_H 1

void unlock_power();
int lock_power();

int lock_gc();
void unlock_gc();

void lock_shell();
void unlock_shell();

void init_shell();
void term_shell();

#endif