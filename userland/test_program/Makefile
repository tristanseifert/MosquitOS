CC=i686-pc-elf-gcc
AS=i686-pc-elf-as
LD=i686-pc-elf-ld

LIBCDIR= ../squlibc

# Freestanding binary, strip extra shits
CFLAGS= -c -finline-functions -g -Wall -fno-builtin -static\
		-mno-sse2 -O2\
		-nostartfiles -nodefaultlibs -std=c99 -Wno-unused-variable \
		-mfpmath=387 -fno-stack-protector -I. -I./$(LIBCDIR)/includes/\
		-Wno-unused-function

LDFLAGS= -nostdlib -nostartfiles -static -Wl,-T,userspace.ld -O2

STARTFILES = $(LIBCDIR)/lib/crt0.o $(LIBCDIR)/lib/crti.o
#ENDFILES = $(LIBCDIR)/lib/crtend.o $(LIBCDIR)/lib/crtn.o
LIBGROUP = -Wl,--start-group $(LIBCDIR)/lib/libc.a -lgcc -Wl,--end-group

all: test.c
	$(CC) $(CFLAGS) test.c
	i686-pc-elf-gcc $(LDFLAGS) -o test.elf $(STARTFILES) *.o $(LIBGROUP)

# Clean up from previous build
clean:
	rm -rf tmp
	rm -f *.o *.elf