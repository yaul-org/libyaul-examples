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

#ifndef ET_H
#define ET_H

/* Embedded Test (ET) version */
#define ET_VERSION "2.0.0"

/*! macro to define a test group */
#define TEST_GROUP(name_) \
    char const ET_Group_[] = name_; \
    void ET_Run_(void)

/* macro to start a new test */
#define TEST(title_) \
    if (ET_Test_(title_, 0))

/* macro to skip a test */
#define SKIP_TEST(title_) \
    if (ET_Test_(title_, 1))

/*! macro to verify a test expectation */
#define VERIFY(cond_) \
    ((cond_) ? (void)0 : ET_Fail(#cond_, &ET_Group_[0], __LINE__))

#define VERIFY_ASSERT(module_, label_) \
    ET_VerifyAssert((module_), (label_))

/*! macro to force a failure of a test */
#define FAIL(note_) \
    (ET_Fail(note_, &ET_Group_[0], __LINE__))

#ifndef ARRAY_NELEM
/*! convenience macro to provide the number of elements in the array a_ */
#define ARRAY_NELEM(a_)  (sizeof(a_) / sizeof((a_)[0]))
#endif /* ARRAY_NELEM */

#ifdef __cplusplus
extern "C" {
#endif

void ET_Setup(void);    /*!< called right before each test */
void ET_Teardown(void); /*!< called right after each test */

// Callback functions to be implemented in the ET board support packages

void ET_OnInit(int argc, char *argv[]);
void ET_OnPrintChar(char const ch);
void ET_OnExit(int err);

// Public helpers
void ET_Fail(char const *cond, char const *group, int line);
void ET_ExpectAssert(char const *module, int label);
void ET_VerifyAssert(char const *module, int label);

// Private helpers
void ET_Run_(void);
int  ET_Test_(char const *title, int skip);
extern char const ET_Group_[];

#ifdef __cplusplus
}
#endif

#endif /* ET_H */
