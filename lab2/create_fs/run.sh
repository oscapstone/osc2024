cd rootfs
find . | cpio -o -H newc > ../initramfs.cpio
cd ..
mv ./initramfs.cpio ../kernel/