#ifndef STD_H
#define STD_H

// string functions: strings
size_t strlen(char* str);

// string functions: memory
void* memchr(void* ptr, uint8_t value, size_t num);
int memcmp(void* ptr1, void* ptr2, size_t num);
void* memcpy(void* destination, void* source, size_t num);
void *memset(void* ptr, uint8_t value, size_t num);

// standard library: memory
void* malloc(size_t num_bytes);
void* memclr(void* start, size_t count);

#endif