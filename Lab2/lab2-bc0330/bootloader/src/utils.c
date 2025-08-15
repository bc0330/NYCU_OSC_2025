#include "utils.h"

void memset(char *s, char c, int n) {
    char *p = s;
    while (n--) {
        *p++ = c;
    }
}

int strlen(const char *str) {
    int len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

void strcat(char *dest, const char src, int limit) {
    int dest_len = strlen(dest);
    if (dest_len >= limit - 1) {
        return;
    }

    dest[dest_len] = src;
    dest[dest_len + 1] = '\0';
}

int strcmp(const char *str1, const char *str2) {
    while (*str1 && *str2) {
        if (*str1 != *str2) {
            return 0;
        }
        str1++;
        str2++;
    }
    return *str1 == *str2;
}

