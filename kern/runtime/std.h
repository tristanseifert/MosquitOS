// string functions: strings
size_t strlen(const char* str);

// string functions: memory
const void* memchr(const void * ptr, int value, size_t num);
int memcmp (const void* ptr1, const void* ptr2, size_t num);
void* memcpy(void* destination, const void* source, size_t num);
void *memset(void * ptr, int value, size_t num);