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

/* The glib timer will subtract a perterbation of up to 10ms based on your
 * DBUS session or HOSTNAME (see g_timeout_set_expiration). */
#define SLEEP 10
#define SLEEP_MIN (SLEEP-10)
#define SLEEP_MAX (SLEEP*4)
#define M2U(m) ((m) * 1000)

static int callcount;
struct timeval starttime;

static char
elapsed(time_t min, time_t max)
{
    struct timeval tv;
    long long diff;

    assert(gettimeofday(&tv, NULL) == 0);
    diff = (tv.tv_sec - starttime.tv_sec) * M2U(1000)
            + tv.tv_usec - starttime.tv_usec;

    assert(gettimeofday(&starttime, NULL) == 0);
    if (diff < M2U(min) || diff > M2U(max)) {
        printf("ERROR: Timeout is out-of-bounds!\n");
        return 0;
    }
    return 1;
}

static void
exit_cb(verto_ctx *ctx, verto_ev *ev)
{
    assert(callcount == 3);
    verto_break(ctx);
}

static void
cb(verto_ctx *ctx, verto_ev *ev)
{
    assert(elapsed(SLEEP_MIN, SLEEP_MAX));
    if (++callcount == 3)
        assert(verto_add_timeout(ctx, VERTO_EV_FLAG_NONE, exit_cb, SLEEP*2));
    else if (callcount == 2) {
        assert(verto_add_timeout(ctx, VERTO_EV_FLAG_NONE, cb, SLEEP));
        verto_del(ev);
    }
}

int
do_test(verto_ctx *ctx)
{
    callcount = 0;

    assert(gettimeofday(&starttime, NULL) == 0);
    assert(verto_add_timeout(ctx, VERTO_EV_FLAG_PERSIST, cb, SLEEP));
    return 0;
}
