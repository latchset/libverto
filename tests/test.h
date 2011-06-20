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

#include <verto.h>

static const char *MODULES[] = {
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
    NULL
};

int do_test(struct vertoEvCtx *ctx);

static int retval = 0;

int
main(int argc, char **argv)
{
    int i;
    struct vertoEvCtx *ctx;

    for (i=0 ; MODULES[i] ; i++) {
        printf("Module: %s\n", MODULES[i]);

        ctx = verto_new(MODULES[i]);
        if (!ctx)
            return 1;

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