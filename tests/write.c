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

#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include <verto-module.h>
#include "test.h"

#define DATA "hello"
#define DATALEN 5

static int fds[2];

static void
timeout_cb(struct vertoEvCtx *ctx, struct vertoEv *ev)
{
    printf("ERROR: Timeout!\n");
    if (fds[0] >= 0)
        close(fds[0]);
    if (fds[1] >= 0)
        close(fds[1]);

    retval = 1;
    verto_break(ctx);
}

static void
error_cb(struct vertoEvCtx *ctx, struct vertoEv *ev)
{
    int fd = verto_get_fd(ev);
    assert(fd == fds[1]);

    /* When we get here, the fd should be closed, so an error should occur */
    assert(write(fd, DATA, DATALEN) != DATALEN);
    verto_break(ctx);
    close(fd);
    fds[1] = -1;
}

static void
read_cb(struct vertoEvCtx *ctx, struct vertoEv *ev)
{
    unsigned char buff[DATALEN];
    int fd = verto_get_fd(ev);
    assert(fd == fds[0]);

    assert(read(fd, buff, DATALEN) == DATALEN);
    assert(verto_add_write(ctx, VERTO_EV_PRIORITY_HIGH, error_cb, NULL, fds[1]));
    close(fd);
    fds[0] = -1;
}

static void
write_cb(struct vertoEvCtx *ctx, struct vertoEv *ev)
{
    int fd = verto_get_fd(ev);
    assert(fd == fds[1]);

    assert(write(fd, DATA, DATALEN) == DATALEN);
}

int
do_test(struct vertoEvCtx *ctx)
{
    fds[0] = -1;
    fds[1] = -1;

    if (!verto_add_signal(ctx, VERTO_EV_PRIORITY_DEFAULT, VERTO_SIG_IGN, NULL, SIGPIPE))
        signal(SIGPIPE, SIG_IGN);

    assert(pipe(fds) == 0);
    assert(verto_add_timeout(ctx, VERTO_EV_PRIORITY_LOW, timeout_cb, NULL, 1000));
    assert(verto_add_write(ctx, VERTO_EV_PRIORITY_HIGH, write_cb, NULL, fds[1]));
    assert(verto_add_read(ctx, VERTO_EV_PRIORITY_LOW, read_cb, NULL, fds[0]));
    return 0;
}
