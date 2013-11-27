#include <types.h>

#include "binfmt_elf.h"
#include "paging.h"
#include "vm.h"
#include "syscall.h"

#define elf_check_magic(x) \
	x[0] != 0x7F || x[1] != 0x45 || \
	x[2] != 0x4C || x[3] != 0x46

elf_file_t* elf_load_binary(void* elfIn) {
	uint8_t *buffer = (uint8_t *) elfIn;
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

	ASSERT(header->ph_entry_size == sizeof(elf_program_entry_t));
	ASSERT(header->sh_entry_size == sizeof(elf_section_entry_t));

	// Try to find the string table
	unsigned char* stringTablePtr = NULL;
	if(header->sh_str_index != SHN_UNDEF) {
		kprintf("String table index: 0x%X 0x%X\n", header->sh_str_index, header->sh_offset);

		elf_section_entry_t *entry = (elf_section_entry_t *) (buffer + header->sh_offset) + (sizeof(elf_section_entry_t) * header->sh_str_index);
		stringTablePtr = buffer + entry->sh_offset;

		kprintf("Found string table at offset 0x%X, size 0x%X (0x%X)\n", entry->sh_offset, entry->sh_size, header->sh_offset + (sizeof(elf_section_entry_t) * header->sh_str_index));
	}

	// Parse program header table
	for(int i = 0; i < header->ph_entry_count; i++) {
		elf_program_entry_t *entry = (elf_program_entry_t *) (buffer + header->ph_offset) + (sizeof(elf_program_entry_t) * i);
	}

	// Parse section header table
	for(int i = 0; i < header->sh_entry_count; i++) {
		elf_section_entry_t *entry = (elf_section_entry_t *) (buffer + header->sh_offset) + (sizeof(elf_section_entry_t) * i);
	}

	return file_struct;
}