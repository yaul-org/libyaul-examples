#include "../test_harness.h"

TEST("angle_t::cos_45.0_deg") {
  const fix16_t a1 = fix16_cos(45.0_deg);

  VERIFY(a1.is_near(0.707_fp));
}
