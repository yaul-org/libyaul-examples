ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/pre.common.mk

# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS+= \
	assets/CPD.DAT;asset_cpd_dat \
	assets/PND.DAT;asset_pnd_dat \
	assets/PAL.DAT;asset_pal_dat

SH_PROGRAM:= vdp2-special-function
SH_SRCS:= \
	vdp2-special-function.c

SH_LIBRARIES:=
SH_CFLAGS+= -O2 -I. -save-temps=obj

IP_VERSION:= V0.001
IP_RELEASE_DATE:= 20180214
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= VDP2 special function
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000

include $(YAUL_INSTALL_ROOT)/share/post.common.mk
