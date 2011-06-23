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

#define tctx(p) ((struct teventEvCtx *) p)->ctx
#define texit(p) ((struct teventEvCtx *) p)->exit

static struct tevent_context *defctx;

struct teventEvCtx {
    struct tevent_context *ctx;
    bool exit;
};

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
    texit(priv) = false;
}

static void
tevent_ctx_run_once(void *priv)
{
    tevent_loop_once(priv);
}

static void
tevent_ctx_break(void *priv)
{
    texit(priv) = true;
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
tevent_ctx_add(void *ctx, const struct vertoEv *ev)
{
    void *priv = NULL;
    time_t interval;
    struct timeval tv;

    interval = verto_get_interval(ev);
    tv = tevent_timeval_current_ofs(interval / 1000, interval % 1000 * 1000);

    switch (verto_get_type(ev)) {
    case VERTO_EV_TYPE_READ:
        priv = tevent_add_fd(tctx(ctx), tctx(ctx), verto_get_fd(ev),
                             TEVENT_FD_READ, tevent_fd_cb, (void *) ev);
        break;
    case VERTO_EV_TYPE_WRITE:
        priv = tevent_add_fd(tctx(ctx), tctx(ctx), verto_get_fd(ev),
                             TEVENT_FD_WRITE, tevent_fd_cb, (void *) ev);
        break;
    case VERTO_EV_TYPE_TIMEOUT:
        priv = tevent_add_timer(tctx(ctx), tctx(ctx), tv,
                                tevent_timer_cb, (void *) ev);
        break;
    case VERTO_EV_TYPE_SIGNAL:
        priv = tevent_add_signal(tctx(ctx), tctx(ctx), verto_get_signal(ev),
                                 0, tevent_signal_cb, (void *) ev);
        break;
    case VERTO_EV_TYPE_IDLE:
    case VERTO_EV_TYPE_CHILD:
    default:
        return NULL; /* Not supported */
    }

    if (!priv)
        return NULL;

    return priv;
}

static void
tevent_ctx_del(void *priv, const struct vertoEv *ev, void *evpriv)
{
    talloc_free(evpriv);
}

VERTO_MODULE(tevent, g_main_context_default);

struct vertoEvCtx *
verto_new_tevent()
{
    return verto_convert_tevent(tevent_context_init(NULL));
}

struct vertoEvCtx *
verto_default_tevent()
{
    if (!defctx)
        defctx = tevent_context_init(NULL);
    return verto_convert_tevent(defctx);
}

struct vertoEvCtx *
verto_convert_tevent(struct tevent_context *context)
{
    struct teventEvCtx *ctx;

    ctx = talloc_zero(NULL, struct teventEvCtx);
    if (ctx) {
        talloc_set_name_const(ctx, "libverto");
        ctx->ctx = context;
        ctx->exit = false;
        if (ctx->ctx != defctx)
            talloc_steal(ctx, ctx->ctx);
    }

    return verto_convert(tevent, ctx);
}
