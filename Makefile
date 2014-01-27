SHELL = /bin/bash

.PHONY: all
all:

# Make these default for now
ARCH            ?= arm32
PLATFORM        ?= vexpress-a15
O		?= out
include arch/$(ARCH)/plat-$(PLATFORM)/conf.mk


ifneq ($O,)
out-dir := $O/
endif

ifneq ($V,1)
Q := @
endif


# Start processing with the top subdirectories
SUBDIRS += drivers kern libc
include mk/subdir.mk

include mk/compile.mk

include arch/$(ARCH)/plat-$(PLATFORM)/link.mk

.PHONY: clean
clean:
	@echo Cleaning
	${Q}rm -f $(CLEANFILES)

.PHONY: cscope
cscope:
	@echo Creating cscope database
	@rm -f cscope.*
	@find $(PWD) -name "*.[chSs]" > cscope.files
	@cscope -b -q
