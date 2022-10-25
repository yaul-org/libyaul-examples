/*
 * Copyright (c) 2012-2022
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

OUTPUT_FORMAT ("elf32-sh")
OUTPUT_ARCH (sh)
ENTRY (_start)
SEARCH_DIR ("$YAUL_INSTALL_ROOT/$YAUL_ARCH_SH_PREFIX")
INPUT (crt0.o)
INPUT (init.o)
GROUP (libyaul.a)
GROUP (libgcc.a)

MEMORY {
  shared (Wx) : ORIGIN = 0x06004000, LENGTH = 512K
}

/* This linkerscript places the overlay start address right at the very end of
 * the main program (including the shared region). */

/* This implies that the overlay start address changes. The heap is also
 * calculated to be at the end of the biggest overlay. For example, if there are
 * 10 overlays, and the biggest one is at 128KiB while the others are 64KiB, then
 * the heap will start 128KiB after the main program (and shared region) */

SECTIONS
{
   .overlay1.uncached (0x20000000 | __overlay1_end) : AT (__overlay1_load_end)
   {
      . = ALIGN (4);
      "*@overlay1@*.o"(.uncached .uncached.*)
      . = ALIGN (4);
   }
   __overlay1_uncached_start = LOADADDR (.overlay1.uncached);
   __overlay1_uncached_end = LOADADDR (.overlay1.uncached) + SIZEOF (.overlay1.uncached);
   __overlay1_uncached_size = SIZEOF (.overlay1.uncached);

   .overlay2.uncached (0x20000000 | __overlay2_end) : AT (__overlay2_load_end)
   {
      . = ALIGN (4);
      "*@overlay2@*.o"(.uncached .uncached.*)
      . = ALIGN (4);
   }
   __overlay2_uncached_start = LOADADDR (.overlay2.uncached);
   __overlay2_uncached_end = LOADADDR (.overlay2.uncached) + SIZEOF (.overlay2.uncached);
   __overlay2_uncached_size = SIZEOF (.overlay2.uncached);

   .overlay3.uncached (0x20000000 | __overlay3_end) : AT (__overlay3_load_end)
   {
      . = ALIGN (4);
      "*@overlay3@*.o"(.uncached .uncached.*)
      . = ALIGN (4);
   }
   __overlay3_uncached_start = LOADADDR (.overlay3.uncached);
   __overlay3_uncached_end = LOADADDR (.overlay3.uncached) + SIZEOF (.overlay3.uncached);
   __overlay3_uncached_size = SIZEOF (.overlay3.uncached);

   __overlay_load_offset = __lma_end;

   .overlay1 ___overlay_start : AT (__overlay_load_offset)
   {
      . = ALIGN (4);

      /* Wildcard the overlay1/ directory. Recall that @ is being used in
       * place of the Unix/Windows path delimeter */
      KEEP ("*@overlay1@*.o"(.text.overlay1))
      "*@overlay1@*.o"(.text .text.* .gnu.linkonce.t.*)
      "*@overlay1@*.o"(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
      "*@overlay1@*.o"(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)
      "*@overlay1@*.o"(.bss .bss.* .gnu.linkonce.b.* .sbss .sbss.* .gnu.linkonce.sb.* .scommon COMMON)

      *(.overlay1.uncached)

      . = ALIGN (4);
   }
   __overlay1_start = ADDR (.overlay1);
   __overlay1_end = ADDR (.overlay1) + SIZEOF (.overlay1);
   __overlay1_size = SIZEOF (.overlay1) + __overlay1_uncached_size;
   __overlay1_load_end = LOADADDR (.overlay1) + SIZEOF (.overlay1);
   __overlay_load_offset += __overlay1_size;

   .overlay2 ___overlay_start : AT (__overlay_load_offset)
   {
      . = ALIGN (4);

      KEEP ("*@overlay2@*.o"(.text.overlay2))
      "*@overlay2@*.o"(.text .text .text.* .gnu.linkonce.t.*)
      "*@overlay2@*.o"(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
      "*@overlay2@*.o"(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)
      "*@overlay2@*.o"(.bss .bss.* .gnu.linkonce.b.* .sbss .sbss.* .gnu.linkonce.sb.* .scommon COMMON)

      *(.overlay2.uncached)

      . = ALIGN (4);
   }
   __overlay2_start = ADDR (.overlay2);
   __overlay2_end = ADDR (.overlay2) + SIZEOF (.overlay2);
   __overlay2_size = SIZEOF (.overlay2) + __overlay2_uncached_size;
   __overlay2_load_end = LOADADDR (.overlay2) + SIZEOF (.overlay2);
   __overlay_load_offset += __overlay2_size;

   .overlay3 ___overlay_start : AT (__overlay_load_offset)
   {
      . = ALIGN (4);

      KEEP ("*@overlay3@*.o"(.text.overlay3))
      "*@overlay3@*.o"(.text .text.* .gnu.linkonce.t.*)
      "*@overlay3@*.o"(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
      "*@overlay3@*.o"(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)
      "*@overlay3@*.o"(.bss .bss.* .gnu.linkonce.b.* .sbss .sbss.* .gnu.linkonce.sb.* .scommon COMMON)

      *(.overlay3.uncached)

      . = ALIGN (4);
   }
   __overlay3_start = ADDR (.overlay3);
   __overlay3_end = ADDR (.overlay3) + SIZEOF (.overlay3);
   __overlay3_size = SIZEOF (.overlay3) + __overlay3_uncached_size;
   __overlay3_load_end = LOADADDR (.overlay3) + SIZEOF (.overlay3);
   __overlay_load_offset += __overlay3_size;

   __overlay_load_size = __overlay_load_offset - __lma_end;

   .program ORIGIN (shared) : AT (ORIGIN (shared))
   {
      /* This should be first */
      crt0.o(.text .text.* .gnu.linkonce.t.*)
      crt0.o(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
      crt0.o(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)

      /* This is where the main program binary is located */
      KEEP ("*@program@*.o"(.text.user_init)) /* Force keep user_init */
      "*@program@*.o"(.text .text.* .gnu.linkonce.t.*)
      "*@program@*.o"(.rdata .rodata .rodata.*)
      "*@program@*.o"(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)

      . = ALIGN (4);
      __program_end = .;
   }

   /* Everything that hasn't been placed in a section goes here */
   .shared __program_end : AT (LOADADDR (.program) + SIZEOF (.program))
   {
      *(.text .text.* .gnu.linkonce.t.*)
      *(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
      *(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)

      libgcc.a(.text .text.* .gnu.linkonce.t.*)
      libgcc.a(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
      libgcc.a(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)

      . = ALIGN (4);
      __shared_end = .;
   }

   .bss __shared_end : AT (LOADADDR (.shared) + SIZEOF (.shared))
   {
      . = ALIGN (16);
      PROVIDE (___bss_start = .);

      *(.bss .bss.* .gnu.linkonce.b.* .sbss .sbss.* .gnu.linkonce.sb.* .scommon COMMON)

      /* crt0.s BSS initialization assumes the section size of .bss is in
       * multiples of 16 */
      . = ALIGN (16);
      PROVIDE (___bss_end = .);
   }

   .uncached (0x20000000 | ___bss_end) : AT (LOADADDR (.bss) + SIZEOF (.bss))
   {
      *(.uncached)
      *(.uncached.*)

      . = ALIGN (4);
   }

   ___end = (___bss_end + SIZEOF (.uncached)) + __overlay_load_size;
   /* The overlays will be placed right after the main program */
   __lma_end = LOADADDR (.bss) + SIZEOF (.bss) + SIZEOF (.uncached);

   /* Use this to find where the overlay executable address is */
   PROVIDE (___overlay_start = ___bss_end + SIZEOF (.uncached));
}
