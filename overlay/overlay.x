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
  /* Place the overlays at the end (via __lma_end) */
  OVERLAY ORIGIN (overlay) : NOCROSSREFS AT (__lma_end)
  {
     .overlay1
     {
        /* Wildcard the overlay1/ directory. Recall that @ is being used in
         * place of the Unix/Windows path delimeter */
        "*@overlay1@*.o"(.text.overlay1)
        "*@overlay1@*.o"(.text .text.* .gnu.linkonce.t.*)
        "*@overlay1@*.o"(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
        "*@overlay1@*.o"(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)
        "*@overlay1@*.o"(.bss .bss.* .gnu.linkonce.b.* .sbss .sbss.* .gnu.linkonce.sb.* .scommon COMMON)
     }

     .overlay2
     {
        "*@overlay2@*.o"(.text.overlay2)
        "*@overlay2@*.o"(.text .text .text.* .gnu.linkonce.t.*)
        "*@overlay2@*.o"(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
        "*@overlay2@*.o"(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)
        "*@overlay2@*.o"(.bss .bss.* .gnu.linkonce.b.* .sbss .sbss.* .gnu.linkonce.sb.* .scommon COMMON)

        /* XXX: Currently, uncached variables are not placed in the overlay section */
     }

     .overlay3
     {
        "*@overlay3@*.o"(.text.overlay3)
        "*@overlay3@*.o"(.text .text.* .gnu.linkonce.t.*)
        "*@overlay3@*.o"(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
        "*@overlay3@*.o"(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)
        "*@overlay3@*.o"(.bss .bss.* .gnu.linkonce.b.* .sbss .sbss.* .gnu.linkonce.sb.* .scommon COMMON)
     }
  }

  .program ORIGIN (shared) : AT (ORIGIN (overlay))
  {
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

  /* Everything that hasn't been placed in a section goes here. */
  .shared __program_end : AT (LOADADDR(.program) + SIZEOF(.program))
  {
     . = ALIGN (16);
     PROVIDE_HIDDEN (__shared_start = .);

     *(.text .text.* .gnu.linkonce.t.*)
     *(.rdata .rodata .rodata.* .gnu.linkonce.r.*)
     *(.data .data.* .gnu.linkonce.d.* .sdata .sdata.* .gnu.linkonce.s.*)

     . = ALIGN (4);
     PROVIDE_HIDDEN (__shared_end = .);
  }

  .bss __shared_end : AT (LOADADDR(.shared) + SIZEOF(.shared))
  {
     . = ALIGN (16);
     PROVIDE (__bss_start = .);

     *(.bss .bss.* .gnu.linkonce.b.* .sbss .sbss.* .gnu.linkonce.sb.* .scommon COMMON)

     /* crt0.s BSS initialization assumes the section size of .bss is in
     multiples of 16 */
     . = ALIGN (16);
     PROVIDE (__bss_end = .);
  }

  .uncached (0x20000000 | __bss_end) : AT (LOADADDR(.bss) + SIZEOF(.bss))
  {
     PROVIDE_HIDDEN (__uncached_start = .);

     *(.uncached)
     *(.uncached.*)

     . = ALIGN (16);
     PROVIDE_HIDDEN (__uncached_end = .);
  }

  __end = __bss_end + SIZEOF (.bss) + SIZEOF (.uncached);
  __lma_end = LOADADDR (.bss) + SIZEOF(.bss) + SIZEOF(.uncached);
}
