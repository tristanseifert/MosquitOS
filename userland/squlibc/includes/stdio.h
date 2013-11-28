/*
 * SQULibC - Standard input/output functions
 */
int putchar(char c);

int printf(const char *fmt, ...);

// FILE handles are offsets into a per-process table for MosquitOS
typedef int FILE;