LINK_SCRIPT = $(ARCH_DIR)/kern/kern.ld

all: $(out-dir)kern.elf $(out-dir)kern.dmp
CLEANFILES += $(out-dir)kern.elf $(out-dir)kern.dmp $(out-dir)kern.map

LDFLAGS += -e _start -T $(LINK_SCRIPT) -Map=$(out-dir)kern.map

$(out-dir)kern.elf: $(OBJS)
	@echo LD $@
	${Q}$(LD) $(LDFLAGS) $(OBJS) $(LIBGCC) -o $@

$(out-dir)kern.dmp: $(out-dir)kern.elf
	@echo OBJDUMP $@
	${Q}$(OBJDUMP) -l -x -d $< > $@
