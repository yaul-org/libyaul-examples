/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <sys/types.h>

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include "tests.h"

#define TEST_PROTOTYPE_DECLARE(name)                                           \
    void __CONCAT(__CONCAT(test_, name),_init(void));                          \
    void __CONCAT(__CONCAT(test_, name),_update(void));                        \
    void __CONCAT(__CONCAT(test_, name),_draw(void));                          \
    void __CONCAT(__CONCAT(test_, name),_exit(void))

#define TEST_ENTRY_INITIALIZE(name) {                                          \
        __STRING(name),                                                        \
        __CONCAT(test_, __CONCAT(name, _init)),                                \
        __CONCAT(test_, __CONCAT(name, _update)),                              \
        __CONCAT(test_, __CONCAT(name, _draw)),                                \
        __CONCAT(test_, __CONCAT(name, _exit))                                 \
    }

TEST_PROTOTYPE_DECLARE(normal_sprite_00);
TEST_PROTOTYPE_DECLARE(normal_sprite_01);

TEST_PROTOTYPE_DECLARE(scaled_sprite_00);
TEST_PROTOTYPE_DECLARE(scaled_sprite_01);

static const struct test tests_primitive_normal_sprite[] = {
        TEST_ENTRY_INITIALIZE(normal_sprite_00),
};

static const struct test tests_primitive_scaled_sprite[] = {
        TEST_ENTRY_INITIALIZE(scaled_sprite_00),
        TEST_ENTRY_INITIALIZE(scaled_sprite_01)
};

static const struct test tests_primitive_distorted_sprite[] = {
};

static const struct test *tests_primitive[] = {
        tests_primitive_normal_sprite,
        tests_primitive_scaled_sprite,
        tests_primitive_distorted_sprite
};

static const struct test *tests_color[] = {
};

static const struct test *tests_special_functions[] = {
};

static const struct test **tests[] = {
        tests_primitive,
        tests_color,
        tests_special_functions
};

const struct test *
tests_fetch(uint32_t type, uint32_t subtype __unused, uint32_t idx __unused)
{
        const struct test **test_type;
        test_type = tests[type];

        const struct test *test_subtype;
        test_subtype = test_type[subtype];

        const struct test *test;
        test = &test_subtype[idx];

        return test;
}
