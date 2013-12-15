#ifndef STD_H
#define STD_H

// string functions: strings
size_t strlen(char* str);
char* strtok(char *s, const char *delim);
char *strchr(const char *s, int c);
char* strsep(char **stringp, const char *delim);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);

long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
int atoi(const char *str);

// string functions: memory
void* memchr(void* ptr, uint8_t value, size_t num);
int memcmp(void* ptr1, void* ptr2, size_t num);
void* memcpy(void* destination, void* source, size_t num);
void *memset(void* ptr, uint8_t value, size_t num);

// standard library: memory
void* malloc(size_t num_bytes);
void* memclr(void* start, size_t count);

// sprintf (defined in kprintf.c)
void sprintf(char* s, char *fmt, ...);

// MosquitOS extensions
unsigned int mstd_popCnt(uint32_t x);

#endif