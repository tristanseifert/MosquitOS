#ifndef TYPES_H
#define TYPES_H

#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdint.h>

// This gives us all the runtime functions
#include <runtime/std.h>
#include <sys/kheap.h>
#include <io/console.h>
// Include panic functions
#include <runtime/panic.h>

#define PANIC(msg) panic(msg, __FILE__, __LINE__);
#define ASSERT(b) ((b) ? (void)0 : panic_assert(__FILE__, __LINE__, #b))

// These tell gcc how to optimise branches since it's stupid
#define likely(x)    __builtin_expect(!!(x), 1)
#define unlikely(x)  __builtin_expect(!!(x), 0)

#define ENDIAN_DWORD_SWAP(x) ((x >> 24) & 0xFF) | ((x << 8) & 0xFF0000) | ((x >> 8) & 0xFF00) | ((x << 24) & 0xFF000000)
#define ENDIAN_WORD_SWAP(x) ((x & 0xFF) << 0x08) | ((x & 0xFF00) >> 0x08)

#endif