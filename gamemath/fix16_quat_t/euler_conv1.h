#include "../test_harness.h"

TEST("fix16_quat_t::euler_conv1") {
  fix16_quat_t a1;
  fix16_quat_t::from_euler({0.0_deg, 0.0_deg, 0.0_deg}, a1);

  VERIFY(a1.w.is_near(1.0_fp));
  VERIFY(a1.comp.x.is_near(0.0_fp));
  VERIFY(a1.comp.y.is_near(0.0_fp));
  VERIFY(a1.comp.z.is_near(0.0_fp));
}
