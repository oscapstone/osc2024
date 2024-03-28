#!/bin/sh

# 切換到 rootfs 資料夾
# 選擇所有檔案並將其放入歸檔中
# '-o, --create' 執行複製出模式
# '-H FORMAT' 歸檔格式,
#    newc: SVR4 可攜式格式

cd rootfs  # 切換到 rootfs 資料夾
find . | cpio -o -H newc > ../initramfs.cpio  # 使用 find 命令選擇所有檔案並通過 cpio 命令以 newc 格式創建歸檔，然後將其輸出到上級目錄的 initramfs.cpio 檔案中
cd ..  # 返回上級目錄
