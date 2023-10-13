#include "../test_harness.h"

TEST("fix16_quat_t::vec3_mult_heading_45") {
  constexpr angle_t heading  = 45.0_deg;
  constexpr angle_t attitude = 0.0_deg;
  constexpr angle_t bank     = 0.0_deg;

  fix16_quat_t q;
  fix16_quat_t::from_euler({bank, heading, attitude}, q);

  const fix16_vec3_t v{1.0_fp, 0.0_fp, 0.0_fp};
  const fix16_vec3_t a1{v * q};

  VERIFY(q.w.is_near(0.9239_fp));
  VERIFY(q.comp.x.is_near(0.0_fp));
  VERIFY(q.comp.y.is_near(0.3827_fp));
  VERIFY(q.comp.z.is_near(0.0_fp));

  VERIFY(a1.x.is_near(0.7071_fp));
  VERIFY(a1.y.is_near(0.0_fp));
  VERIFY(a1.z.is_near(-0.7071_fp));
}
