#ifndef __USER_PROG_H
#define __USER_PROG_H

void jump_user_prog(void *entry, void *user_stack);
void exec_prog(char *filename);
void dummy_prog(void);

#endif