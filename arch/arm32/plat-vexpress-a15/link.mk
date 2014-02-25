LINK_SCRIPT = $(ARCH_DIR)/kern/kern.ld

AWK	 = awk

all: $(out-dir)kern.elf $(out-dir)kern.dmp
all: $(out-dir)kern.mem_usage $(out-dir)kern.symb_sizes
CLEANFILES += $(out-dir)kern.elf $(out-dir)kern.dmp $(out-dir)kern.map
CLEANFILES += $(out-dir)kern.mem_usage $(out-dir)kern.symb_sizes

LDFLAGS += -e _start -T $(LINK_SCRIPT) -Map=$(out-dir)kern.map
LDFLAGS += --sort-section=alignment

$(out-dir)kern.elf: $(OBJS)
	@echo LD $@
	${Q}$(LD) $(LDFLAGS) $(OBJS) $(LIBGCC) -o $@

$(out-dir)kern.dmp: $(out-dir)kern.elf
	@echo OBJDUMP $@
	${Q}$(OBJDUMP) -l -x -d $< > $@

$(out-dir)kern.mem_usage: $(out-dir)kern.elf
	@echo Mem usage $@
	$(Q)${READELF} -a -W $< | ${AWK} -f ./scripts/mem_usage.awk > $@

$(out-dir)kern.symb_sizes: $(out-dir)kern.elf
	@echo Symb sizes $@
	$(Q)${READELF} -a -W $< | ${AWK} -f ./scripts/symb_sizes.awk > $@
