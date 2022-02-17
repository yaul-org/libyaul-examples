ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/pre.common.mk

# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS+= \
	assets/BITMAP.CPD;asset_bitmap_cpd \
	assets/BITMAP.PAL;asset_bitmap_pal

SH_PROGRAM:= vdp2-reduction-bitmap
SH_SRCS:= \
	vdp2-reduction-bitmap.c

SH_LIBRARIES:= tga
SH_CFLAGS+= -O2 -I. -save-temps=obj

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= VDP2 reduction bitmap
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000

include ../shared/assets.mk

include $(YAUL_INSTALL_ROOT)/share/post.common.mk
