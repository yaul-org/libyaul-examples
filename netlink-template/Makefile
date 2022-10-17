THIS_ROOT:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

SH_SPECS:= $(THIS_ROOT)/yaul-netlink.specs

include $(YAUL_INSTALL_ROOT)/share/pre.common.mk

SH_PROGRAM:= netlink-template
SH_SRCS:= \
	netlink-template.c
ROMDISK_SYMBOLS=
ROMDISK_DIRS=

SH_LIBRARIES:=
SH_CFLAGS+= -Os -I.

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= Netlink template
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06005300

include $(YAUL_INSTALL_ROOT)/share/post.common.mk
