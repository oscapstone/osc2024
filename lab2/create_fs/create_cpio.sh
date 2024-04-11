#!/bin/sh

# Go to rootfs folder
# Select all files and put into archives
# '-o, --create' Run in copy-out mode
# '-H FORMAT' archive format, 
#    newc: SVR4 portable format
# 創建一個CPIO歸檔格式 用於打包目錄與文件 包含rootfs目錄與其中的所有內容
# cpio 是UNIX作業系統的一個檔案備份程式及檔案格式
cd rootfs 
find . | cpio -o -H newc > ../initramfs.cpio 
# find 列出rootfs底下的所有文件
# 並將這些傳給cpio命令
cd ..
