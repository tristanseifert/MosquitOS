#ifndef TASK_H
#define TASK_H

#include <types.h>
#include "sched.h"

typedef struct task {
	// These can be directly copied from the stack when the task is interrupted
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
	uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.

	// Page table address
	uint32_t page_table;

	// FPU/SSE state memory pointer (must be aligned to 16 byte boundary, kernel ptr)
	void* fpu_state; // FXSAVE/FXRSTOR

	// Miscellaneous task info
	uint32_t pid;
	uint32_t user;
	uint32_t group;

	char name[128];

	// Pointer to scheduler-specific data (kernel ptr)
	void* scheduler_info;

	// Event handling
	bool isWaitingForEvent;
	bool eventHasArrived; // If true, scheduler favours this

	uint32_t eventCode;
	uint32_t eventUsr;

	// Linked list
	struct task* prev;
	struct task* next;
} i386_task_t;

// Saves the state of the task in an interrupt/syscall handler
void task_save_state(i386_task_t task);
// Switches to the specified task
void task_switch(i386_task_t task);

// Creation/destruction of tasks
i386_task_t *task_allocate();
void task_deallocate(i386_task_t* task);

// Access to the linked list
i386_task_t *task_get_first();
i386_task_t *task_get_last();

#endif