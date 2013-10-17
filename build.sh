echo "Building Bootloader..."

nasm -f bin -i./loader/ -o loader/stage1.bin -l loader/stage1.lst loader/stage1.asm
nasm -f bin -i./loader/ -o loader/stage2.bin -l loader/stage2.lst loader/stage2.asm

echo "Building kernel..."

cd kern
# export PATH=$PATH:/Users/tristanseifert/SquelchenOS/kern/toolchain/gcc/bin
make
cd ..

echo "Assembling hard drive image..."

# Inject the first and second stage bootloaders
dd conv=notrunc if=loader/stage1.bin of=disk_image.img bs=512 seek=0
dd conv=notrunc if=loader/stage2.bin of=disk_image.img bs=512 seek=1 count=30

# Attach disk image
hdiutil attach disk_image.img

# Copy kernel, etc
cp kern/kernel.bin /Volumes/MOSQUITOS/KERNEL.BIN

# Unmount disk image
umount /Volumes/MOSQUITOS/

qemu-1.60/qemu-system-i386 -hda /Users/tristanseifert/SquelchenOS/disk_image.img -boot c -m 32M -soundhw adlib -vga std -s