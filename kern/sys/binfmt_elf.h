/*
 * ELF loading and friends.
 */

#ifndef BINFMT_ELF_H
#define BINFMT_ELF_H

#include <types.h>

#define SHN_UNDEF		0
#define SHN_LORESERVE	0xFF00
#define SHN_LOPROC		0xFF00
#define SHN_HIPROC		0xFF1F
#define SHN_ABS			0xFFF1
#define SHN_COMMON		0xFFF2
#define SHN_HIRESERVE	0xFFFF

#define SHT_NULL 		0
#define SHT_PROGBITS	1
#define SHT_SYMTAB		2
#define SHT_STRTAB		3
#define SHT_RELA		4
#define SHT_HASH		5
#define SHT_DYNAMIC		6
#define SHT_NOTE		7
#define SHT_NOBITS		8
#define SHT_REL			9
#define SHT_SHLIB		10
#define SHT_DYNSYM		11
#define SHT_LOPROC		0x70000000
#define SHT_HIPROC		0x7FFFFFFF
#define SHT_LOUSER		0x80000000
#define SHT_HIUSER		0xFFFFFFFF

#define SHF_WRITE		0x00000001
#define SHF_ALLOC		0x00000002
#define SHF_EXECINSTR	0x00000004
#define SHF_MASKPROC	0xF0000000

#define STN_UNDEF		0

#define STB_LOCAL		0
#define STB_GLOBAL		1
#define STB_WEAK		2
#define STB_LOPROC		13
#define STB_HIPROC		15

#define STT_NOTYPE		0
#define STT_OBJECT		1
#define STT_FUNC		2
#define STT_SECTION		3
#define STT_FILE		4
#define STT_LOPROC		13
#define STT_HIPROC		15

#define PT_NULL			0
#define PT_LOAD			1
#define PT_DYNAMIC		2
#define PT_INTERP		3
#define PT_NOTE			4
#define PT_SHLIB		5
#define PT_PHDR			6
#define PT_LOPROC		0x70000000
#define PT_HIPROC		0x7FFFFFFF

#define PF_X			0x00000001
#define PF_W			0x00000002
#define PF_R			0x00000004
#define PF_MASKPROC		0xF0000000

#define ELF32_ST_BIND (i) ((i) >> 4)
#define ELF32_ST_TYPE (i) ((i) & 0xF)
#define ELF32_ST_INFO (b, t) (((b) << 4) + ((t) & 0xF)

typedef struct {
	char magic[4];

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

typedef struct{
	uint32_t sh_name;
	uint32_t sh_type;
	uint32_t sh_flags;
	uint32_t sh_addr;
	uint32_t sh_offset;
	uint32_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint32_t sh_addralign;
	uint32_t sh_entsize;
} __attribute__((packed)) elf_section_entry_t;

typedef struct elf_symbol_entry {
	uint32_t st_name; // k
	uint32_t st_address; // k
	uint32_t st_size; // k
	unsigned char st_info; // k
	unsigned char st_other; // k
	uint16_t st_shndx; // k
} __attribute__((packed)) elf_symbol_entry_t;

typedef struct{
	uint32_t r_offset;
	uint32_t r_info;
} __attribute__((packed)) elf_program_relocation;

typedef struct{
	uint32_t r_offset;
	uint32_t r_info;
	int32_t r_addend;
} __attribute__((packed)) elf_program_relocation_addend;

typedef struct{
	uint32_t p_type;
	uint32_t p_offset;
	uint32_t p_vaddr;
	uint32_t p_paddr;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags;
	uint32_t p_align;
} __attribute__((packed)) elf_program_entry_t;


typedef struct elf_file {
	void* elf_file_memory;
	elf_header_t* header;

	unsigned char* stringTable;
	unsigned char* symbolStringTable;

	void *section_text;

	elf_symbol_entry_t *symbolTable;
} elf_file_t;

elf_file_t* elf_load_binary(void* buffer);

#endif