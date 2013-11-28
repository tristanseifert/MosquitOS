#include "syscalls_internal.h"

int do_syscall(int num, void* info) {
	uint32_t syscall_return;

	// Use sysenter mechanism for syscall
	__asm__ volatile("mov %%esp, %%ecx; mov %0, %%ebx; mov %1, %%eax; mov $.+7, %%edx; sysenter;"
		: "=a" (syscall_return)
		: "r" (num), "r" (info));

	// Set error number, return
	errno = syscall_return;
	return syscall_return;
}