ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/build.pre.mk

# Required for library usage
include $(YAUL_INSTALL_ROOT)/share/build.menu.mk

# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS+= \
	assets/NBG0.CPD;asset_nbg0_cpd \
	assets/NBG0.PND;asset_nbg0_pnd \
	assets/NBG0.PAL;asset_nbg0_pal

SH_PROGRAM:= vdp2-effect-tunnel
SH_SRCS:= \
	vdp2-effect-tunnel.c

SH_CFLAGS+= -Os -I. $(MENU_CFLAGS)
SH_LDFLAGS+= $(MENU_LDFLAGS)

IP_VERSION:= V0.001
IP_RELEASE_DATE:= 20180214
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= VDP2 effect tunnel
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000
IP_1ST_READ_SIZE:= 0

include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk
