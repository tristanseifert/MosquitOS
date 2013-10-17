CC=gcc

# Freestanding binary, strip extra shits
CFLAGS= -c -Os -Oz -funroll-loops -finline-functions -arch i386 -g -Wmost -fno-builtin -static\
		-fomit-frame-pointer -mpreferred-stack-boundary=2 -fno-align-functions -mno-sse2\
		-nostartfiles -nodefaultlibs -std=c99 -Wno-unused-variable \
		-mfpmath=387 -fno-stack-protector -I/Users/tristanseifert/SquelchenOS/kern/

all: runtime drivers io sys loader kernel

# Compile main.c into flat binary
kernel: main.c
	$(CC) $(CFLAGS) -ffreestanding *.c
	gcc -static -Wl,-preload -Wl,-e,_loader -Wl,-segaddr,__TEXT,3000 -ffreestanding -nostdlib -arch i386 -o kernel.o -Wl,-segalign,20 *.o
	gobjcopy -O binary kernel.o kernel.bin
	rm *.o

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
sys: sys/*.c sys/*.s
	$(CC) $(CFLAGS) -ffreestanding sys/*.c sys/*.s

# Assembly loader
loader: entry.s
	$(CC) $(CFLAGS) *.s