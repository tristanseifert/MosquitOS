#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdint-gcc.h>
#include <limits.h>

#include "errno.h" 

// Internal functions
__attribute__((visibility("internal"))) int isupper(int ch);
__attribute__((visibility("internal"))) int isdigit(int ch);
__attribute__((visibility("internal"))) int isspace(int ch);
__attribute__((visibility("internal"))) int isalpha(int ch);