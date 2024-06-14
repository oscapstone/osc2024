# 變量定義
HELLO_FILE := rootfs/Hello
WORLD_FILE := rootfs/World
MAKEFILE_NAME := makefile Makefile
EXISTING_MAKEFILES := $(foreach name,$(MAKEFILE_NAME),$(wildcard */$(name)))
MAKE_SUBDIRS := $(dir $(EXISTING_MAKEFILES))
# the sets of directories to do various things in
BUILDDIRS = $(MAKE_SUBDIRS:%=build-%)
CLEANDIRS = $(MAKE_SUBDIRS:%=clean-%)

# 找到所有子目錄中的 img 文件
_IMG := $(wildcard *.img) $(wildcard */*.img)
ROOTFS_IMG := $(addprefix rootfs/,$(notdir $(_IMG)))
SYSTEM_FILES := $(HELLO_FILE) $(WORLD_FILE) $(ROOTFS_IMG)

# 所有目標文件 $(BUILDDIRS) gen_cpio
all: $(BUILDDIRS) gen_cpio

# 運行子目錄中的 Makefile
$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%)

# 清理
$(CLEANDIRS): 
	$(MAKE) -C $(@:clean-%=%) clean

clean: $(CLEANDIRS)
	rm -rf rootfs
	rm -f ../initramfs.cpio

gen_cpio:
	$(MAKE) ../initramfs.cpio

# 生成 initramfs.cpio
../initramfs.cpio:  $(SYSTEM_FILES)
	cd rootfs && find . | cpio -o -H newc > ../../initramfs.cpio

# 創建 rootfs 目錄
rootfs:
	mkdir -p $(@)

# 創建 Hello 文件
rootfs/Hello: rootfs
	touch $@
	echo "Hello" > $@

# 創建 World 文件
rootfs/World: rootfs
	touch $@
	echo "W\no\nr\nl\nd" > $@

$(ROOTFS_IMG): $(_IMG)
	cp $^ rootfs


.PHONY: all clean gen_cpio $(BUILDDIRS) $(CLEANDIRS)