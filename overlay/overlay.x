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
  overlay     (Wx) : ORIGIN = 0x06080000, LENGTH = 256K
  master_stack (W) : ORIGIN = 0x06004000, LENGTH = 0x00002000
  slave_stack  (W) : ORIGIN = 0x06002000, LENGTH = 0x00001400
}

PROVIDE (__master_stack = ORIGIN (master_stack));
PROVIDE (__master_stack_end = ORIGIN (master_stack) - LENGTH (master_stack));
PROVIDE_HIDDEN (__slave_stack = ORIGIN (slave_stack));
PROVIDE_HIDDEN (__slave_stack_end = ORIGIN (slave_stack) - LENGTH (slave_stack));

PROVIDE (__overlay_start = ORIGIN (overlay));

SECTIONS
{
   .overlay1.uncached (0x20000000 | __overlay1_end) : AT (__overlay1_load_end)
   {
      . = ALIGN (16);
      "*@overlay1@*.o"(.uncached .uncached.*)
      . = ALIGN (16);
   }
   __overlay1_uncached_start = LOADADDR (.overlay1.uncached);
   __overlay1_uncached_end = LOADADDR (.overlay1.uncached) + SIZEOF (.overlay1.uncached);
   __overlay1_uncached_size = SIZEOF (.overlay1.uncached);

   .overlay2.uncached (0x20000000 | __overlay2_end) : AT (__overlay2_load_end)
   {
      . = ALIGN (16);
      "*@overlay2@*.o"(.uncached .uncached.*)
      . = ALIGN (16);
   }
   __overlay2_uncached_start = LOADADDR (.overlay2.uncached);
   __overlay2_uncached_end = LOADADDR (.overlay2.uncached) + SIZEOF (.overlay2.uncached);
   __overlay2_uncached_size = SIZEOF (.overlay2.uncached);

   .overlay3.uncached (0x20000000 | __overlay3_end) : AT (__overlay3_load_end)
   {
      . = ALIGN (16);
      "*@overlay3@*.o"(.uncached .uncached.*)
      . = ALIGN (16);
   }
   __overlay3_uncached_start = LOADADDR (.overlay3.uncached);
   __overlay3_uncached_end = LOADADDR (.overlay3.uncached) + SIZEOF (.overlay3.uncached);
   __overlay3_uncached_size = SIZEOF (.overlay3.uncached);

   __overlay_load_offset = __lma_end;

   .overlay1 ORIGIN (overlay) : AT (__overlay_load_offset)
   {
      . = ALIGN (16);

      /* Wildcard the overlay1/ directory. Recall that @ is being used in
       * place of the Unix/Windows path delimeter */
      KEEP ("*@overlay1@*.o"(.text.overlay1))
      "*@overlay1@*.o"(.text .text.* .gnu.linkonce.t.*)
      "*@overlay1@*.o"(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
      "*@overlay1@*.o"(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)
      "*@overlay1@*.o"(.bss .bss.* .gnu.linkonce.b.* .sbss .sbss.* .gnu.linkonce.sb.* .scommon COMMON)

      *(.overlay1.uncached)

      . = ALIGN (16);
   }
   __overlay1_start = ADDR (.overlay1);
   __overlay1_end = ADDR (.overlay1) + SIZEOF (.overlay1);
   __overlay1_size = SIZEOF (.overlay1) + __overlay1_uncached_size;
   __overlay1_load_end = LOADADDR (.overlay1) + SIZEOF (.overlay1);
   __overlay_load_offset += __overlay1_size;

   .overlay2 ORIGIN (overlay) : AT (__overlay_load_offset)
   {
      . = ALIGN (16);

      KEEP ("*@overlay2@*.o"(.text.overlay2))
      "*@overlay2@*.o"(.text .text .text.* .gnu.linkonce.t.*)
      "*@overlay2@*.o"(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
      "*@overlay2@*.o"(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)
      "*@overlay2@*.o"(.bss .bss.* .gnu.linkonce.b.* .sbss .sbss.* .gnu.linkonce.sb.* .scommon COMMON)

      *(.overlay2.uncached)

      . = ALIGN (16);
   }
   __overlay2_start = ADDR (.overlay2);
   __overlay2_end = ADDR (.overlay2) + SIZEOF (.overlay2);
   __overlay2_size = SIZEOF (.overlay2) + __overlay2_uncached_size;
   __overlay2_load_end = LOADADDR (.overlay2) + SIZEOF (.overlay2);
   __overlay_load_offset += __overlay2_size;

   .overlay3 ORIGIN (overlay) : AT (__overlay_load_offset)
   {
      . = ALIGN (16);

      KEEP ("*@overlay3@*.o"(.text.overlay3))
      "*@overlay3@*.o"(.text .text.* .gnu.linkonce.t.*)
      "*@overlay3@*.o"(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
      "*@overlay3@*.o"(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)
      "*@overlay3@*.o"(.bss .bss.* .gnu.linkonce.b.* .sbss .sbss.* .gnu.linkonce.sb.* .scommon COMMON)

      *(.overlay3.uncached)

      . = ALIGN (16);
   }
   __overlay3_start = ADDR (.overlay3);
   __overlay3_end = ADDR (.overlay3) + SIZEOF (.overlay3);
   __overlay3_size = SIZEOF (.overlay3) + __overlay3_uncached_size;
   __overlay3_load_end = LOADADDR (.overlay3) + SIZEOF (.overlay3);
   __overlay_load_offset += __overlay3_size;

   .program ORIGIN (shared) : AT (ORIGIN (overlay))
   {
      . = ALIGN (4);
      PROVIDE_HIDDEN (__program_start = .);

      /* This should be first */
      crt0.o(.text .text.* .gnu.linkonce.t.*)
      crt0.o(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
      crt0.o(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)

      . = ALIGN (16);
      __INIT_SECTION__ = .;
      KEEP (*(.init))

      . = ALIGN (16);
      __FINI_SECTION__ = .;
      KEEP (*.o(.fini))
      SHORT (0x000B) /* RTS */
      SHORT (0x0009) /* NOP */

      . = ALIGN (16);
      __CTOR_SECTION__ = .;
      KEEP (*(.ctor))
      SHORT (0x000B) /* RTS */
      SHORT (0x0009) /* NOP */

      . = ALIGN (16);
      __DTOR_SECTION__ = .;
      KEEP (*(.dtor))
      SHORT (0x000B) /* RTS */
      SHORT (0x0009) /* NOP */

      libgcc.a(.text .text.* .gnu.linkonce.t.*)
      libgcc.a(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
      libgcc.a(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)

      /* This is where the main program binary is located */
      "*@program@*.o"(.text .text.* .gnu.linkonce.t.*)
      "*@program@*.o"(.rdata .rodata .rodata.*)
      "*@program@*.o"(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)

      . = ALIGN (4);
      PROVIDE_HIDDEN (__program_end = .);
   }

   /* Everything that hasn't been placed in a section goes here */
   .shared __program_end : AT (LOADADDR (.program) + SIZEOF (.program))
   {
      . = ALIGN (4);
      PROVIDE_HIDDEN (__shared_start = .);

      *(.text .text.* .gnu.linkonce.t.*)
      *(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
      *(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)

      . = ALIGN (4);
      PROVIDE_HIDDEN (__shared_end = .);
   }

   .bss __shared_end : AT (LOADADDR (.shared) + SIZEOF (.shared))
   {
      . = ALIGN (16);
      PROVIDE (__bss_start = .);

      *(.bss .bss.* .gnu.linkonce.b.* .sbss .sbss.* .gnu.linkonce.sb.* .scommon COMMON)

      /* crt0.s BSS initialization assumes the section size of .bss is in
      multiples of 16 */
      . = ALIGN (16);
      PROVIDE (__bss_end = .);
   }

   .uncached (0x20000000 | __bss_end) : AT (LOADADDR (.bss) + SIZEOF (.bss))
   {
      . = ALIGN (16);
      PROVIDE_HIDDEN (__uncached_start = .);

      *(.uncached)
      *(.uncached.*)

      . = ALIGN (16);
      PROVIDE_HIDDEN (__uncached_end = .);
   }

   __end = __bss_end + SIZEOF (.bss) + SIZEOF (.uncached);
   __lma_end = LOADADDR (.bss) + SIZEOF (.bss) + SIZEOF (.uncached);
}
