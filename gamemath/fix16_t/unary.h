#include "test_harness.h"

#include <gamemath/fix16.h>

TEST("fix16_t::unary") {
  const fix16_t a1 = fix16_t::from_double(-32767.0);
  const fix16_t b1 = -fix16_t::from_double(32767.0);

  VERIFY(static_cast<uint32_t>(a1.value) == 0x80010000UL);
  VERIFY(static_cast<uint32_t>(b1.value) == 0x80010000UL);
}
