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
    printf("ERROR: Read event never fired!\n");
    retval = 1;
    verto_break(ctx);
}

static void
cb(struct vertoEvCtx *ctx, struct vertoEv *ev)
{
    unsigned char buff[DATALEN];
    int fd = verto_get_fd(ev);

    count++;
    if (read(fd, buff, DATALEN) != DATALEN) {
        /* If we got called because the fd was closed, OK.
         * Otherwise, an error occurred. */
        if (count != 2) {
            printf("ERROR: %s\n", strerror(errno));
            retval = 1;
        }

        goto out;
    }

    if (count > 1) {
        printf("ERROR: Second call successful!?\n");
        retval = 1;
        goto out;
    }

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
    int fds[2] = {-1, -1};
    pid = 0;
    count = 0;

    if (pipe(fds) != 0)
        return 1;

    if (!verto_add_read(ctx, VERTO_EV_PRIORITY_DEFAULT, cb, NULL, fds[0])) {
        printf("ERROR: Unable to add read call!\n");
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
        close(fds[0]);
        usleep(100000); /* 0.1 seconds */
        if (write(fds[1], DATA, DATALEN) != DATALEN)
            exit(1);
        usleep(100000); /* 0.1 seconds */
        close(fds[1]);
        exit(0);
    }
    close(fds[1]);

    return 0;
}
