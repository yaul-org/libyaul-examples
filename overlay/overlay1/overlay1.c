#include <stdbool.h>

#include <cpu.h>
#include <dbgio/dbgio.h>
#include <vdp2.h>

extern void overlay1_foo(void);

int32_t
overlay1(void *work __unused)
{
        dbgio_printf("Hello from overlay1\n");
        dbgio_flush();
        vdp2_sync();
        vdp2_sync_wait();

        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();
        cpu_instr_nop();

        return 101;
}
