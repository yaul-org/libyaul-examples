EXAMPLES:= \
	arp-comm \
	bcl \
	c++ \
	cd-block \
	cpu-divu \
	cpu-dmac \
	cpu-dual \
	cpu-frt \
	cpu-wdt \
	dbgio-menu \
	dbgio-usb-cart \
	dma-queue \
	dram-cart \
	gdb \
	mm-stats \
	netlink-template \
	scu-dsp \
	smpc-rtc \
	usb-cart \
	vdp1-balls \
	vdp1-drawing \
	vdp1-g3d \
	vdp1-interlace \
	vdp1-mesh \
	vdp1-mic3d \
	vdp1-software-blending \
	vdp1-st-niccc \
	vdp1-uv-coords \
	vdp1-zoom-sprite \
	vdp2-24bpp-bitmap \
	vdp2-2x2-plane \
	vdp2-back-screen \
	vdp2-effect-tunnel \
	vdp2-line-scroll \
	vdp2-nbg0 \
	vdp2-normal-bitmap \
	vdp2-rbg0 \
	vdp2-rbg0-bitmap \
	vdp2-reduction-bitmap \
	vdp2-special-function \
	vdp2-zooming

# Following examples are broken:
#   scu-timers
#   fileserver

ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

ifeq ($(strip $(SILENT)),)
  ECHO=
else
  ECHO=@
endif
export ECHO

.PHONY: all clean list-examples

all clean:
	$(ECHO)for example in $(EXAMPLES); do \
		printf -- "[1;36m$@[m [1;32mexamples/$$example[m\n"; \
		($(MAKE) -C $$example $@) || exit $$?; \
	done

list-examples:
	$(ECHO)/usr/bin/find . -maxdepth 1 -type d | tail -n +2 | sed -E 's/^\.\///g;/^\./d;/^_/d;/^shared$$/d' | sort -n
