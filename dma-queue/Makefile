ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/pre.common.mk

# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS+= \
	assets/BITMAP1.TGA;asset_bitmap1_tga \
	assets/BITMAP2.TGA;asset_bitmap2_tga \
	assets/BITMAP3.TGA;asset_bitmap3_tga \
	assets/BITMAP4.TGA;asset_bitmap4_tga \
	assets/BITMAP5.TGA;asset_bitmap5_tga \
	assets/BITMAP6.TGA;asset_bitmap6_tga \
	assets/BITMAP7.TGA;asset_bitmap7_tga \
	assets/BITMAP8.TGA;asset_bitmap8_tga \
	assets/BITMAP9.TGA;asset_bitmap9_tga

SH_PROGRAM:= dma-queue
SH_SRCS:= \
	dma-queue.c

SH_LIBRARIES:= tga
SH_CFLAGS+= -O2

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= DMA queue example
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000

include $(YAUL_INSTALL_ROOT)/share/post.common.mk
