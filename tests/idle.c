/*
 * Copyright 2011 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "test.h"

static int occurrences;

void
exit_cb(struct vertoEvCtx *ctx, struct vertoEv *ev)
{
    if (occurrences == 0)
        printf("WARNING: Idle not supported!\n");
    else if (occurrences > 1) {
        printf("ERROR: Idle must not recur!\n");
        retval = 1;
    }

    verto_break(ctx);
}

void
cb(struct vertoEvCtx *ctx, struct vertoEv *ev)
{
    if (++occurrences > 1)
        exit_cb(ctx, ev);

    verto_add_timeout(ctx, VERTO_EV_PRIORITY_DEFAULT, exit_cb, NULL, 100);
}

int
do_test(struct vertoEvCtx *ctx)
{
    occurrences = 0;

    if (!verto_add_idle(ctx, VERTO_EV_PRIORITY_DEFAULT, cb, NULL))
        verto_add_timeout(ctx, VERTO_EV_PRIORITY_DEFAULT, exit_cb, NULL, 1);

    return 0;
}
