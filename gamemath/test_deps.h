#ifndef TEST_DEPS_H
#define TEST_DEPS_H

#include <assert.h>
#include <stdio.h>

#include <gamemath/fix16.h>
#include <gamemath/fix16/fix16_quat.h>
#include <gamemath/fix16/fix16_trig.h>
#include <gamemath/fix16/fix16_vec3.h>

template <class T> void PutsType(const T& type) {
  // This might cause us trouble
  char buffer[128];

  const size_t size __unused = type.to_string(buffer);
  assert(size < sizeof(buffer));

  (void)puts(buffer);
}

#endif // TEST_DEPS_H
