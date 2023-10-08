#include "../test_harness.h"

TEST("angle_t::unary") {
  const angle_t a1 = -90.0_deg;
  const angle_t a2 = 270.0_deg;

  VERIFY(a1 == a2);
}
