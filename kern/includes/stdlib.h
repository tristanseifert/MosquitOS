#ifndef STD_H
#define STD_H

#include <stdarg.h>

// string functions: strings
size_t strlen(char* str);
char* strtok(char *s, const char *delim);
char *strchr(const char *s, int c);
char* strsep(char **stringp, const char *delim);
int strcmp(const char * s1, const char * s2);
int strncmp(const char * s1, const char * s2, size_t n);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);

long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
int atoi(const char *str);

// string functions: memory
void* memchr(void* ptr, uint8_t value, size_t num);
int memcmp(const void* ptr1, const void* ptr2, size_t num);
void* memcpy(void* destination, void* source, size_t num);
void *memset(void* ptr, uint8_t value, size_t num);
void *memmove(void *dest, const void *src, size_t n);

// string functions: miscellaneous
int isalnum(int c);
int isalpha(int c);
int isblank(int c);
int iscntrl(int c);
int isdigit(int c);
int isgraph(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
int tolower(int c);
int toupper(int c);

#define isalnum(c) (isalpha(c) || isdigit(c))
#define isalpha(c) (islower(c) || isupper(c))
#define isblank(c) ((c) == ' ' || (c) == '\t')
#define iscntrl(c) ((c) >= 0x0 && (c) <= 0x8)
#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isgraph(c) (ispunct(c) || isalnum(c))
#define islower(c) ((c) >= 'a' && (c) <= 'z')
#define isprint(c) (isgraph(c) || isspace(c))
#define ispunct(c) (((c) >= 0x21 && (c) <= 0x2F) || ((c) >= 0x3A && (c) <= 0x40)\
					|| ((c) >= 0x5B && (c) <= 0x60) || ((c) >= 0x7B && (c) <= 0x7E))
#define isspace(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n' ||\
					(c) == '\f' || (c) == '\v')
#define isupper(c) ((c) >= 'A' && (c) <= 'Z')
#define isxdigit(c) (isdigit(c) || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))
#define tolower(c) (isupper(c) ? ((c) + 'a' - 'A') : (c))
#define toupper(c) (islower(c) ? ((c) + 'A' - 'a') : (c))

// standard library: memory
void* malloc(size_t num_bytes);
void* memclr(void* start, size_t count);

// standard library: input/output
int snprintf(char* str, size_t size, const char* format, ...);
int sprintf(char* str, const char* format, ...);
int vsprintf(char* str, const char* format, va_list ap);
int vsnprintf(char* str, size_t size, const char* format, va_list ap);

// MosquitOS extensions
unsigned int mstd_popCnt(uint32_t x);

#endif