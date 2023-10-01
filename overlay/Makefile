THIS_ROOT:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/build.pre.mk

# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS=

SH_PROGRAM:= overlay

SH_SRCS:= \
	program/program.c \
\
	overlay1/overlay1.c \
	overlay1/foo.c \
\
	overlay2/overlay2.c \
	overlay2/foo.c \
\
	overlay3/overlay3.c

SH_SPECS:= $(THIS_ROOT)/overlay.specs

SH_CFLAGS+= -g -Os -I. -DDEBUG
SH_LDFLAGS+=

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= Overlay example
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000
IP_1ST_READ_SIZE:= 0

# We need to depend on any changes made to $(THIS_ROOT)/overlay.specs in case
# there is a change in which linkerscript is used
$(SH_BUILD_PATH)/overlays.x: overlays.lst $(THIS_ROOT)/overlay.specs
	@printf -- "$(V_BEGIN_YELLOW)$(@F)$(V_END)\n"
	$(ECHO)cat $< | awk -f tools/generate-ldscript.awk > $@

$(SH_BUILD_PATH)/$(SH_PROGRAM).elf: $(SH_BUILD_PATH)/overlays.x

$(SH_BUILD_PATH)/ovl1.elf: $(SH_BUILD_PATH)/$(SH_PROGRAM).elf
	$(YAUL_INSTALL_ROOT)/share/wrap-error $(SH_OBJCOPY) -O elf32-sh -j ".overlay1" -j ".overlay1.*" $< $@

cd/OVL1.BIN: $(SH_BUILD_PATH)/ovl1.elf
	$(YAUL_INSTALL_ROOT)/share/wrap-error $(SH_OBJCOPY) -O binary $< $@

$(SH_BUILD_PATH)/ovl2.elf: $(SH_BUILD_PATH)/$(SH_PROGRAM).elf
	$(YAUL_INSTALL_ROOT)/share/wrap-error $(SH_OBJCOPY) -O elf32-sh -j ".overlay2" -j ".overlay2.*" $< $@

cd/OVL2.BIN: $(SH_BUILD_PATH)/ovl2.elf
	$(YAUL_INSTALL_ROOT)/share/wrap-error $(SH_OBJCOPY) -O binary $< $@

$(SH_BUILD_PATH)/ovl3.elf: $(SH_BUILD_PATH)/$(SH_PROGRAM).elf
	$(YAUL_INSTALL_ROOT)/share/wrap-error $(SH_OBJCOPY) -O elf32-sh -j ".overlay3" -j ".overlay3.*" $< $@

cd/OVL3.BIN: $(SH_BUILD_PATH)/ovl3.elf
	$(YAUL_INSTALL_ROOT)/share/wrap-error $(SH_OBJCOPY) -O binary $< $@

pre-build-iso: cd/OVL1.BIN cd/OVL2.BIN cd/OVL3.BIN

# This is a hack:
#   Overwrite the executable copied to cd/. Doing it this way allows us to avoid
#   repeating the same target more than once after a complete build
post-build-iso:
	$(YAUL_INSTALL_ROOT)/share/wrap-error $(SH_OBJCOPY) -O binary -R ".overlay1" -R ".overlay2" -R ".overlay3"  -R ".overlay1.*" -R ".overlay2.*" -R ".overlay3.*" $(SH_BUILD_PATH)/$(SH_PROGRAM).elf cd/A.BIN

.PHONY: .clean-overlays.elfs
.clean-overlays.elfs:
	@$(RM) -f \
	    $(SH_BUILD_PATH)/ovl1.elf \
	    $(SH_BUILD_PATH)/ovl2.elf \
	    $(SH_BUILD_PATH)/ovl3.elf

.PHONY: .clean-overlays.x
.clean-overlays.x:
	@$(RM) -f $(SH_BUILD_PATH)/overlays.x

clean: .clean-overlays.elfs
clean: .clean-overlays.x

# Build the executable only
include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk
