echo "Building kernel..."

cd kern
# export PATH=/Users/tristanseifert/SquelchenOS/kern/toolchain/gcc/bin:/Users/tristanseifert/SquelchenOS/kern/toolchain/gcc/i686-pc-elf/bin:$PATH
make
cd ..

echo "Assembling hard drive image..."
# Attach disk image
hdiutil attach disk_image.img

# Copy kernel, etc
rm -f /Volumes/MOSQUITOS/kernel.elf
cp kern/kernel.elf /Volumes/MOSQUITOS/kernel.elf

# Clean up OS X's crap
rm -rf /Volumes/MOSQUITOS/.fseventsd
rm -rf /Volumes/MOSQUITOS/.Trashes

# Remove "dot underbar" resource forks
dot_clean -m /Volumes/MOSQUITOS/

# Unmount disk image
diskutil unmount /Volumes/MOSQUITOS/

clear

# -monitor telnet::4444,server
qemu-1.60/qemu-system-i386 -hda /Users/tristanseifert/SquelchenOS/disk_image.img -m 128M -soundhw ac97 -net nic,model=e1000 -net user -device sysbus-ohci -device usb-ehci -serial stdio -device vmware-svga -s
