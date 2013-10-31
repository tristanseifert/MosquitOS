#include <types.h>
#include "task.h"
#include "kheap.h"

// Needed to set up the task
static uint32_t next_pid;

// Used to speed up linked-list stuff
static i386_task_t *task_first;
static i386_task_t *task_last;

/*
 * Saves the state of the task.
 */
void task_save_state(i386_task_t task) {

}

/*
 * Performs a context switch to the specified task.
 */
void task_switch(i386_task_t task) {

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
	kfree(task->fpu_state);
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