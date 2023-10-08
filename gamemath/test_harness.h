#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#ifdef __INTELLISENSE__
#include "et/et.h"

// Avoid causing intellisense to freak out
#ifndef INSIDE_TEST_GROUP
#include "test_deps.h"

#undef TEST
#define TEST(...) static void __attribute__ ((unused)) _stub()
#endif // !INSIDE_TEST_GROUP
#endif // __INTELLISENSE__

#endif // TEST_HARNESS_H
