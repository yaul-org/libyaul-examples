ifeq ($(strip $(INSTALL_ROOT)),)
  $(error Undefined INSTALL_ROOT (install root directory))
endif

include $(INSTALL_ROOT)/share/pre.common.mk

SH_PROGRAM:= sprite-priority
SH_OBJECTS:= \
	root.romdisk.o \
	fs.o \
	bg.o \
\
	game.o \

SH_CFLAGS+= -I.
SH_LIBRARIES:=

IP_VERSION:= V0.001
IP_RELEASE_DATE:= 20180214
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= Sprite priority
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06002000
IP_1ST_READ_ADDR:= 0x06004000

include $(INSTALL_ROOT)/share/post.common.mk
