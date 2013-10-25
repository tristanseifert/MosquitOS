#include <types.h>
#include "std_sse.h"
#include "std.h"
#include "sys/kheap.h"
#include "sys/system.h"

typedef void* (*memclr_prototype)(void*, size_t);

// function pointers that point to SSE/non-SSE versions of code
memclr_prototype std_memclr_fp;

/*
 * Sets up function pointers to SEE/non-SSE versions of some STD functions.
 */
void std_set_up_fp() {
	cpu_info_t *info = sys_get_cpu_info();
	// We have SSE support
	if(info->cpuid_edx & CPUID_FEAT_EDX_SSE) {
		std_memclr_fp = &memclr_sse;
	} else {
		std_memclr_fp = &memclr_std;
	}
}

/*
 * Returns the length of the string passed in str in bytes before a \0 terminator.
 */ 
size_t strlen(char* str) {
	size_t ret = 0;
	
	while (str[ret] != 0x00) {
		ret++;
	}
	
	return ret;
}

/*
 * Finds the first occurrence of value in the first num bytes of ptr.
 */
void* memchr(void* ptr, uint8_t value, size_t num) {
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
int memcmp(void* ptr1, void* ptr2, size_t num) {
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
void* memcpy(void* destination, void* source, size_t num) {
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
	/*if(value == 0x00) {
		return memclr(ptr, num);
	}*/

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

/*
 * Clears count of memory, starting at start, with 0x00.
 */
void* memclr(void* start, size_t count) {

	// Programmer is an idiot
	if(!count) return start;

	if(sys_get_cpu_info()->cpuid_edx & CPUID_FEAT_EDX_SSE) {
		return memclr_sse(start, count);
	} else {
		return memclr_std(start, count);
	}
}