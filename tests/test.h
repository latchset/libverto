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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <string.h>

#include <verto.h>

static char *MODULES[] = {
#ifdef HAVE_GLIB
    "glib",
#endif
#ifdef HAVE_LIBEV
    "libev",
#endif
#ifdef HAVE_LIBEVENT
    "libevent",
#endif
#ifdef HAVE_TEVENT
    "tevent",
#endif
    NULL,
    NULL,
};

int do_test(verto_ctx *ctx);

static int retval = 0;

int
main(int argc, char **argv)
{
    int i;
    verto_ctx *ctx;

    if (argc == 2) {
        MODULES[0] = argv[1];
        MODULES[1] = NULL;
    }

    for (i=0 ; MODULES[i] ; i++) {
        printf("Module: %s\n", MODULES[i]);

        assert((ctx = verto_default(MODULES[i], VERTO_EV_TYPE_NONE)));

        retval = do_test(ctx);
        if (retval != 0) {
            verto_free(ctx);
            return retval;
        }

        verto_run(ctx);
        verto_free(ctx);

        if (retval != 0)
            break;
    }

    return retval;
}

void *passert(void *p) {
    assert(p);
    return p;
}

#define new0(type) _new0(sizeof(type))
void * _new0(ssize_t size) {
    void *p = malloc(size);
    return passert(p ? memset(p, 0, size) : p);
}
