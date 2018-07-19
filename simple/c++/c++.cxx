/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdlib.h>

#include <yaul.h>

static void _vblank_in_handler(irq_mux_handle_t *);
static void _vblank_out_handler(irq_mux_handle_t *);

static void _hardware_init(void);

static char _ctor_buffer[16];
static char _dtor_buffer[16];

static char* _ctor = &_ctor_buffer[0];
static char* _dtor = &_dtor_buffer[0];

namespace Foo {
    void bar() {
        cons_buffer("Foo::bar()\n");
    }
}

class A {
public:
    A() {
        *_ctor++ = 'A';
    }

    virtual ~A() {
        *_dtor++ = 'A';
    }
};

class B {
public:
    B() {
        *_ctor++ = 'B';
    }

    ~B() {
        *_dtor++ = 'B';
    }
};

class C {
public:
    C() {
        *_ctor++ = 'C';
    }

    ~C() {
        *_dtor++ = 'A';
    }
};

class D: public A {
public:
    D() {
        *_ctor++ = 'D';
    }

    ~D() {
        *_dtor++ = 'A';
    }
};

class T {
public:
    ~T() {
        cons_buffer("T::~T()\n");
    }

    void call() {
        cons_buffer("T::call(), dynamically allocated object\n");
    }
};

template<typename T>
inline T _max(T a, T b) {
    return (a > b) ? a : b;
}

static inline void _function_overload(int a) {
    char buffer[32];

    (void)sprintf(buffer, "%s(), type int: %i\n", __FUNCTION__, a);
    cons_buffer(buffer);
}

static inline void _function_overload(char a) {
    char buffer[32];

    (void)sprintf(buffer, "%s(), type char: '%c'\n", __FUNCTION__, a);
    cons_buffer(buffer);
}

static A a;
static B b;
static C c;
static D d;

int main(void) {
    _hardware_init();

    cons_init(CONS_DRIVER_VDP2, 40, 30);

    cons_buffer("Global constructor order: ");
    cons_buffer(_ctor_buffer);
    cons_buffer("\n");

    char *buffer;
    buffer = (char *)malloc(1024);
    assert(buffer != NULL);

    (void)sprintf(buffer, "_max<T> template: %i, %lu, '%c'\n", _max<int>(-2, -1), _max<uint32_t>(0, -1), _max<char>('A', 'Z'));
    cons_buffer(buffer);

    _function_overload(1);
    _function_overload('A');

    Foo::bar();

    T* t;
    t = new T();

    t->call();

    delete t;

    free(buffer);

    while (true) {
        vdp2_tvmd_vblank_out_wait();

        vdp2_tvmd_vblank_in_wait();
        cons_flush();
    }
}

static void _hardware_init(void) {
    vdp2_init();

    vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
                              TVMD_VERT_224);

    vdp2_sprite_priority_set(0, 0);
    vdp2_sprite_priority_set(1, 0);
    vdp2_sprite_priority_set(2, 0);
    vdp2_sprite_priority_set(3, 0);
    vdp2_sprite_priority_set(4, 0);
    vdp2_sprite_priority_set(5, 0);
    vdp2_sprite_priority_set(6, 0);
    vdp2_sprite_priority_set(7, 0);

    vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE),
                                    COLOR_RGB555(0, 3, 3));

    irq_mux_t *vblank_in;
    vblank_in = vdp2_tvmd_vblank_in_irq_get();
    irq_mux_handle_add(vblank_in, _vblank_in_handler, NULL);

    irq_mux_t *vblank_out;
    vblank_out = vdp2_tvmd_vblank_out_irq_get();
    irq_mux_handle_add(vblank_out, _vblank_out_handler, NULL);

    // Enable interrupts
    cpu_intc_mask_set(0x7);

    vdp2_tvmd_display_set();
}

static void _vblank_in_handler(irq_mux_handle_t *irq_mux __unused) {
    vdp2_commit();
}

static void _vblank_out_handler(irq_mux_handle_t *irq_mux __unused) {
}
