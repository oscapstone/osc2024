.PHONY: all clean


USER_PROC_DIR := user_proc
_USER_PROC_IMG := user_proc.img
USER_PROC_IMG := rootfs/$(_USER_PROC_IMG)
HELLO_FILE := rootfs/Hello
WORLD_FILE := rootfs/World
SYSTEM_FILES :=  $(HELLO_FILE) $(WORLD_FILE) $(USER_PROC_IMG)
SUBDIRS := $(USER_PROC_DIR)



all: ../initramfs.cpio

../initramfs.cpio: $(SUBDIRS) $(SYSTEM_FILES)
	cd rootfs && find . | cpio -o -H newc > ../../initramfs.cpio

rootfs:
	mkdir -p $(@)

rootfs/Hello: rootfs
	touch $@
	echo "Hello" > $@

rootfs/World: rootfs
	touch $@
	echo "W\no\nr\nl\nd" > $@

$(SUBDIRS):
	$(MAKE) -C $@

$(USER_PROC_IMG): rootfs $(USER_PROC_DIR)/$(_USER_PROC_IMG)
	cp $(USER_PROC_DIR)/$(_USER_PROC_IMG) rootfs/

$(USER_PROC_DIR)/$(_USER_PROC_IMG):
	@$(MAKE) -C $(@D)

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -rf rootfs
	rm ../initramfs.cpio