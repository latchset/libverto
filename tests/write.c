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

static pid_t pid;
static int count;

static void
error_cb(struct vertoEvCtx *ctx, struct vertoEv *ev)
{
    printf("ERROR: Timeout!\n");
    retval = 1;
    verto_break(ctx);
}

static void
cb(struct vertoEvCtx *ctx, struct vertoEv *ev)
{
    int fd = verto_get_fd(ev);

    count++;
    if (write(fd, DATA, DATALEN) != DATALEN) {
        if (count != 2) {
            printf("ERROR: %d: %s\n", errno, strerror(errno));
            retval = 1;
        }

        goto out;
    }

    if (count > 1) {
        printf("ERROR: Second call successful!?\n");
        retval = 1;
        goto out;
    }

    usleep(200000); /* 0.2 seconds; make time for child to close() */
    verto_repeat(ev);
    return;

    out:
        waitpid(pid, NULL, 0);
        close(fd);
        verto_break(ctx);
        return;
}

int
do_test(struct vertoEvCtx *ctx)
{
    unsigned char buff[DATALEN];
    int fds[2] = {-1, -1};
    pid = 0;
    count = 0;

    /* Ignore when the pipe breaks */
    verto_add_signal(ctx, VERTO_EV_PRIORITY_DEFAULT, VERTO_SIG_IGN, NULL, SIGPIPE);

    if (pipe(fds) != 0)
        return 1;

    if (!verto_add_write(ctx, VERTO_EV_PRIORITY_DEFAULT, cb, NULL, fds[1])) {
        printf("ERROR: Unable to add write call!\n");
        close(fds[0]);
        close(fds[1]);
        return 1;
    }

    if (!verto_add_timeout(ctx, VERTO_EV_PRIORITY_DEFAULT, error_cb, NULL, 1000)) {
        printf("ERROR: Unable to add timeout!\n");
        close(fds[0]);
        close(fds[1]);
        return 1;
    }

    pid = fork();
    if (pid < 0)
        return 1;
    else if (pid == 0) {
        close(fds[1]);
        usleep(100000); /* 0.1 seconds */
        if (read(fds[0], buff, DATALEN) != DATALEN) {
            close(fds[0]);
            exit(1);
        }
        close(fds[0]);
        exit(0);
    }
    close(fds[0]);

    return 0;
}
