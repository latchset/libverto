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

#include <sys/time.h>

#include "test.h"

#define M2U(m) ((m) * 1000)

static struct timeval starttime;
static struct timeval endtime;

void
exit_cb(struct vertoEvCtx *ctx, struct vertoEv *ev)
{
    verto_break(ctx);
}

void
cb(struct vertoEvCtx *ctx, struct vertoEv *ev)
{
    long long diff;

    if (starttime.tv_sec == endtime.tv_sec && starttime.tv_usec == endtime.tv_usec) {
        gettimeofday(&endtime, NULL);

        diff = (endtime.tv_sec - starttime.tv_sec) * M2U(1000)
                + endtime.tv_usec - starttime.tv_usec;

        retval = diff < M2U(50) || diff > M2U(100);
        if (retval != 0)
            printf("ERROR: Timeout is out-of-bounds!\n");
        verto_add_timeout(ctx, VERTO_EV_PRIORITY_DEFAULT, exit_cb, NULL, 100);
        return;
    }

    printf("ERROR: Timeout must not repeat!\n");
    retval = 1;
    exit_cb(ctx, ev);
}

int
do_test(struct vertoEvCtx *ctx)
{
    gettimeofday(&starttime, NULL);
    endtime = starttime;

    assert(verto_add_timeout(ctx, VERTO_EV_PRIORITY_DEFAULT, cb, NULL, 50));
    return 0;
}
