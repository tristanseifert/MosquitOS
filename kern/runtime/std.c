#include <types.h>
#include "sys/kheap.h"

/*
 * Returns the length of the string passed in str in bytes before a \0 terminator.
 */ 
size_t strlen(const char* str) {
	size_t ret = 0;
	
	while (str[ret] != 0x00) {
		ret++;
	}
	
	return ret;
}

/*
 * Finds the first occurrence of value in the first num bytes of ptr.
 */
const void* memchr(const void * ptr, uint8_t value, size_t num) {
	uint8_t *read = (uint8_t *) ptr;

	for(int i = 0; i < num; i++) {
		if(read[i] == value) return &read[i];
	}

	return NULL;
}

/*
 * Compares the first num bytes in two blocks of memory.
 * Returns 0 if equal, a value greater than 0 if the first byte in ptr1 is
 * greater than the first byte in ptr2; and a value less than zero if the
 * opposite. Note that these comparisons are performed on uint8_t types.
 */
int memcmp(const void* ptr1, const void* ptr2, size_t num) {
	uint8_t *read1 = (uint8_t *) ptr1;
	uint8_t *read2 = (uint8_t *) ptr2;

	for(int i = 0; i < num; i++) {
		if(read1[i] != read2[i]) {
			if(read1[i] > read2[i]) return 1;
			else return -1;
		}
	}

	return 0;
}

/*
 * Copies num bytes from source to destination.
 */
void* memcpy(void* destination, const void* source, size_t num) {
	uint8_t *dst = (uint8_t *) destination;
	uint8_t *src = (uint8_t *) source;

	for(int i = 0; i < num; i++) {
		dst[i] = src[i];
	}

	return destination;
}

/*
 * Fills a given segment of memory with a specified value.
 */
void* memset(void* ptr, uint8_t value, size_t num) {
	uint8_t *write = (uint8_t *) ptr;

	for(int i = 0; i < num; i++) {
		write[i] = value;
	}

	return ptr;
}

/*
 * Allocates num_bytes bytes of memory on the kernel heap.
 */
void* malloc(size_t num_bytes) {
	return (void *) kmalloc_int(num_bytes, false, 0);
}