ifeq ($(strip $(INSTALL_ROOT)),)
  $(error Undefined INSTALL_ROOT (install root directory))
endif

include $(INSTALL_ROOT)/share/pre.common.mk

SH_PROGRAM:= scu-dsp
SH_OBJECTS:= \
	scu-dsp.o

SH_LIBRARIES:=
SH_CFLAGS+= -I. -save-temps

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= SCU DSP test
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06002000
IP_1ST_READ_ADDR:= 0x06004000

M68K_PROGRAM:= 
M68K_OBJECTS:=

include $(INSTALL_ROOT)/share/post.common.mk
