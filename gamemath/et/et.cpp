/* ET: Embedded Test <https://github.com/QuantumLeaps/Embedded-Test>
 *
 *                    Q u a n t u m  L e a P s
 *                    ------------------------
 *                    Modern Embedded Software
 *
 * Copyright (C) 2005 Quantum Leaps, <state-machine.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE. */

#include "et.h"

static void print_str(char const* str);
static void print_dec(unsigned const num);
static void print_summary(unsigned ok);
static void test_end(void);
// TODO: Replace with Yaul implementation
static int str_cmp(char const* str1, char const* str2);

static unsigned _l_test_count;
static unsigned _l_skip_count;
static unsigned _l_skip_last;

static char const* _l_expect_assert_module;
static int _l_expect_assert_label;

int main(int argc, char* argv[]) {
  ET_OnInit(argc, argv);

  print_str("\nET Embedded Test " ET_VERSION ": <https://github.com/QuantumLeaps/ET>\n");
  print_str("---------------- Group: ");
  print_str(ET_Group_);
  print_str(" ----------------\n");

  ET_Run_();

  test_end();
  print_summary(1U);

  ET_OnExit(0);

  return 0;
}

int ET_Test_(char const* title, int skip) {
  test_end();
  ++_l_test_count;
  ET_OnPrintChar('[');
  print_dec(_l_test_count);
  print_str("] \"");
  print_str(title);
  print_str("\" ");
  if (skip) {
    ++_l_skip_count;
  } else {
    ET_Setup();
    ET_OnPrintChar('.');
  }
  _l_skip_last = skip;
  return skip == 0;
}

static void test_end(void) {
  if (_l_expect_assert_module != (char const*)0) {
    ET_Fail("Expected Assertion didn't fire", _l_expect_assert_module, _l_expect_assert_label);
  } else if (_l_test_count > 0) {
    if (_l_skip_last) {
      print_str(" SKIPPED\n");
      _l_skip_last = 0;
    } else {
      ET_Teardown();
      print_str(". PASSED\n");
    }
  }
}

void ET_Fail(char const* cond, char const* group, int line) {
  print_str(" FAILED\n--> ");
  print_str(group);
  ET_OnPrintChar(':');
  print_dec(line);
  ET_OnPrintChar(' ');
  print_str(cond);
  ET_OnPrintChar('\n');
  print_summary(0U);

  ET_OnExit(-1);
}

void ET_expect_assert(char const* module, int label) {
  _l_expect_assert_module = module;
  _l_expect_assert_label  = label;
}

void ET_verify_assert_(char const* module, int label) {
  if ((_l_expect_assert_label == label) && (str_cmp(module, _l_expect_assert_module) == 0)) {
    _l_expect_assert_module = (char const*)0;
    test_end();
    print_str("Assertion (expected) --> Exiting\n");
    print_summary(1U);

    ET_OnExit(0); /* success */
  } else {
    ET_Fail("Unexpected assertion", module, label);
  }
}

static void print_summary(unsigned ok) {
  print_str("---------------- ");
  print_dec(_l_test_count);
  print_str(" test(s), ");
  print_dec(_l_skip_count);
  print_str(" skipped ----------------\n");
  print_str(ok ? "OK\n" : "FAILED\n");
}

static void print_str(char const* str) {
  for (; *str != '\0'; ++str) {
    ET_OnPrintChar(*str);
  }
}

static void print_dec(unsigned const num) {
  // Find power of 10 of the first decimal digit of the number
  unsigned pwr10 = 1U;
  for (; num > (pwr10 * 9U); pwr10 *= 10U) {
  }
  // Print the decimal digits of the number
  do {
    ET_OnPrintChar((char)('0' + ((num / pwr10) % 10U)));
    pwr10 /= 10U;
  } while (pwr10 != 0U);
}

static int str_cmp(char const* str1, char const* str2) {
  while (*str1 == *str2++) {
    if (*str1++ == '\0') {
      return 0;
    }
  }
  --str2;
  return *(unsigned char*)str1 - *(unsigned char*)str2;
}
