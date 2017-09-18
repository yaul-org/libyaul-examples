/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

static char ctor_buf[16];
static char dtor_buf[16];

static char* ctor_bufp = &ctor_buf[0];
static char* dtor_bufp = &dtor_buf[0];

class A {
public:
    A() {
        *ctor_bufp++ = 'A';
    }

    virtual ~A() {
        *dtor_bufp++ = 'A';
    }
};

class B {
public:
    B() {
        *ctor_bufp++ = 'B';
    }

    ~B() {
        *dtor_bufp++ = 'B';
    }
};

class C {
public:
    C() {
        *ctor_bufp++ = 'C';
    }

    ~C() {
        *dtor_bufp++ = 'A';
    }
};

class D: public A {
public:
    D() {
        *ctor_bufp++ = 'D';
    }

    ~D() {
        *dtor_bufp++ = 'A';
    }
};

static A a;
static B b;
static C c;
static D d;

int
main(void)
{
    vdp1_init();

    vdp2_init();
    vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(2, 0x01FFFE),
                                    COLOR_RGB555(0, 7, 0));
    cons_init(CONS_DRIVER_VDP2, 40, 30);

    cons_buffer("Global constructor order: ");
    cons_buffer(ctor_buf);
    cons_buffer("\n");

    while(1) {
        vdp2_tvmd_vblank_out_wait(); {
        }

        vdp2_tvmd_vblank_in_wait(); {
        }

        cons_flush();
    }

    return 0;
}
