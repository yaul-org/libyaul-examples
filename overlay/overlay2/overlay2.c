#include <stdbool.h>

#include <dbgio/dbgio.h>
#include <vdp2.h>

extern void overlay2_foo(void);

int32_t
overlay2(void *work)
{
        uint32_t arg1;
        arg1 = *(uint32_t *)work;

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
