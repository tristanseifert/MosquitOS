/*
 * ELF loading and friends.
 */

#ifndef BINFMT_ELF_H
#define BINFMT_ELF_H

#include <types.h>

#define ELF_MAGIC 0x7F454C46

typedef struct {
	uint32_t magic;

	uint8_t bitness; // 1 = 32 bit, the only one supported by this
	uint8_t endianness; // 1 = little, 2 = big
	uint8_t version;

	uint8_t abi;
	uint8_t abi_version;

	uint8_t reserved[7];
} __attribute__((packed)) elf_ident_t;

typedef struct {
	elf_ident_t ident;

	uint16_t type;
	uint16_t machine;

	uint32_t version;

	// these fields are 64 bits if the ELF is 64 bits
	uint32_t entry;
	uint32_t ph_offset; // program header table offset
	uint32_t sh_offset; // section header table offset

	uint32_t flags;

	uint16_t header_size; // should be 52 bytes for 32-bit header

	uint16_t ph_entry_size; // size of program header table entry
	uint16_t ph_entry_count;

	uint16_t sh_entry_size; // size of section header table entry
	uint16_t sh_entry_count;

	uint16_t sh_str_index; // section header index for section names
} __attribute__((packed)) elf_header_t;

void* elf_load_binary(void* buffer);

#endif