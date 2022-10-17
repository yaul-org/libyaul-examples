/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _GDBSTUB_H_
#define _GDBSTUB_H_

#include <sys/cdefs.h>

#include <stdint.h>

#include <cpu/instructions.h>

__BEGIN_DECLS

#define GDBSTUB_LOAD_ADDRESS        (0x202FE000)
#define GDBSTUB_TRAPA_VECTOR_NUMBER (32)

static inline void __always_inline
gdb_break(void)
{
        cpu_instr_trapa(GDBSTUB_TRAPA_VECTOR_NUMBER);
}

typedef void (*gdb_device_init_t)(void);
typedef uint8_t (*gdb_device_byte_read_t)(void);
typedef void (*gdb_device_byte_write_t)(uint8_t value);

typedef struct {
        gdb_device_init_t init;
        gdb_device_byte_read_t byte_read;
        gdb_device_byte_write_t byte_write;
} __aligned(16) gdb_device_t;

typedef struct {
        unsigned int :8;
        unsigned int major:8;
        unsigned int minor:8;
        unsigned int patch:8;
} __packed gdb_version_t;

typedef struct {
        gdb_version_t version;
        void (*init)(void);
        gdb_device_t *device;
} __aligned(16) gdbstub_t;

__END_DECLS

#endif /* !_GDBSTUB_H_ */
