ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/pre.common.mk

# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS+= \
	assets/CPD.BIN;asset_cpd \
	assets/PND.BIN;asset_pnd \
	assets/PAL.BIN;asset_pal

SH_PROGRAM:= vdp2-rbg0
SH_SRCS:= \
	vdp2-rbg0.c

SH_LIBRARIES:=
SH_CFLAGS+= -Os -I.

IP_VERSION:= V0.001
IP_RELEASE_DATE:= 20180214
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= VDP2 RBG0
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000

include $(YAUL_INSTALL_ROOT)/share/post.common.mk
