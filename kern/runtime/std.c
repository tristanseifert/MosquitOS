#include <types.h>
#include "std_sse.h"
#include "std.h"
#include "sys/kheap.h"
#include "sys/system.h"
#include "sys/cpuid.h"

/*
 * Portions of this code are taken from the OpenBSD libc.
 *
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

typedef void* (*memclr_prototype)(void*, size_t);

// function pointers that point to SSE/non-SSE versions of code
memclr_prototype std_memclr_fp;

/*
 * Sets up function pointers to SEE/non-SSE versions of some STD functions.
 */
void std_set_up_fp() {
	uint32_t cpuid_edx, temp;
	cpuid(1, cpuid_edx, temp, temp, temp);
	
	// We have SSE support
	if(cpuid_edx & CPUID_FEAT_EDX_SSE) {
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
 * Separates string s by delim, returning either NULL if there is no more 
 * strings that can be split out, or the next split if there is any.
 */
char* strtok(char *s, const char *delim) {
	char *spanp;
	int c, sc;
	char *tok;
	static char *last;

	if (s == NULL && (s = last) == NULL)
		return NULL;

	// Skip (span) leading delimiters (s += strspn(s, delim), sort of).
cont:
	c = *s++;
	for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}

	if (c == 0) { // no non-delimiter characters
		last = NULL;
		return NULL;
	}
	tok = s - 1;

	/*
	 * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
	 * Note that delim must have one NUL; we stop if we see that, too.
	 */
	for (;;) {
		c = *s++;
		spanp = (char *) delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				last = s;
				return tok;
			}
		} while (sc != 0);
	}
}

/*
 * Returns a pointer to the first occurrence of character c in string s.
 */
char* strchr(const char *s, int c) {
	const char ch = c;

	for ( ; *s != ch; s++)
		if (*s == '\0')
			return NULL;
	return (char *) s;
}

/*
 * Get next token from string *stringp, where tokens are possibly-empty
 * strings separated by characters from delim.  
 *
 * Writes NULs into the string at *stringp to end tokens.
 * delim need not remain constant from call to call.
 * On return, *stringp points past the last NUL written (if there might
 * be further tokens), or is NULL (if there are definitely no more tokens).
 *
 * If *stringp is NULL, strsep returns NULL.
 */
char* strsep(char **stringp, const char *delim) {
	char *s;
	const char *spanp;
	int c, sc;
	char *tok;

	if ((s = *stringp) == NULL) {
		return NULL;
	}

	for (tok = s;;) {
		c = *s++;
		spanp = delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0) {
					s = NULL;
				} else {
					s[-1] = 0;
				}

				*stringp = s;
				return tok;
			}
		} while (sc != 0);
	}
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
 * Clears count bytes of memory, starting at start, with 0x00.
 */
void* memclr(void* start, size_t count) {
	// Programmer is an idiot
	if(!count) return start;

	uint32_t cpuid_edx, temp;
	cpuid(1, cpuid_edx, temp, temp, temp);

	if(cpuid_edx & CPUID_FEAT_EDX_SSE) {
		return memclr_sse(start, count);
	} else {
		return memclr_std(start, count);
	}
}