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

static int callcount;

void
exit_cb(verto_ctx *ctx, verto_ev *ev)
{
    retval = 1;
    switch (callcount) {
        case 0:
            printf("ERROR: Idle (persist) did not fire!\n");
            break;
        case 1:
            printf("ERROR: Idle (persist) did not recur!\n");
            break;
        case 2:
            printf("ERROR: Idle (non-persist) did not fire!\n");
            break;
        case 3:
            printf("ERROR: Idle on free did not fire!\n");
            break;
        case 4:
            retval = 0;
            break;
        default:
            break;
    }

    verto_break(ctx);
}

void
onfree(verto_ctx *ctx, verto_ev *ev) {
    ++callcount;
}

void
cb(verto_ctx *ctx, verto_ev *ev)
{
    if (++callcount == 2) {
        verto_ev *newev = verto_add_idle(ctx, VERTO_EV_FLAG_NONE, cb);
        verto_set_private(newev, NULL, onfree);
        assert(newev);
        verto_del(ev);
    }
}

int
do_test(verto_ctx *ctx)
{
    callcount = 0;

    if (!(verto_get_supported_types(ctx) & VERTO_EV_TYPE_IDLE)) {
        printf("WARNING: Idle not supported!\n");
        verto_break(ctx);
        return 0;
    }

    assert(verto_add_idle(ctx, VERTO_EV_FLAG_PERSIST, cb));
    assert(verto_add_timeout(ctx, VERTO_EV_FLAG_NONE, exit_cb, 1000));
    return 0;
}
