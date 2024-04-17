echo -e "Building Bootloader image...\n"
cd bootloader
make bootloader

echo -e "\nBuilding Kernel Image...\n"
cd ../kernel
make kernel

echo -e "\nBuilding User Program...\n"
cd ../user_prog
make user_prog

echo -e  "\nArchive rootfs..."
cd ../rootfs
echo -e "\nRoot File system"
ls ./
find . | cpio -o -H newc > ../initramfs.cpio

cd ..