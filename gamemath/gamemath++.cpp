/*
 * Copyright (c) 2012-2023 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <yaul.h>

#include "et/et.h"

static size_t _file_write(FILE*, const uint8_t* s, size_t l) {
  for (uint32_t i = 0; i < l; i++) {
    MEMORY_WRITE(8, CS0(0x00100001), s[i]);
  }

  return l;
}

void ET_OnInit(int, char*[]) {
  /* Nothing else is needed */
  __stdout_FILE.write = _file_write;

  vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A, VDP2_TVMD_VERT_224);

  vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE), RGB1555(1, 0, 0, 7));

  vdp2_tvmd_display_set();

  dbgio_init();
  dbgio_dev_default_init(DBGIO_DEV_MEDNAFEN_DEBUG);

  // Hide cursor
  dbgio_printf("[?25l");
  dbgio_flush();

  vdp2_sync();
  vdp2_sync_wait();
}

void ET_OnPrintChar(char const ch) { fputc(ch, stdout); }

void ET_OnExit(int code) {
  rgb1555_t color;

  if (code < 0) {
    color = RGB1555(1, 7, 0, 0);
  } else {
    color = RGB1555(1, 0, 7, 0);
  }

  vdp2_tvmd_vblank_in_next_wait(1);

  MEMORY_WRITE(16, VDP2_VRAM_ADDR(3, 0x01FFFE), color.raw);

  exit(code);
}

void ET_Setup(void) {}

void ET_Teardown(void) {}

TEST_GROUP("C++ GameMath") {
#define INSIDE_TEST_GROUP

  // clang-format: off
#include "fix16_t/add.h"
#include "fix16_t/add_negate.h"
#include "fix16_t/sub.h"
#include "fix16_t/unary.h"
#include "fix16_t/multiply.h"
#include "fix16_t/multiply_negate.h"
#include "fix16_t/abs.h"
  // clang-format: on

#undef INSIDE_TEST_GROUP
}
