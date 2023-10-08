#include "../test_harness.h"

TEST("angle_t::deg_conv1") {
  const angle_t a1 = 90.0_deg;

  VERIFY(static_cast<uint16_t>(a1.value) == 0x4000);
}
