#include "../test_harness.h"

TEST("angle_t::deg_conv2") {
  const angle_t a1 = 180.0_deg;

  VERIFY(static_cast<uint16_t>(a1.value) == 0x8000);
}
