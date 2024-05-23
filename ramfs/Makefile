OUTPUT := initramfs.cpio

SOURCES := $(wildcard rootfs/*)

$(OUTPUT): $(SOURCES)
	cd rootfs && find . | sed 's|^\./||' |cpio -o -H newc > ../$@

.PHONY: clean
clean:
	rm -f $(OUTPUT)