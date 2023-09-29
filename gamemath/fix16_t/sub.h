#include "test_harness.h"

#include <gamemath/fix16.h>

TEST("fix16_t::sub") {
  const fix16_t a1 = fix16_t::from_double(32767.0);
  const fix16_t b1 = fix16_t::from_double(1.0);
  const fix16_t c1 = a1 - b1;

  VERIFY(static_cast<uint32_t>(a1.value == 0x7FFF0000));
  VERIFY(static_cast<uint32_t>(b1.value == 0x00010000));
  VERIFY(static_cast<uint32_t>(c1.value == 0x7FFE0000));
}
