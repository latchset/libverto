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
#include <string.h>
#include <errno.h>

#include <verto-tevent.h>
#include <verto-module.h>

#define tctx(p) ((tevent_ev_ctx *) p)->ctx
#define texit(p) ((tevent_ev_ctx *) p)->exit

static struct tevent_context *defctx;

typedef struct {
    struct tevent_context *ctx;
    char exit;
} tevent_ev_ctx;

static void
tevent_ctx_free(void *priv)
{
    if (priv != defctx)
        talloc_free(priv);
}

static void
tevent_ctx_run(void *priv)
{
    while (!texit(priv))
        tevent_loop_once(tctx(priv));
    texit(priv) = 0;
}

static void
tevent_ctx_run_once(void *priv)
{
    tevent_loop_once(priv);
}

static void
tevent_ctx_break(void *priv)
{
    texit(priv) = 1;
}

#define definecb(type, ...) \
    static void tevent_ ## type ## _cb(struct tevent_context *c, \
                                       struct tevent_ ## type *e, \
                                       __VA_ARGS__, void *data) { \
        verto_fire(data); \
    }

definecb(fd, uint16_t fl)
definecb(timer, struct timeval ct)
definecb(signal, int signum, int count, void *siginfo)

static void *
tevent_ctx_add(void *ctx, const verto_ev *ev, verto_ev_flag *flags)

{
    time_t interval;
    struct timeval tv;
    uint16_t teventflags = 0;

    *flags |= VERTO_EV_FLAG_PERSIST;
    switch (verto_get_type(ev)) {
    case VERTO_EV_TYPE_IO:
        if (verto_get_flags(ev) & VERTO_EV_FLAG_IO_READ)
            teventflags |= TEVENT_FD_READ;
        if (verto_get_flags(ev) & VERTO_EV_FLAG_IO_WRITE)
            teventflags |= TEVENT_FD_WRITE;
        return tevent_add_fd(tctx(ctx), tctx(ctx), verto_get_fd(ev),
                             teventflags, tevent_fd_cb, (void *) ev);
    case VERTO_EV_TYPE_TIMEOUT:
        *flags &= ~VERTO_EV_FLAG_PERSIST; /* Timeout events don't persist */
        interval = verto_get_interval(ev);
        tv = tevent_timeval_current_ofs(interval / 1000, interval % 1000 * 1000);
        return tevent_add_timer(tctx(ctx), tctx(ctx), tv,
                                tevent_timer_cb, (void *) ev);
    case VERTO_EV_TYPE_SIGNAL:
        return tevent_add_signal(tctx(ctx), tctx(ctx), verto_get_signal(ev),
                                 0, tevent_signal_cb, (void *) ev);
    case VERTO_EV_TYPE_IDLE:
    case VERTO_EV_TYPE_CHILD:
    default:
        return NULL; /* Not supported */
    }
}

static void
tevent_ctx_del(void *priv, const verto_ev *ev, void *evpriv)
{
    talloc_free(evpriv);
}

VERTO_MODULE(tevent, g_main_context_default);

verto_ev_ctx *
verto_new_tevent()
{
    return verto_convert_tevent(tevent_context_init(NULL));
}

verto_ev_ctx *
verto_default_tevent()
{
    if (!defctx)
        defctx = tevent_context_init(NULL);
    return verto_convert_tevent(defctx);
}

verto_ev_ctx *
verto_convert_tevent(struct tevent_context *context)
{
    tevent_ev_ctx *ctx;

    ctx = talloc_zero(NULL, tevent_ev_ctx);
    if (ctx) {
        talloc_set_name_const(ctx, "libverto");
        ctx->ctx = context;
        ctx->exit = 0;
        if (ctx->ctx != defctx)
            (void) talloc_steal(ctx, ctx->ctx);
    }

    return verto_convert(tevent, ctx);
}
