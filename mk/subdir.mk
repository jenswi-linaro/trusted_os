SRCS :=

define subdir-srcs-y-template
fname				:= $$(SUBDIR)/$1
SRCS 				+= $$(fname)
CFLAGS_$$(fname) 		:= $$(cflags-y) $$(cflags_$(1)-y)
CFLAGS_REMOVE_$$(fname) 	:= $$(cflags_remove-y) \
					$$(cflags_remove_$(1)-y)
CPPFLAGS_$$(fname) 		:= $$(cppflags-y) $$(cppflags_$(1)-y)
CPPFLAGS_REMOVE_$$(fname) 	:= $$(cppflags_remove-y) \
					$$(cppflags_remove_$(1)-y)
SFLAGS_$$(fname) 		:= $$(sflags-y) $$(sflags_$(1)-y)
SFLAGS_REMOVE_$$(fname) 	:= $$(sflags_remove-y) \
					$$(sflags_remove_$(1)-y)
# Clear local filename specific variables to avoid accidental reuse
# in another subdirectory
cflags_$(1)-y 			:=
cflags_remove_$(1)-y		:=
cppflags_$(1)-y			:=
cppflags_remove_$(1)-y		:=
sflags_$(1)-y 			:=
sflags_remove_$(1)-y		:=
endef #subdir-srcs-y-template

define subdir-template
SUBDIR := $1
include $1/sub.mk
subdirs := $$(addprefix $1/,$$(subdirs-y))

# Process files in current directory
$$(foreach s, $$(srcs-y), $$(eval $$(call subdir-srcs-y-template,$$(s))))
# Clear flags used when processing current directory
srcs-y :=
cflags-y :=
cppflags-y :=
sflags-y :=
cflags_remove-y :=
subdirs-y :=

# Process subdirectories in current directory
$$(foreach sd, $$(subdirs), $$(eval $$(call subdir-template,$$(sd))))
endef #subdir-template

# Top subdirectories
$(foreach sd, $(SUBDIRS), $(eval $(call subdir-template,$(sd))))
