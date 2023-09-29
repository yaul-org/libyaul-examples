#include <sys/cdefs.h>

#include <stdbool.h>

#include <cpu.h>
#include <dbgio.h>
#include <vdp2.h>

extern void overlay2_foo(void);

static uint8_t _variable1 __uncached __used = 0xC5;
static uint8_t _variable2 __uncached __used = 0x7A;
static uint8_t _variable3 __uncached __used = 0x75;

int32_t
overlay2(void *work)
{
        uint32_t arg1;
        arg1 = *(uint32_t *)work;

        dbgio_printf("0x%08X: 0x%08X\n", &_variable1, _variable1);
        dbgio_printf("0x%08X: 0x%08X\n", &_variable2, _variable2);
        dbgio_printf("0x%08X: 0x%08X\n", &_variable3, _variable3);

        while (arg1 > 0) {
                dbgio_printf("Hello from overlay2 %i times\n", arg1);
                overlay2_foo();
                dbgio_flush();
                vdp2_sync();
                vdp2_sync_wait();

                arg1--;
        }

        return 100;
}
