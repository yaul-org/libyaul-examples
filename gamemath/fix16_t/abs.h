#include "../test_harness.h"

TEST("fix16_t::abs") {
  const fix16_t a1 = -1.0_fp;
  const fix16_t b1 = 32767.0_fp;
  const fix16_t c1 = abs(a1);
  const fix16_t d1 = abs(b1);

  VERIFY(static_cast<uint32_t>(a1.value) == 0xFFFF0000UL);
  VERIFY(static_cast<uint32_t>(b1.value) == 0x7FFF0000UL);
  VERIFY(static_cast<uint32_t>(c1.value) == 0x00010000UL);
  VERIFY(static_cast<uint32_t>(d1.value) == 0x7FFF0000UL);
}
