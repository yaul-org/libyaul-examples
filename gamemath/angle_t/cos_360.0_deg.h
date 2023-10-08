#include "../test_harness.h"

TEST("angle_t::cos_360.0_deg") {
  const fix16_t a1 = fix16_cos(360.0_deg);

  VERIFY(a1.is_near(1.0_fp));
}
