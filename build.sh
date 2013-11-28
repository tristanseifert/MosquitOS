echo "Building first stage bootloader..."

nasm -f bin -i./loader/ -o loader/stage1.bin -l loader/stage1.lst loader/stage1.asm
nasm -f bin -i./loader/stage2/ -o loader/stage2/stage2.bin -l loader/stage2/stage2.lst loader/stage2/stage2.asm

echo "Building kernel..."

cd kern
# export PATH=/Users/tristanseifert/SquelchenOS/kern/toolchain/gcc/bin:/Users/tristanseifert/SquelchenOS/kern/toolchain/gcc/i686-pc-elf/bin:$PATH
make
cd ..

echo "Assembling hard drive image..."

# Inject the first and second stage bootloaders
dd conv=notrunc if=loader/stage1.bin of=disk_image.img bs=512 seek=0
dd conv=notrunc if=loader/stage2/stage2.bin of=disk_image.img bs=512 seek=1 count=30

# Attach disk image
hdiutil attach disk_image.img

# Copy kernel, etc
rm -f /Volumes/MOSQUITOS/KERNEL.BIN
cp kern/kernel.bin /Volumes/MOSQUITOS/KERNEL.BIN

# Clean up OS X's crap
rm -rv /Volumes/MOSQUITOS/.fseventsd
rm -rv /Volumes/MOSQUITOS/.Trashes

# Remove "dot underbar" resource forks
dot_clean -m /Volumes/MOSQUITOS/

# Unmount disk image
diskutil unmount /Volumes/MOSQUITOS/

clear

qemu-1.60/qemu-system-i386 -hda /Users/tristanseifert/SquelchenOS/disk_image.img -m 128M -soundhw ac97 -net nic,model=e1000 -net user -device sysbus-ohci -device usb-ehci -monitor stdio -device vmware-svga -s
