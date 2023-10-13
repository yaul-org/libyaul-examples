#include "../test_harness.h"

TEST("fix16_quat_t::quat_mult1") {
  constexpr angle_t heading1  = -90.0_deg;
  constexpr angle_t attitude1 = 0.0_deg;
  constexpr angle_t bank1     = 0.0_deg;

  const fix16_quat_t q1 = fix16_quat_t::from_euler({bank1, heading1, attitude1});

  constexpr angle_t heading2  = 0.0_deg;
  constexpr angle_t attitude2 = 0.0_deg;
  constexpr angle_t bank2     = -90.0_deg;

  const fix16_quat_t q2 = fix16_quat_t::from_euler({bank2, heading2, attitude2});

  const fix16_quat_t a1{q1 * q2};

  VERIFY(a1.w.is_near(0.5_fp));
  VERIFY(a1.comp.x.is_near(-0.5_fp));
  VERIFY(a1.comp.y.is_near(-0.5_fp));
  VERIFY(a1.comp.z.is_near(-0.5_fp));
}
