#include "test_harness.h"

#include <gamemath/fix16.h>

TEST("fix16_t::multiply") {
  const fix16_t a1 = fix16_t::from_double(16383.5);
  const fix16_t b1 = fix16_t::from_double(2.0);
  const fix16_t c1 = a1 * b1;

  VERIFY(static_cast<uint32_t>(a1.value) == 0x3FFF8000UL);
  VERIFY(static_cast<uint32_t>(b1.value) == 0x00020000UL);
  VERIFY(static_cast<uint32_t>(c1.value) == 0x7FFF0000UL);
}
