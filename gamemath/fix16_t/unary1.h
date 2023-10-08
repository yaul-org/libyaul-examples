#include "../test_harness.h"

TEST("fix16_t::unary1") {
  const fix16_t a1 = -32767.0_fp;
  const fix16_t b1 = -32767.0_fp;

  VERIFY(static_cast<uint32_t>(a1.value) == 0x80010000UL);
  VERIFY(static_cast<uint32_t>(b1.value) == 0x80010000UL);
}
