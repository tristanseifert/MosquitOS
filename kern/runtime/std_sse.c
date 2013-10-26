#include <types.h>
#include "std_sse.h"

#define SSE_XMSIZE 0x10

/*
 * Clears count bytes starting at start with 0x00.
 */
void *memclr_sse(void* start, size_t count) {
	// "i" is our counter of how many bytes we've cleared
	size_t i;

	// find out if "start" is aligned on a SSE_XMSIZE boundary
	if((size_t) start & (SSE_XMSIZE - 1)) {
		i = 0;

		// we need to clear byte-by-byte until "start" is aligned on an SSE_XMSIZE boundary
		// ... and lets make sure we don't copy 'too' many bytes (i < count)
		while(((size_t) start + i) & (SSE_XMSIZE - 1) && i < count) {
			__asm__("stosb;" :: "D"((size_t) start + i), "a"(0));
			i++;
		}
	} else {
		// if "start" was aligned, set our count to 0
		i = 0;
	}
 
	// clear 64-byte chunks of memory (4 16-byte operations)
	for(; i + 64 <= count; i += 64) {
		__asm__ volatile(" pxor %%xmm0, %%xmm0;	" // set XMM0 to 0
					 " movdqa %%xmm0, 0(%0);	" // move 16 bytes from XMM0 to %0 + 0
					 " movdqa %%xmm0, 16(%0);	"
					 " movdqa %%xmm0, 32(%0);	"
					 " movdqa %%xmm0, 48(%0);	"
					 :: "r"((size_t) start + i));
	}
 
	// copy the remaining bytes (if any)
	__asm__(" rep stosb; " :: "a"((size_t)(0)), "D"(((size_t) start) + i), "c"(count - i));

	// "i" will contain the total amount of bytes that were actually transfered
	i += count - i;

	// we return "start" + the amount of bytes that were transfered
	return (void *)(((size_t) start) + i);
}

void *memclr_std(void* start, size_t count) {
    __asm__("rep stosl;"::"a"(0),"D"((size_t) start),"c"(count / 4));
    __asm__("rep stosb;"::"a"(0),"D"(((size_t) start) + ((count / 4) * 4)),"c"(count - ((count / 4) * 4)));

    return (void *) ((size_t) start + count);
}