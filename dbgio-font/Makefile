ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/build.pre.mk

# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS+= \
	assets/mrc_square_16x16.cpd;asset_font_cpd \
	assets/mrc_square_16x16.pal;asset_font_pal

SH_PROGRAM:= dbgio-font
SH_SRCS:= \
	dbgio-font.c

SH_LIBRARIES:=
SH_CFLAGS+= -I. -Os

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= DBGIO font test
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000
IP_1ST_READ_SIZE:= 0

include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk

assets/mrc_square_16x16.cpd: work/mrc_square_16x16.bmp
	@printf -- "$(V_BEGIN_MAGENTA)$@$(V_END)\n"
	$(ECHO)cd work; satconv assets.txt
	$(ECHO)python work/strip_tle.py work/mrc_squa.tle work/mrc_squa.cpd work/mrc_squa.pal
	$(ECHO)python work/4bpp_to_1bpp.py work/mrc_squa.cpd $@
	$(ECHO)rm -f work/mrc_squa.tle
	$(ECHO)rm -f work/mrc_squa.cpd

assets/mrc_square_16x16.pal: assets/mrc_square_16x16.cpd
	@printf -- "$(V_BEGIN_MAGENTA)$@$(V_END)\n"
	$(ECHO)mv work/mrc_squa.pal $@
