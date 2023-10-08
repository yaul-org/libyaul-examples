#include "../test_harness.h"

TEST("angle_t::deg_conv4") {
  const angle_t a1 = 360.0_deg;

  VERIFY(static_cast<uint16_t>(a1.value) == 0x0000);
}
