ifeq ($(strip $(INSTALL_ROOT)),)
  $(error Undefined INSTALL_ROOT (install root directory))
endif

include $(INSTALL_ROOT)/share/pre.common.mk

SH_PROGRAM:= blue
SH_OBJECTS:= \
	root.romdisk.o \
	engine/collider.o \
	engine/engine.o \
	engine/fs.o \
	engine/matrix_stack.o \
	engine/object.o \
	engine/objects.o \
	engine/particle.o \
	engine/physics.o \
	engine/rigid_body.o \
	engine/scene.o \
	blue.o \
	globals.o \
	scene_splash.o \
	scene_title.o \
	scene_game.o \
	object_camera.o \
	object_world.o \
	object_blue.o

SH_LIBRARIES:= tga

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= Blue
IP_MASTER_STACK_ADDR:= 0x06100000
IP_SLAVE_STACK_ADDR:= 0x00000000
IP_1ST_READ_ADDR:= 0x06004000

ROMDISK_DEPS:= romdisk/*.TGA

.SUFFIXES:
.SUFFIXES: $(SUFFIXES) .tmx .json

%.json: %.tmx
	if ! which tiled >/dev/null 2>&1; then \
	    printf -- "Tiled Map Editor (tiled) is required (http://www.mapeditor.org/)\n" >&2; \
	    exit 1; \
	fi
	echo tiled --export-map "Json files (*.json)" $< $@
	tools/tmx2map
	echo $(RM) $@

include $(INSTALL_ROOT)/share/post.common.mk
