THIS_ROOT:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/build.pre.mk

# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS+= \

SH_PROGRAM:= overlay

SH_LDFLAGS:= \
	-Wl,-Map,$(SH_BUILD_PATH)/$(SH_PROGRAM).map

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

SH_LIBRARIES:=
SH_CFLAGS+= -g -Os -I.

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= SCU DSP test
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000
IP_1ST_READ_SIZE:= 0

cd/OVL1.BIN: $(SH_BUILD_PATH)/$(SH_PROGRAM).elf
	$(ECHO)sh2eb-elf-objcopy -O binary -j ".overlay1" $< $@

cd/OVL2.BIN: $(SH_BUILD_PATH)/$(SH_PROGRAM).elf
	$(ECHO)sh2eb-elf-objcopy -O binary -j ".overlay2" $< $@

cd/OVL3.BIN: $(SH_BUILD_PATH)/$(SH_PROGRAM).elf
	$(ECHO)sh2eb-elf-objcopy -O binary -j ".overlay3" $< $@

# This is a hack:
#   1. Strip out the overlays from the program binary
#   2. Create the overlays and copy them
pre-build-iso: cd/OVL1.BIN cd/OVL2.BIN cd/OVL3.BIN
	$(ECHO)rm -f $(SH_BUILD_PATH)/$(SH_PROGRAM).bin
	$(ECHO)sh2eb-elf-objcopy -O binary -R ".overlay1" -R ".overlay2" -R ".overlay3" $(SH_BUILD_PATH)/$(SH_PROGRAM).elf $(SH_BUILD_PATH)/$(SH_PROGRAM).bin

# Build the executable only
include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk
