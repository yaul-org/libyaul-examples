ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/pre.common.mk

# plane.s3d.o
# cube.s3d.o
# sms.s3d.o
# quake.s3d.o
# torus.s3d.o
# quake_split.s3d.o
# sonic.s3d.o

SH_PROGRAM:= vdp1-sega3d
SH_OBJECTS:= vdp1-sega3d.o textures.tex.o $(MODEL_SH_OBJECT).s3d.o

# Important, libsega3d must be included
SH_LIBRARIES:= sega3d
SH_CFLAGS+= -O2 -I. -save-temps

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= SEGA3D demo
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001000
IP_1ST_READ_ADDR:= 0x06004000

M68K_PROGRAM:=
M68K_OBJECTS:=

include $(YAUL_INSTALL_ROOT)/share/post.common.mk
