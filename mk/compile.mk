# Disable all builtin rules
.SUFFIXES:

CC	= $(CROSS_COMPILE)gcc
LD	= $(CROSS_COMPILE)ld
AR	= $(CROSS_COMPILE)ar
NM	= $(CROSS_COMPILE)nm
OBJCOPY	= $(CROSS_COMPILE)objcopy
OBJDUMP	= $(CROSS_COMPILE)objdump
READELF = $(CROSS_COMPILE)readelf

NOSTDINC := -nostdinc -isystem $(shell $(CC) -print-file-name=include)
CPPFLAGS += $(NOSTDINC) -Iinclude

CPPFLAGS	+= $(PLATFORM_CPPFLAGS)
CFLAGS		+= $(PLATFORM_CFLAGS) $(CFLAGS_WARNS)
SFLAGS		+= $(PLATFORM_SFLAGS)

# Get location of libgcc from gcc
LIBGCC  := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)


ifndef NOWERROR
CFLAGS+= -Werror
endif
CFLAGS  += -fdiagnostics-show-option
CFLAGS_WARNS_1+= $(CFLAGS_HIGH)
CFLAGS_WARNS_2+= $(CFLAGS_WARNS_1) $(CFLAGS_MEDIUM)
CFLAGS_WARNS_3+= $(CFLAGS_WARNS_2) $(CFLAGS_LOW)

CFLAGS_HIGH = -Wall -Wcast-align \
		-Werror-implicit-function-declaration -Wextra -Wfloat-equal \
		-Wformat-nonliteral -Wformat-security -Wformat=2 -Winit-self \
		-Wmissing-declarations -Wmissing-format-attribute \
		-Wmissing-include-dirs -Wmissing-noreturn \
		-Wmissing-prototypes -Wnested-externs -Wpointer-arith \
		-Wshadow -Wstrict-prototypes -Wswitch-default \
		-Wwrite-strings
CFLAGS_MEDIUM = -Waggregate-return \
		-Wredundant-decls
CFLAGS_LOW = -Winline -Wno-missing-field-initializers -Wno-unused-parameter \
		-Wold-style-definition -Wstrict-aliasing=2 \
		-Wundef -pedantic -std=gnu99 -Wno-format-zero-length \
		-Wdeclaration-after-statement

CFLAGS_WARNS+= $(CFLAGS_WARNS_$(WARNS))

WARNS ?= 3

define SRCS_template
OBJ_$1	 = $$(out-dir)$$(basename $1).o
OBJS	+= $$(OBJ_$1)
DEP_$1	 = $$(dir $$(OBJ_$1)).$$(notdir $$(OBJ_$1)).d
TDEP_$1	 = $$(DEP_$1).tmp
DEPS	+= $$(DEP_$1)

ifeq ($(filter %.c,$1),$1)
FLAGS_$1 = $(filter-out $$(CFLAGS_REMOVE) $$(CFLAGS_REMOVE_$1), \
		$$(CFLAGS) $$(CFLAGS_$1))
else ifeq ($$(filter %.S,$1),$1)
FLAGS_$1 = -DASM=1 $(filter-out $$(SFLAGS_REMOVE) $$(SFLAGS_REMOVE_$1), \
		$$(SFLAGS) $$(SFLAGS_$1))
else
$$(error "Don't know what to do with $1")
endif

FLAGS_$1 += -Wp,-MD,$$(TDEP_$1) \
	   $(filter-out $$(CPPFLAGS_REMOVE) $$(CPPFLAGS_REMOVE_$1), \
		$$(CPPFLAGS) $$(CPPFLAGS_$1))
$$(OBJ_$1): $1
	@mkdir -p $$(dir $$(OBJ_$1))
	@echo CC $1
	$(Q)$$(CC) $$(FLAGS_$1) -c $$< -o $$@
	@./scripts/fixdep $$(dir $$(OBJ_$1)) < $$(TDEP_$1) > $$(DEP_$1)
	@rm -f $$(TDEP_$1)

endef

$(foreach f, $(SRCS), $(eval $(call SRCS_template,$(f))))

CLEANFILES += $(OBJS) $(DEPS)

-include $(DEPS)
