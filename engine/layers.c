/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "engine.h"

static bool _initialized = false;

/*
 * Initialize layers system.
 */
void
layers_init(void)
{
        if (_initialized) {
                return;
        }

        _initialized = true;
}
