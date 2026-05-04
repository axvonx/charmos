#pragma once
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memchr(const void *s, int c, size_t n);
void *memrchr(const void *s, int c, size_t n);

size_t strlen(const char *str);
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);
int strncmp(const char *s1, const char *s2, size_t n);
int strcmp(const char *str1, const char *str2);
char *strncpy(char *dest, const char *src, size_t n);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char *strpbrk(const char *s, const char *accept);
char *strstr(const char *haystack, const char *needle);

char *strtok(char *str, const char *delim);
char *strtok_r(char *str, const char *delim, char **saveptr);

char *strncat(char *dest, const char *src, size_t n);
size_t strnlen(const char *s, size_t maxlen);

int islower(int c);
int isupper(int c);
int isdigit(int c);
int isalpha(int c);
int isalnum(int c);
int isspace(int c);
int isprint(int c);
int toupper(int c);
int tolower(int c);

char *strdup(const char *str);
char *strndup(const char *str, size_t n);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);

char *strrev(char *s);
char *strtoupper(char *s);
char *strtolower(char *s);

int64_t atoi(const char *str);
size_t atoui(const char *str);
size_t atohex(const char *str);

char *itoa(int64_t value, char *buf, int base);
char *utoa(size_t value, char *buf, int base);

long strtol(const char *nptr, char **endptr, int base);

int vsnprintf(char *buffer, int buffer_len, const char *format, va_list args);
int snprintf(char *buffer, int buffer_len, const char *format, ...);
