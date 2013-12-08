/*
 * acmosquitos.h: MosquitOS-specific platform code for ACPICA.
 */

#ifndef __ACMOSQUITOS_H__
#define __ACMOSQUITOS_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <runtime/std.h>
#include <sys/system.h>

#define ACPI_USE_SYSTEM_CLIBRARY
#define ACPI_USE_DO_WHILE_0

// We cheat a bit and implement mutexes as semaphores.
#define ACPI_MUTEX_TYPE				ACPI_BINARY_SEMAPHORE

// Host-dependant types
#define ACPI_MACHINE_WIDTH			32
#define ACPI_EXPORT_SYMBOL(symbol)

#define ACPI_SPINLOCK				void *
#define ACPI_CPU_FLAGS				unsigned long

#include "acgcc.h"

#define ACPI_SYSTEM_XFACE
#define ACPI_EXTERNAL_XFACE
#define ACPI_INTERNAL_XFACE
#define ACPI_INTERNAL_VAR_XFACE

#define ACPI_FLUSH_CPU_CACHE sys_flush_cpu_caches

#define ACPI_DIV_64_BY_32(n_hi, n_lo, d32, q32, r32) __asm__ volatile("div %2" : "=a"(q32), "=d"(r32) : "d"(n_hi), "a"(n_lo), "m"(d32));
#define ACPI_SHIFT_RIGHT_64(n_hi, n_lo) 

#endif /* __ACMOSQUITOS_H__ */
