#ifndef UTILS_H
#define UTILS_H

extern void delay(unsigned long);
extern void put32(unsigned long, unsigned int);
extern unsigned int get32(unsigned long);

int strlen(const char *str);
void strcat(char *dest, const char src, int limit);
int strcmp(const char *str1, const char *str2);
void memset(char *ptr, char value, int num);

unsigned int hex2dec(const char *hex, int len);
unsigned int be2le(unsigned int be);

#endif