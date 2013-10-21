CC=i686-pc-elf-gcc
AS=i686-pc-elf-as

# Freestanding binary, strip extra shits
CFLAGS= -c -funroll-loops -finline-functions -g -Wall -fno-builtin -static\
		-mno-sse2\
		-nostartfiles -nodefaultlibs -std=c99 -Wno-unused-variable \
		-mfpmath=387 -fno-stack-protector -I/Users/tristanseifert/SquelchenOS/kern/\
		-Wno-unused-function

all: runtime drivers io system loader kernel

# Clean up from previous build
clean:
	rm -f *.o

# Compile main.c into flat binary
kernel: main.c
	$(CC) $(CFLAGS) -ffreestanding *.c
	i686-pc-elf-gcc -Wl,-T,kern.ld -o kernel.elf -ffreestanding -O2 -nostdlib *.o 
	gobjcopy -I elf32-i386 -O binary kernel.elf kernel.bin
	gobjdump -b elf32-i386 -D kernel.elf > symbols.txt
	rm -f *.o

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
system: sys/*.c sys/*.S
	$(CC) $(CFLAGS) -ffreestanding sys/*.c
	$(AS) -o irq_handlers.o sys/irq_handlers.S

# Assembly loader
loader: entry.s
	$(AS) -o entry.o entry.s