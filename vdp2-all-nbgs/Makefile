ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/build.pre.mk

SATCONV:= $(YAUL_INSTALL_ROOT)/bin/satconv$(EXE_EXT)

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

SH_CFLAGS+= -Os -I.
SH_LDFLAGS+=

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

assets/NBG0.CPD: work/tiles.bmp
	$(ECHO)cd work; $(SATCONV) assets.txt
	$(ECHO)python work/generate_cpd_pal.py work/tiles.tle assets/NBG0.CPD assets/NBG0.PAL
	$(ECHO)rm -f work/tiles.tle

assets/NBG0.PAL: assets/NBG0.CPD

assets/NBG0.PND: work/nbg0.tmx
	$(ECHO)cd work; $(SATCONV) nbg0.assets.txt
	$(ECHO)python work/generate_pnd.py work/nbg0.map $@
	$(ECHO)rm -f work/nbg0.map

assets/NBG1.PND: work/nbg1.tmx
	$(ECHO)cd work; $(SATCONV) nbg1.assets.txt
	$(ECHO)python work/generate_pnd.py work/nbg1.map $@
	$(ECHO)rm -f work/nbg1.map

assets/NBG2.PND: work/nbg2.tmx
	$(ECHO)cd work; $(SATCONV) nbg2.assets.txt
	$(ECHO)python work/generate_pnd.py work/nbg2.map $@
	$(ECHO)rm -f work/nbg2.map

assets/NBG3.PND: work/nbg3.tmx
	$(ECHO)cd work; $(SATCONV) nbg3.assets.txt
	$(ECHO)python work/generate_pnd.py work/nbg3.map $@
	$(ECHO)rm -f work/nbg3.map

.PHONY: .clean-assets
.clean-assets:
	$(ECHO)rm -f \
	    assets/NBG0.CPD \
	    assets/NBG0.PND \
	    assets/NBG1.PND \
	    assets/NBG2.PND \
	    assets/NBG3.PND \
	    assets/NBG0.PAL

clean: .clean-assets
