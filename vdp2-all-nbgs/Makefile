ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/build.pre.mk

# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS+= \
	assets/NBG0.CPD;asset_cpd \
	assets/NBG0.PND;asset_nbg0_pnd \
	assets/NBG1.PND;asset_nbg1_pnd \
	assets/NBG2.PND;asset_nbg2_pnd \
	assets/NBG3.PND;asset_nbg3_pnd \
	assets/NBG0.PAL;asset_pal \

SH_PROGRAM:= vdp2-all-nbgs
SH_SRCS:= \
	vdp2-all-nbgs.c

SH_LIBRARIES:=
SH_CFLAGS+= -Os -I.

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= VDP2 all NBGs
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000
IP_1ST_READ_SIZE:= 0

include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk
