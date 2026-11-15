#ifndef CTRL_H
#define CTRL_H 1

extern unsigned int SCE_CTRL_CONFIRM;
extern unsigned int SCE_CTRL_CANCEL;

int init_ctrl();
int get_key();
unsigned int get_key_noblock();

#endif //CTRL_H