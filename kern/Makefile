CC=i686-pc-elf-gcc
AS=i686-pc-elf-as
LD=i686-pc-elf-ld

obj_arc_ext=oa

make_obj_archive= \
	$(LD) -r *.o -o $@.$(obj_arc_ext); \
	rm -f *.o \

BUILD_NUMBER_FILE=build.num
BUILD_NUMBER_LDFLAGS += -Xlinker --defsym -Xlinker KERN_BNUM=$$(cat $(BUILD_NUMBER_FILE))

# Freestanding binary, strip extra shits
CFLAGS= -c -finline-functions -g -Wall -fno-builtin -static\
		-mno-sse2 -Os\
		-nostartfiles -nodefaultlibs -std=c99 -Wno-unused-variable \
		-mfpmath=387 -fno-stack-protector -I/Users/tristanseifert/SquelchenOS/kern/\
		-Wno-unused-function

CFLAGSNOOPT = $(filter-out -Os,$(CFLAGS))

ASFLAGS =

all: setup runtime drivers io vga system fs bus loader kernel clean

# Clean up from previous build
clean:
	rm -rf tmp
	rm -f *.o *.$(obj_arc_ext)

# Compile main.c into flat binary
kernel: main.c
	@if ! test -f $(BUILD_NUMBER_FILE); then echo 0 > $(BUILD_NUMBER_FILE); fi
	@echo $$(($$(cat $(BUILD_NUMBER_FILE)) + 1)) > $(BUILD_NUMBER_FILE)

	$(CC) $(CFLAGS) -ffreestanding *.c -o kern_main.$(obj_arc_ext)
	i686-pc-elf-gcc -Wl,-T,kern.ld -o kernel.elf -ffreestanding -Os -nostdlib $(BUILD_NUMBER_LDFLAGS) tmp/entry.o *.$(obj_arc_ext)
	gobjcopy -I elf32-i386 -O binary kernel.elf kernel.bin
	gobjdump -b elf32-i386 -d kernel.elf > symbols.txt

# Sets up directories and stuff.
setup:
	rm -rf tmp
	mkdir tmp
	clear

# Runtime functions, such as the C standard library (printf, fopen, strlen, etc)
runtime: runtime/*.c
	$(CC) $(CFLAGS) -ffreestanding runtime/*.c
	$(make_obj_archive)

# Drivers for hardware
drivers: device/*.c
	$(CC) $(CFLAGS) -ffreestanding device/*.c
	$(make_obj_archive)

# IO routines, i.e. driver interfaces
io: io/*.c
	$(CC) $(CFLAGS) -ffreestanding io/*.c
	$(make_obj_archive)

# System interfaces, such as syscalls and hardware setup (V86 too!)
# These are *not* optimised as gcc likes to muck with inline assembly
system: sys/*.c sys/*.S
	$(CC) $(CFLAGSNOOPT) -ffreestanding sys/*.c
	$(AS) $(ASFLAGS) -o irq_handlers.o sys/irq_handlers.S
	$(AS) $(ASFLAGS) -o syscall_alt.o sys/syscall.S
	$(AS) $(ASFLAGS) -o sched_alt.o sys/sched.S
	$(AS) $(ASFLAGS) -o task_alt.o sys/task.S
	$(AS) $(ASFLAGS) -o vm_alt.o sys/vm.S
	$(make_obj_archive)

# Filesystem libraries
fs: fs/*.c
	$(CC) $(CFLAGS) -ffreestanding fs/*.c
	$(make_obj_archive)

# Video-related modules
vga: vga/*.c
	$(CC) $(CFLAGS) -ffreestanding vga/*.c
	$(make_obj_archive)

# Modules for interfacing with various busses (PCI, ISA, etc)
bus: bus/*.c
	$(CC) $(CFLAGS) -ffreestanding bus/*.c
	$(make_obj_archive)

# Assembly loader
loader: entry.s
	$(AS) $(ASFLAGS) -o entry.o entry.s
	mv entry.o tmp/entry.o