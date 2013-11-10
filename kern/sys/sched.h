#ifndef SCHED_H
#define SCHED_H

#include <types.h>
#include "task.h"

// software interrupt to trap into scheduler
#define SCHED_TRAP_NUM 0x88

// PIT interval for a timeslice
#define SCHED_TIMESLICE 14915
#define SCHED_TIMESLICE_MS SCHED_TIMESLICE / (3579545 / 3) * 1000

// Maximum times a process can get run in one scheduling cycle
#define SCHED_MAX_EXEC_PER_CYCLE 8

typedef struct sched_trap_registers {
	uint32_t event_code, event_state;

	uint32_t gs, fs, es, ds;

	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
	uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
} __attribute__((packed)) sched_trap_regs_t;

typedef struct sched_info {
	// The last "scheduling cycle" this process was ran.
	uint64_t last_cycle;
	// How many times this process had the CPU
	uint32_t num_cpu_per_cycle;

	// Pointer to the task descriptor
	void *task_descriptor;
} sched_task_t;

// Initialises scheduler
void sched_init();
// Called when a process yields control to the scheduler
void sched_yield(sched_trap_regs_t regs);
// Called when a task is destroyed
void sched_task_deleted(void *in);
// Called when a task is created
void sched_task_created(void *in);
// Returns the currently executing task.
void* sched_curr_task();

#endif