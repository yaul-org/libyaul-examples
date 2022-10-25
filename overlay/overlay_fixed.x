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
  shared      (Wx) : ORIGIN = 0x06004000, LENGTH = 512K
  overlay     (Wx) : ORIGIN = 0x06080000, LENGTH = 496K
}

SECTIONS
{
   INCLUDE ./build/overlays.x

   .program ORIGIN (shared) : AT (___overlay_start)
   {
      /* This should be first */
      crt0.o(.text .text.* .gnu.linkonce.t.*)
      crt0.o(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
      crt0.o(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)

      /* This is where the main program binary is located */
      KEEP ("*@program@*.o"(.text.user_init)) /* Force keep user_init */
      "*@program@*.o"(.text .text.* .gnu.linkonce.t.*)
      INCLUDE ldscripts/yaul-c++.x
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

   ___end = ___bss_end + SIZEOF (.uncached);
   /* The overlays will be placed right after the main program */
   __lma_end = LOADADDR (.bss) + SIZEOF (.bss) + SIZEOF (.uncached);

   /* Use this to find where the overlay executable address is */
   PROVIDE (___overlay_start = ORIGIN (overlay));
}
