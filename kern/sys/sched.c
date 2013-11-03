#include <types.h>
#include "sched.h"
#include "task.h"
#include "system.h"
#include "kheap.h"

// External handler
extern void sched_trap(void);

// Miscellaneous required stuff
static uint64_t scheduler_cycle;

// Point to the previous, current and next task's struct
static i386_task_t *prevTask;
static i386_task_t *currTask;
static i386_task_t *nextTask;

/*
 * Initialises the scheduler.
 */
void sched_init() {
	// Set up a trap gate in the IDT
	sys_set_idt_gate(SCHED_TRAP_NUM, (uint32_t) sched_trap, 0x08, 0x8F);
}

/*
 * Routine called by a process to yield processor control to another.
 *
 * This is usually done by a process executing "int $0x88" and the CPU
 * taking the trap to an assembly wrapper.
 */
void sched_yield(sched_trap_regs_t regs) {
	// Save the task state
	task_save_state(currTask, &regs);

	// Prepare the next process for execution
	prevTask = currTask;
	currTask = nextTask;

	// Run scheduler to decide next process
	sched_chose_next();

	// Update scheduler cycle info
	sched_task_t *schedInfo = currTask->scheduler_info;
	schedInfo->last_cycle = scheduler_cycle;

	// Do context switch
	task_switch(currTask);
}

/*
 * Chooses the next process to run.
 */
void sched_chose_next() {
	// Check to see if we have any processes whose events have been processed
	i386_task_t *iterator = task_get_first();

	while(iterator) {
		// Does task have an event pending?
		if(iterator->isWaitingForEvent && iterator->eventHasArrived) {
			sched_task_t *schedInfo = iterator->scheduler_info;

			// Check if it was executed this cycle
			if(schedInfo->last_cycle != scheduler_cycle) {
				// If not, run it and set CPU use counter
				nextTask = iterator;
				schedInfo->num_cpu_per_cycle = 1;
				return;
			} else {
				// We now need to check if it's used the CPU more than permitted
				if(schedInfo->num_cpu_per_cycle < SCHED_MAX_EXEC_PER_CYCLE) {
					// If not, run it
					nextTask = iterator;
					schedInfo->num_cpu_per_cycle++;
					return;
				} else {
					// It's got an event pending but used its allowance of CPU cycles
				}
			}
		}

		// Go to next task
		iterator = iterator->next;
	}

	// If there's no tasks that have events, just pick the next one
	i386_task_t *next = currTask->next;

	// We've executed all tasks, loop over
	if(!next) {
		next = task_get_first();
		scheduler_cycle++;
	}

	nextTask = next;
}

/*
 * Cleans up scheduler info and the like, and makes sure that we do not try to
 * schedule this task as it is deleted and may crash the system.
 */
void sched_task_deleted(void *in) {
	i386_task_t *task = in;

	kfree(task->scheduler_info);

	// The task slated for execution next is this one
	if(nextTask == task) {
		if(task->next) {
			nextTask = task->next;
		} else {
			nextTask = task_get_first();
		}
	}
}

/*
 * Sets up scheduler info and ensures the scheduler is aware of this task.
 */
void sched_task_created(void *in) {
	i386_task_t *task = in;

	sched_task_t *schedInfo = (sched_task_t *) kmalloc(sizeof(sched_task_t));
	ASSERT(schedInfo != NULL);

	memset(schedInfo, 0x00, sizeof(sched_task_t));

	task->scheduler_info = schedInfo;
}