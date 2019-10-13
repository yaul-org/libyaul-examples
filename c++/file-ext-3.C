#include <sys/cdefs.h>

// This file on (Windows or possibly Mac OS) case-insensitive file systems would
// treat this file not as a C++ file, but C.
//
// Be aware of this.

static void _unused_func() __used;

static void _unused_func() {
}
