#include "utils.h"

void memset(char *s, char c, int n) {
    char *p = s;
    while (n--) {
        *p++ = c;
    }
}

void memcpy(void *dest, const void *src, size_t n) {
    char *d = dest;
    const char *s = src;
    while (n--) {
        *d++ = *s++;
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

void strcpy(char *dest, const char *src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

void strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
}

unsigned int atoi(const char *str) {
    unsigned int num = 0;
    while (*str) {
        if (*str < '0' || *str > '9') {
            break;
        }
        num = num * 10 + (*str - '0');
        str++;
    }
    return num;
}

inline unsigned long be2le(unsigned int be) {
    return ((be & 0x000000FF) << 24) |
           ((be & 0x0000FF00) << 8) |
           ((be & 0x00FF0000) >> 8) |
           ((be & 0xFF000000) >> 24);
}

unsigned long hex2dec(const char *hex, int len) {
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


unsigned long ahex2int(const char *hex) {
    unsigned long dec = 0;
    while (*hex) {
        char c = *hex++;
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


int power(int base, int exp) {
    int result = 1;
    for (int i = 0; i < exp; i++) {
        result *= base;
    }
    return result;
}

