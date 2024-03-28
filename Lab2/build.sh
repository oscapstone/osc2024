echo -e "Building Bootloader image...\n"
cd bootloader
make all

echo -e "\nBuilding Kernel image...\n"
cd ../kernel
make all

echo -e  "\nArchive rootfs..."
cd ../rootfs
find . | cpio -o -H newc > ../initramfs.cpio

cd ..