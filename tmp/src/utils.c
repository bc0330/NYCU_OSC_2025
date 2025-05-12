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

unsigned int hex2dec(const char *hex, int len) {
    unsigned int dec = 0;
    for (int i = 0; i < len; i++) {
        char c = hex[i];
        if (c >= '0' && c <= '9') {
            dec = dec * 16 + c - '0';
        } else if (c >= 'a' && c <= 'f') {
            dec = dec * 16 + c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            dec = dec * 16 + c - 'A' + 10;
        }
    }
    return dec;
}

inline unsigned int be2le(unsigned int be) {
    return ((be & 0x000000FF) << 24) |
           ((be & 0x0000FF00) << 8) |
           ((be & 0x00FF0000) >> 8) |
           ((be & 0xFF000000) >> 24);
}