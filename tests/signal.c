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

#include <verto-module.h>
#include "test.h"

static int count;

void
exit_cb(vertoEvCtx *ctx, vertoEv *ev)
{
    if ((pid_t) (uintptr_t) verto_get_private(ev) != 0)
        waitpid((pid_t) (uintptr_t) verto_get_private(ev), NULL, 0);

    switch (count) {
        case 0:
            printf("ERROR: Signal callback never fired!\n");
            break;
        case 1:
            printf("ERROR: Signal MUST recur!\n");
            break;
        default:
            break;
    }
    retval = count != 2;
    verto_break(ctx);
}

void
cb(vertoEvCtx *ctx, vertoEv *ev)
{
    count++;
}

int
do_test(vertoEvCtx *ctx)
{
    pid_t pid = 0;

    count = 0;
    if (!verto_add_signal(ctx, VERTO_EV_FLAG_PERSIST, cb, NULL, SIGUSR1)) {
        printf("WARNING: Signal not supported!\n");
        count = 2;
    } else {
        verto_add_signal(ctx, VERTO_EV_FLAG_NONE, VERTO_SIG_IGN, NULL, SIGUSR2);

        pid = fork();
        if (pid < 0)
            return 1;
        else if (pid == 0) {
            usleep(10000); /* 0.01 seconds */
            kill(getppid(), SIGUSR1);
            usleep(10000); /* 0.01 seconds */
            kill(getppid(), SIGUSR1);
            usleep(10000); /* 0.01 seconds */
            kill(getppid(), SIGUSR2);
            exit(0);
        }
    }

    verto_add_timeout(ctx, VERTO_EV_FLAG_NONE, exit_cb, (void *) (uintptr_t) pid, 100);

    return 0;
}
