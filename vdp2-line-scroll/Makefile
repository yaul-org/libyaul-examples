ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/pre.common.mk

# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS+= \
	assets/LINE_SCROLL.TBL;asset_line_scroll_tbl \
	assets/VF.CPD;asset_vf_cpd \
	assets/VF.PND;asset_vf_pnd \
	assets/VF.PAL;asset_vf_pal

SH_PROGRAM:= vdp2-line-scroll
SH_SRCS:= \
	vdp2-line-scroll.c

SH_LIBRARIES:=
SH_CFLAGS+= -O2 -I. -save-temps=obj

IP_VERSION:= V0.001
IP_RELEASE_DATE:= 20180214
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= VDP2 RBG0
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000

include ../shared/assets.mk

include $(YAUL_INSTALL_ROOT)/share/post.common.mk
