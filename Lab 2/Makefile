ROOT	 = $(PWD)
SUBDIR 	 = bootloader
SUBDIR	+= kernel

OUT_DIR  = out


define make_subdir
@for i in $(SUBDIR); do	\
	( cd $$i && make $1 ) \
done;
endef

all: clean
	$(call make_subdir)

clean:
	rm -rf $(OUT_DIR) *.img 
	$(call make_subdir, clean)