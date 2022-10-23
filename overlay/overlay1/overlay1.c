#include <stdbool.h>

#include <cpu.h>
#include <dbgio/dbgio.h>
#include <vdp2.h>

extern void overlay1_foo(void);

static uint8_t _variable1 __uncached __used = 0xB5;
static uint8_t _variable2 __uncached __used = 0x6A;
static uint8_t _variable3 __uncached __used = 0x65;

int32_t
overlay1(void *work __unused)
{
        dbgio_printf("Hello from overlay1\n");
        dbgio_printf("0x%08X: 0x%08X\n", &_variable1, _variable1);
        dbgio_printf("0x%08X: 0x%08X\n", &_variable2, _variable2);
        dbgio_printf("0x%08X: 0x%08X\n", &_variable3, _variable3);
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
