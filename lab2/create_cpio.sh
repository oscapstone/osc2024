#!/bin/sh

# Make rootfs folder
# Select all files and put into archives
# '-o, --create' Run in copy-out mode
# '-H FORMAT' archive format, 
#    newc: SVR4 portable format

mkdir -p rootfs

cat <<EOL > rootfs/Hello
Hello
EOL

cat <<EOL > rootfs/World
W
o
r
l
d
EOL

cd rootfs
find . | cpio -o -H newc > ../initramfs.cpio
cd ..

rm -rf rootfs