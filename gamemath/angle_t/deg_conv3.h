#include "../test_harness.h"

TEST("angle_t::deg_conv3") {
  const angle_t a1 = 270.0_deg;

  VERIFY(static_cast<uint16_t>(a1.value) == 0xC000);
}
