/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdlib.h>

#include <yaul.h>

static char _ctor_buffer[16];
static char _dtor_buffer[16];

static char* _ctor = &_ctor_buffer[0];
static char* _dtor = &_dtor_buffer[0];

namespace Foo {
    void bar() {
        dbgio_puts("Foo::bar()\n");
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
        *_dtor++ = 'C';
    }
};

class D: public A {
public:
    D() {
        *_ctor++ = 'D';
    }

    ~D() {
        *_dtor++ = 'D';
    }
};

class T {
public:
    ~T() {
        dbgio_puts("T::~T()\n");
    }

    void call() {
        dbgio_puts("T::call(), dynamically allocated object\n");
    }
};

template<typename T>
inline T _max(T a, T b) {
    return (a > b) ? a : b;
}

static inline void _function_overload(int a) {
    dbgio_printf("%s(), type int: %i\n", __FUNCTION__, a);
}

static inline void _function_overload(char a) {
    dbgio_printf("%s(), type char: '%c'\n", __FUNCTION__, a);
}

static A a;
static B b;
static C c;
static D d;

int main(void) {
    dbgio_init();
    dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
    dbgio_dev_font_load();

    dbgio_puts("Global constructor order: ");
    dbgio_puts(_ctor_buffer);
    dbgio_puts("\n");

    dbgio_printf("_max<T> template: %i, %lu, '%c'\n",
                 _max<int>(-2, -1),
                 _max<uint32_t>(0, -1),
                 _max<char>('A', 'Z'));

    _function_overload(1);
    _function_overload('A');

    Foo::bar();

    T* t;
    t = new T();

    t->call();

    delete t;

    dbgio_flush();
    vdp2_sync();
    vdp2_sync_wait();

    while (true) {
    }
}

void user_init(void) {
    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
                              VDP2_TVMD_VERT_224);

    vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
                             RGB1555(1, 0, 3, 15));

    vdp2_tvmd_display_set();
}
