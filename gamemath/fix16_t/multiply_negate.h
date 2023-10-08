#include "../test_harness.h"

TEST("fix16_t::multiply_negate") {
  const fix16_t a1 = 16383.5_fp;
  const fix16_t b1 = -2.0_fp;
  const fix16_t c1 = a1 * b1;

  VERIFY(static_cast<uint32_t>(a1.value) == 0x3FFF8000UL);
  VERIFY(static_cast<uint32_t>(b1.value) == 0xFFFE0000UL);
  VERIFY(static_cast<uint32_t>(c1.value) == 0x80010000UL);
}
