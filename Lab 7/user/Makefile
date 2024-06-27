SUBDIR	 = user1

BUILDSUBDIR = $(SUBDIR:%=build-%)
CLEANSUBDIR = $(SUBDIR:%=clean-%)

all: $(BUILDSUBDIR)

$(BUILDSUBDIR):
	@echo "===>" $@
		$(MAKE) -C $(@:build-%=%)
	@echo "<===" $@

$(CLEANSUBDIR):
	@echo "===>" $@
		$(MAKE) -C $(@:clean-%=%) clean
	@echo "<===" $@

clean: $(CLEANSUBDIR)

# $(call make_subdir, clean)

.PHONY: all clean $(SUBDIR) $(BUILDSUBDIR) $(CLEANSUBDIR)