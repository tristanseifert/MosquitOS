CC=i686-pc-elf-gcc
AS=i686-pc-elf-as
LD=i686-pc-elf-ld

CFLAGS= -c -g -Wall -fno-builtin -static -mno-sse2 -O2\
		-nostartfiles -nodefaultlibs -std=c99 -Wno-unused-variable \
		-mfpmath=387 -fno-stack-protector -I. -I./includes/ \
		-Wno-unused-function -Wno-implicit-function-declaration \
		-fvisibility=default

ASFLAGS =

# This is a sort of hack to combine a bunch of .o files into one bigger .o, which
# makes it easier to deal with the code
make_obj_archive= \
	$(LD) -r *.o -o $@.oa; \
	rm -f *.o \

all: libc_string libc_io libc_syscalls libc_link libc_crts clean

# Links everything into a single static library
libc_link: libc_string libc_io libc_syscalls
	rm lib/libc.a
	ar rcs lib/libc.a *.oa
	nm lib/libc.a | grep "T " > lib/libc.txt

# Various modules in the C library
libc_string: string/*.c
	$(CC) $(CFLAGS) string/*.c
	$(make_obj_archive)

libc_io: io/*.c
	$(CC) $(CFLAGS) io/*.c
	$(make_obj_archive)

libc_syscalls: syscall/*.c
	$(CC) $(CFLAGS) syscall/*.c
	$(make_obj_archive)

# Builds the various object files containing C runtime setup code, such as
# crt1.o.
libc_crts: crt/*.S
	$(AS) $(ASFLAGS) -o lib/crt0.o crt/crt0.S
	$(AS) $(ASFLAGS) -o lib/crti.o crt/crti.S
	$(AS) $(ASFLAGS) -o lib/crtend.o crt/crtend.S
	$(AS) $(ASFLAGS) -o lib/crtn.o crt/crtn.S
# Clean up from previous build
clean:
	rm -f *.o *.oa