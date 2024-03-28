# SUBDIR 	 = bootloader
SUBDIR	 = kernel bootloader

BUILDSUBDIR = $(SUBDIR:%=build-%)
CLEANSUBDIR = $(SUBDIR:%=clean-%)

SD_DIR   = sd
FS_DIR   = rootfs

all: $(SD_DIR)/initrd.cpio $(BUILDSUBDIR)

$(SD_DIR)/initrd.cpio:
	cd $(SD_DIR)/$(FS_DIR) && ls -a | cpio -o -H newc > ../../$@ && cd ..

$(BUILDSUBDIR):
	@echo "===>" $@
		$(MAKE) -C $(@:build-%=%)
	@echo "<===" $@

$(CLEANSUBDIR):
	@echo "===>" $@
		$(MAKE) -C $(@:clean-%=%) clean
	@echo "<===" $@

clean: $(CLEANSUBDIR)
	rm -f $(SD_DIR)/*.cpio


# $(call make_subdir, clean)

.PHONY: all clean $(SUBDIR) $(BUILDSUBDIR) $(CLEANSUBDIR) $(SD_DIR)/initrd.cpio