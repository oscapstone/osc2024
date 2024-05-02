echo -e "Building Bootloader image...\n"
cd bootloader
make all

echo -e "\nBuilding Kernel Image...\n"
cd ../kernel
make all

echo -e "\nBuilding User Program...\n"
cd ../user_prog
make all

echo -e  "\nArchive rootfs..."
cd ../rootfs
echo -e "\nRoot File system"
ls ./
find . | cpio -o -H newc > ../initramfs.cpio

cd ..