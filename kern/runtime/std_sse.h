#ifndef STD_H
#error Do not include std_sse.h directly!
#endif

/*
 * Declarations for SSE-enabled standard functions, and their standard non-SSE
 * counterparts.
 *
 * Note: This file should NOT be included directly!
 */
void* memclr_sse(void* start, size_t count);
void* memclr_std(void* start, size_t count);
