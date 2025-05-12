#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

extern void delay(unsigned long);
extern void put32(unsigned long, unsigned int);
extern unsigned int get32(unsigned long);

int strlen(const char *str);
void strcat(char *dest, const char src, int limit);
int strcmp(const char *str1, const char *str2);
void memset(char *ptr, char value, int num);
void memcpy(void *dest, const void *src, size_t num);

unsigned int atoi(const char *str);
unsigned long be2le(unsigned int be);
unsigned long hex2dec(const char *hex, int len);
unsigned long ahex2int(const char *hex);

int power(int base, int exp);

#endif