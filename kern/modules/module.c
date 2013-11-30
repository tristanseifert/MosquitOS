#include "module.h"
#include <types.h>

extern uint32_t __kern_initcalls, __kern_exitcalls, __kern_callsend;

/*
 * Runs the init functions of all modules compiled statically into the kernel.
 */
void modules_load() {
	module_initcall_t *initcallArray = (module_initcall_t *) &__kern_initcalls;

	int i = 0;
	while(initcallArray[i] != NULL) {
		int returnValue = (initcallArray[i]());

		i++;
	}
}