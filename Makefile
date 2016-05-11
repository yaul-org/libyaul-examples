ifeq ($(strip $(INSTALL_ROOT)),)
  $(error Undefined INSTALL_ROOT (install root directory))
endif

include $(INSTALL_ROOT)/share/pre.common.mk

SH_PROGRAM:= blue
SH_OBJECTS:= \
	root.romdisk.o \
\
	engine/component/camera.o \
	engine/component/collider.o \
	engine/component/rigid_body.o \
	engine/component/sprite.o \
	engine/component/transform.o \
	engine/engine.o \
	engine/fs.o \
	engine/matrix_stack.o \
	engine/object.o \
	engine/objects.o \
	engine/physics.o \
	engine/scene.o \
\
	blue.o \
\
	scene/splash.o \
	scene/title.o \
	scene/game.o \
\
	object/blue.o \
	object/camera.o \
	object/coin.o \
	object/world.o \
\
	component/coin_mgr.o \
	component/world_mgr.o \
	component/blue_mgr.o \
	component/coin.o \
	component/camera_mgr.o
SH_LIBRARIES:= tga

ifeq ($(strip $(ASSERT)),1)
  SH_CFLAGS+= -DHAVE_ASSERT
endif

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= Blue
IP_MASTER_STACK_ADDR:= 0x06100000
IP_SLAVE_STACK_ADDR:= 0x00000000
IP_1ST_READ_ADDR:= 0x06004000

ROMDISK_DEPS:= \
	worlds/test.json

.SUFFIXES:
.SUFFIXES: $(SUFFIXES) .tmx .json

%.json: %.tmx
	if ! which tiled >/dev/null 2>&1; then \
	    printf -- "Tiled Map Editor (tiled) is required (http://www.mapeditor.org/)\n" >&2; \
	    exit 1; \
	fi
	tiled --export-map "Json files (*.json)" $< $@
	tools/tmx2map
	$(RM) $@

include $(INSTALL_ROOT)/share/post.common.mk
