#include "../test_harness.h"

TEST("angle_t::cos_270.0_deg") {
  const fix16_t a1 = fix16_cos(270.0_deg);

  VERIFY(a1.is_near(0.0_fp));
}
