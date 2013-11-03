#include <types.h>
#include "task.h"
#include "kheap.h"

// Needed to set up the task
static uint32_t next_pid;

// Used to speed up linked-list stuff
static i386_task_t *task_first;
static i386_task_t *task_last;

// External assembly routines
void task_restore_context(i386_task_state_t*);

static uint8_t task_temp_fxsave_region[512] __attribute__((aligned(16)));

/*
 * Saves the state of the task.
 */
void task_save_state(i386_task_t* task, void *regPtr) {
	// Back up the process' FPU/SSE state
	i386_task_state_t *state = task->task_state;

	// If FPU state memory area is not allocated, allocate it
	if(state->fpu_state == NULL) {
		state->fpu_state = (uint8_t *) kmalloc(512);
	}

	__asm__ volatile(" fxsave %0; " : "=m" (task_temp_fxsave_region));

	// Copy into the thread context table
	memcpy(state->fpu_state, &task_temp_fxsave_region, 512);

	// Cast the register struct pointer to what it really is
	sched_trap_regs_t *regs = regPtr;

	// Since starting at gs and ending with ss, the format of sched_trap_regs_t
	// and i386_task_state_t is identical, we can simply perform a memcpy of
	// 68 bytes (17 dwords) the relevant places in the struct.
	memcpy(state, regs+offsetof(sched_trap_regs_t, gs), 0x44);
}

/*
 * Performs a context switch to the specified task.
 */
void task_switch(i386_task_t* task) {
	// Restore CPU state
	i386_task_state_t *state = task->task_state;
	task_restore_context(state);
}

/*
 * Allocates a new task.
 */
i386_task_t *task_allocate() {
	// Try to get some memory for the task
	i386_task_t *task = (i386_task_t*) kmalloc(sizeof(i386_task_t));

	ASSERT(task != NULL);

	// Set up the task struct
	memset(task, 0x00, sizeof(i386_task_t));

	task->prev = task_last;
	task_last = task;

	task->pid = next_pid++;

	task->task_state->fpu_state = (uint8_t *) kmalloc(512);

	// TODO: Set up paging for the task, allocating some memory for it

	return task;
}

/*
 * Deallocates the task.
 */
void task_deallocate(i386_task_t* task) {
	i386_task_t *next = task->next;
	i386_task_t *prev = task->prev;

	// If there is an next task, update its previous pointer
	if(next) {
		if(prev) {
			next->prev = prev;
		} else {
			next->prev = NULL;
		}
	}

	// If there's a previous task, update its next pointer
	if(prev) {
		if(next) {
			prev->next = next;
		} else {
			prev->next = NULL;
		}
	}

	// Notify scheduler that this task is removed
	sched_task_deleted(task);

	// Clean up memory.
	kfree(task->task_state->fpu_state);
	kfree(task);
}

/*
 * Access to the linked list pointers
 */
i386_task_t *task_get_first() {
	return task_first;
}

i386_task_t *task_get_last() {
	return task_last;
}