#include "../test_harness.h"

TEST("fix16_quat_t::euler_conv19") {
  constexpr angle_t heading  = 180.0_deg;
  constexpr angle_t attitude = 0.0_deg;
  constexpr angle_t bank     = 180.0_deg;

  fix16_quat_t a1;
  fix16_quat_t::from_euler({bank, heading, attitude}, a1);

  VERIFY(a1.w.is_near(0.0_fp));
  VERIFY(a1.comp.x.is_near(0.0_fp));
  VERIFY(a1.comp.y.is_near(0.0_fp));

  // Error?
  //   z = c1 s2 c3 - s1 c2 s3
  //
  //   where:
  //   c1 = cos(heading / 2  = 90) = 0
  //   c2 = cos(attitude / 2 =  0) = 1
  //   c3 = cos(bank / 2     = 90) = 0
  //   s1 = sin(heading / 2  = 90) = 1
  //   s2 = sin(attitude / 2 =  0) = 0
  //   s3 = sin(bank / 2     = 90) = 1
  //
  // z = 1 Should be -1?
  VERIFY(a1.comp.z.is_near(-1.0_fp));
}
