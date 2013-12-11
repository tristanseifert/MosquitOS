echo "Building kernel..."

cd kern
make

# Get return value of make
OUT=$?
if [ $OUT -eq 0 ];then
	echo "Build succeeded"
else
	make clean >> /dev/null
	exit
fi

cd ..

echo "Assembling hard drive image..."
# Attach disk image
hdiutil attach disk_image.img

# Copy kernel, etc
rm -f /Volumes/MosquitOS/kernel.elf
cp kern/kernel.elf /Volumes/MosquitOS/kernel.elf

# Clean up OS X's crap
rm -rf /Volumes/MosquitOS/.fseventsd
rm -rf /Volumes/MosquitOS/.Trashes

# Remove "dot underbar" resource forks
dot_clean -m /Volumes/MosquitOS/

# Unmount disk image
# diskutil unmount /Volumes/MosquitOS/

clear

# -monitor telnet::4444,server
qemu-1.60/qemu-system-i386 -hda /Users/tristanseifert/SquelchenOS/disk_image.img -m 384M -soundhw ac97 -net nic,model=e1000 -net user -device sysbus-ohci -device usb-ehci -serial stdio -device vmware-svga -smbios type=1,manufacturer=NoEuh,product=Computermajig -s
