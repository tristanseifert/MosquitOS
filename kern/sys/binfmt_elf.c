#include <types.h>

#include "binfmt_elf.h"
#include "paging.h"
#include "vm.h"
#include "syscall.h"

void* elf_load_binary(void* buffer) {
	elf_header_t *header = (elf_header_t*) buffer;

	if(header->ident.magic != ELF_MAGIC) {
		kprintf("Invalid ELF file.\n");
		return NULL;
	}

	return 0xDEADBEEF;
}