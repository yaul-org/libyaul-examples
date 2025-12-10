ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/build.pre.mk

# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS+= \
	assets/adp68k.bin;asset_sound_driver \
	assets/ouch.adp;asset_ouch_adp \
	assets/short.adp;asset_short_adp

SH_PROGRAM:= scsp-adpcm
SH_SRCS:= \
	scsp-adpcm.c \
	adpcm.c

SH_CFLAGS+= -Os -I. $(MENU_CFLAGS)
SH_LDFLAGS+= $(MENU_LDFLAGS)

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= SCSP ADPCM Test
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000
IP_1ST_READ_SIZE:= 0

include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk
