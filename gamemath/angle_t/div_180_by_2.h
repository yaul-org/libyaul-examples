#include "../test_harness.h"

TEST("angle_t::div_180_by_2") {
  const angle_t a1 = 180.0_deg;
  const angle_t a2 = a1 >> 1;

  VERIFY(a2 == 90.0_deg);
}
