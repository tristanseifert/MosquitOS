CC=i686-pc-elf-gcc
AS=i686-pc-elf-as
LD=i686-pc-elf-ld

obj_arc_ext=oa

make_obj_archive= \
	rm -f $@.$(obj_arc_ext); \
	$(LD) -r *.o -o $@.$(obj_arc_ext); \
	rm -f *.o \

BUILD_NUMBER_FILE=build.num
BUILD_NUMBER_LDFLAGS += -Xlinker --defsym -Xlinker KERN_BNUM=$$(cat $(BUILD_NUMBER_FILE))

# Freestanding binary, strip extra shits
CFLAGS= -c -finline-functions -g -Wall -static -mmmx -msse\
		-O2 -fno-exceptions -ftree-vectorize -nostartfiles -nodefaultlibs -std=c99\
		-Wno-unused-variable -mfpmath=387+sse -fno-stack-protector -I. -I./includes/\
		-Wno-unused-function -mtune=pentium3 -fno-omit-frame-pointer \
		-Wno-unused-but-set-variable -pipe -march=i686

CFLAGSNOOPT = $(filter-out -O2,$(CFLAGS))

ASFLAGS =

# Add toolchain to path
export PATH := ../toolchain/gcc/bin/:$(PATH)

all: setup acpi vga drivers_intel dev_drivers runtime system fs bus v86 modules loader kernel

# Clean up from previous build
clean:
	@rm -rf tmp
	@rm -f *.o *.$(obj_arc_ext)

# Compile main.c into flat binary
kernel: main.c
	@if ! test -f $(BUILD_NUMBER_FILE); then echo 0 > $(BUILD_NUMBER_FILE); fi
	@echo $$(($$(cat $(BUILD_NUMBER_FILE)) + 1)) > $(BUILD_NUMBER_FILE)

	@$(CC) $(CFLAGS) -ffreestanding *.c -o kern_main.$(obj_arc_ext)
	@$(CC) -Wl,-T,kern.ld -o kernel.elf -ffreestanding -g -O2 -nostdlib $(BUILD_NUMBER_LDFLAGS) tmp/entry.o *.$(obj_arc_ext)
	
	@i686-pc-elf-objdump -b elf32-i386 -S -d kernel.elf > symbols.txt
	@i686-pc-elf-objcopy -I elf32-i386 --only-keep-debug kernel.elf kernel.sym
	@i686-pc-elf-objcopy -I elf32-i386 --strip-debug kernel.elf

# Sets up directories and stuff.
setup:
	@rm -rf tmp
	@mkdir tmp
	@clear

# Runtime functions, such as the C standard library (printf, fopen, strlen, etc)
runtime: runtime/*.c
	@$(CC) $(CFLAGS) -ffreestanding runtime/*.c
	@$(make_obj_archive)

# Drivers for hardware
dev_drivers: io device/*.c
	@$(CC) $(CFLAGS) -ffreestanding device/*.c
	@$(make_obj_archive)

# IO routines, i.e. driver interfaces
io: io/*.c
	@$(CC) $(CFLAGS) -ffreestanding io/*.c
	@$(make_obj_archive)

# Virtual 8086 mode support
v86: v86/*.c v86/*.S
	@$(CC) $(CFLAGS) -ffreestanding v86/*.c
	@nasm -f elf v86/v86_trampoline.S -o v86_trampoline.o
	@$(make_obj_archive)

# System interfaces, such as syscalls and hardware setup (V86 too!)
# These are *not* optimised as gcc likes to muck with inline assembly
system: sys/*.c sys/*.S
	@$(CC) $(CFLAGSNOOPT) -ffreestanding sys/*.c
	@$(AS) $(ASFLAGS) -o irq_handlers.o sys/irq_handlers.S
	@$(AS) $(ASFLAGS) -o syscall_alt.o sys/syscall.S
	@$(AS) $(ASFLAGS) -o sched_alt.o sys/sched.S
	@$(AS) $(ASFLAGS) -o task_alt.o sys/task.S
	@$(make_obj_archive)

# Filesystem libraries
fs: fs/*.c
	@$(CC) $(CFLAGS) -ffreestanding fs/*.c
	@$(make_obj_archive)

# Video-related modules
vga: vga/*.c
	@$(CC) $(CFLAGS) -ffreestanding vga/*.c
	@$(make_obj_archive)

# Modules for interfacing with various busses (PCI, ISA, etc)
bus: bus/*.c
	@$(CC) $(CFLAGS) -I./acpi/acpica/include/ -ffreestanding bus/*.c
	@$(make_obj_archive)

# Management of kernel modules
modules: modules/*.c
	@$(CC) $(CFLAGS) -ffreestanding modules/*.c
	@$(make_obj_archive)

# ACPI-related code
acpi: acpi/*.c
	@$(CC) $(CFLAGS) -I./acpi/acpica/include/ -ffreestanding acpi/*.c
	@cp objs/acpica.oa acpica.o
	@$(make_obj_archive)

# ACPICA is only compiled on request, otherwise we link against the prebuilt one
acpica: acpi/acpica/*.c
	@$(CC) $(CFLAGSNOOPT) -I./acpi/acpica/include/ -Wno-format -fno-strict-aliasing -ffreestanding acpi/acpica/*.c
	@$(make_obj_archive)
	@mv $@.oa objs/$@.oa

# Assembly loader
loader: entry.s
	@$(AS) $(ASFLAGS) -o entry.o entry.s
	@mv entry.o tmp/entry.o

# Drivers for Intel hardware and chipsets
drivers_intel: drivers_intel/*.c
	@$(CC) $(CFLAGS) -ffreestanding drivers_intel/*.c
	@$(make_obj_archive)