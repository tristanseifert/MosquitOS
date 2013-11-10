CC=i686-pc-elf-gcc
AS=i686-pc-elf-as

# Freestanding binary, strip extra shits
CFLAGS= -c -funroll-loops -finline-functions -g -Wall -fno-builtin -static\
		-mno-sse2 -O2\
		-nostartfiles -nodefaultlibs -std=c99 -Wno-unused-variable \
		-mfpmath=387 -fno-stack-protector -I/Users/tristanseifert/SquelchenOS/kern/\
		-Wno-unused-function

CFLAGSNOOPT = $(filter-out -O2,$(CFLAGS))

ASFLAGS =

all: setup runtime drivers io system fs loader kernel clean

# Clean up from previous build
clean:
	rm -f *.o
	rm -rf tmp

# Compile main.c into flat binary
kernel: main.c
	$(CC) $(CFLAGS) -ffreestanding *.c
	i686-pc-elf-gcc -Wl,-T,kern.ld -o kernel.elf -ffreestanding -O2 -nostdlib tmp/entry.o *.o 
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

# Drivers for hardware
drivers: device/*.c
	$(CC) $(CFLAGS) -ffreestanding device/*.c

# IO routines, i.e. driver interfaces
io: io/*.c
	$(CC) $(CFLAGS) -ffreestanding io/*.c

# System interfaces, such as syscalls and hardware setup (V86 too!)
# These are *not* optimised as gcc likes to muck with inline assembly
system: sys/*.c sys/*.S
	$(CC) $(CFLAGSNOOPT) -ffreestanding sys/*.c
	$(AS) $(ASFLAGS) -o irq_handlers.o sys/irq_handlers.S
	$(AS) $(ASFLAGS) -o syscall_alt.o sys/syscall.S
	$(AS) $(ASFLAGS) -o sched_alt.o sys/sched.S
	$(AS) $(ASFLAGS) -o task_alt.o sys/task.S
	$(AS) $(ASFLAGS) -o vm_alt.o sys/vm.S

# Filesystem libraries
fs: fs/*.c
	$(CC) $(CFLAGS) -ffreestanding fs/*.c

# Assembly loader
loader: entry.s
	$(AS) $(ASFLAGS) -o entry.o entry.s
	mv entry.o tmp/entry.o