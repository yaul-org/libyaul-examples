#include "../test_harness.h"

TEST("angle_t::div_90_by_2") {
  const angle_t a1 = 90.0_deg;
  const angle_t a2 = a1 >> 1;

  VERIFY(a2 == 45.0_deg);
}
