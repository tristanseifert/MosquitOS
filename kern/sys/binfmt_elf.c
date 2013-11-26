#include <types.h>

#include "binfmt_elf.h"
#include "paging.h"
#include "vm.h"
#include "syscall.h"

#define elf_check_magic(x) \
	x[0] != 0x7F || x[1] != 0x45 || \
	x[2] != 0x4C || x[3] != 0x46

elf_file_t* elf_load_binary(void* buffer) {
	elf_header_t *header = (elf_header_t*) buffer;

	if(elf_check_magic(header->ident.magic)) {
		kprintf("Invalid ELF file: Invalid magic 0x%X%X%X%X\n", header->ident.magic[0], header->ident.magic[1], header->ident.magic[2], header->ident.magic[3]);
		return NULL;
	}

	// Check architecture
	if(header->machine != 0x03) {
		kprintf("Invalid ELF file: Invalid architecture 0x%X\n", header->machine);
		return NULL;
	}

	// If the file is valid, allocate some memory
	elf_file_t *file_struct = (elf_file_t *) kmalloc(sizeof(elf_file_t));
	ASSERT(file_struct != NULL);

	memclr(file_struct, sizeof(elf_file_t));

	file_struct->elf_file_memory = buffer;
	file_struct->header = header;

	// Parse program header table
	void* program_header_table = buffer + header->ph_offset;
	for(int i = 0; i < header->ph_entry_count; i++) {

		// Go to next header table entry
		program_header_table += header->ph_entry_size;
	}

	// Parse section header table
	void* section_header_table = buffer + header->sh_offset;
	for(int i = 0; i < header->sh_entry_count; i++) {

		// Go to next header table entry
		section_header_table += header->sh_entry_size;
	}

	return file_struct;
}