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

#include <verto-tevent.h>
#include <verto-module.h>

#define tctx(priv) ((struct teventEvCtx *) priv)->ctx
#define texit(priv) ((struct teventEvCtx *) priv)->exit

static struct teventEvCtx *defctx;

struct teventEvCtx {
    struct tevent_context *ctx;
    bool exit;
};

struct teventEv {
    struct vertoEv ev;
    union {
        struct tevent_fd *fd;
        struct tevent_signal *signal;
        struct tevent_timer *timer;
    } tev;
};

static void *
tevent_ctx_new()
{
    struct teventEvCtx *ctx;

    ctx = talloc_zero(NULL, struct teventEvCtx);
    if (ctx) {
        talloc_set_name_const(ctx, "libverto");
        ctx->ctx = tevent_context_init(ctx);
        ctx->exit = false;
    }
    return ctx;
}

static void *
tevent_ctx_default()
{
    if (!defctx)
        defctx = tevent_ctx_new();
    return defctx;
}

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
        tevent_loop_once(priv);
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

static void
fd_cb(struct tevent_context *c, struct tevent_fd *fd, uint16_t fl, void *data)
{
    struct teventEv *tev = (struct teventEv *) data;
    tev->ev.callback(&tev->ev);
}

static struct vertoEv *
tevent_ctx_add_read(void *priv, enum vertoEvPriority priority,
                  vertoCallback callback, void *data, int fd)
{
    struct teventEv *ev = talloc_zero(tctx(priv), struct teventEv);
    if (ev) {
        ev->tev.fd = tevent_add_fd(tctx(priv), tctx(priv), fd,
                                   TEVENT_FD_READ, fd_cb, ev);
        if (!ev->tev.fd) {
            talloc_free(ev);
            ev = NULL;
        }
    }
    return &ev->ev;
}

static struct vertoEv *
tevent_ctx_add_write(void *priv, enum vertoEvPriority priority,
                   vertoCallback callback, void *data, int fd)
{
    struct teventEv *ev = talloc_zero(tctx(priv), struct teventEv);
    if (ev) {
        ev->tev.fd = tevent_add_fd(tctx(priv), tctx(priv), fd,
                                   TEVENT_FD_WRITE, fd_cb, ev);
        if (!ev->tev.fd) {
            talloc_free(ev);
            ev = NULL;
        }
    }
    return &ev->ev;
}

static void
timer_cb(struct tevent_context *c, struct tevent_timer *te,
         struct timeval ct, void *data)
{
    struct timeval tv;
    struct teventEv *tev = (struct teventEv *) data;

    tev->ev.callback(&tev->ev);

    /* Make the event recur */
    tv.tv_sec = tev->ev.data.interval / 1000;
    tv.tv_usec = tev->ev.data.interval % 1000 * 1000;
    tev->tev.timer = tevent_add_timer(c, c, tv, timer_cb, tev);
}

static struct vertoEv *
tevent_ctx_add_timeout(void *priv, enum vertoEvPriority priority,
                     vertoCallback callback, void *data, time_t interval)
{
    struct timeval tv;

    struct teventEv *ev = talloc_zero(tctx(priv), struct teventEv);
    if (ev) {
        tv.tv_sec = interval / 1000;
        tv.tv_usec = interval % 1000 * 1000;
        ev->tev.timer = tevent_add_timer(tctx(priv), tctx(priv),
                                         tv, timer_cb, ev);
        if (!ev->tev.timer) {
            talloc_free(ev);
            ev = NULL;
        }
    }
    return &ev->ev;
}

static struct vertoEv *
tevent_ctx_add_idle(void *priv, enum vertoEvPriority priority,
                  vertoCallback callback, void *data)
{
    return NULL; /* Not currently supported */
}

static void
signal_cb(struct tevent_context *c, struct tevent_signal *se,
          int signum, int count, void *siginfo, void *data)
{
    struct teventEv *tev = (struct teventEv *) data;
    tev->ev.callback(&tev->ev);
}

static struct vertoEv *
tevent_ctx_add_signal(void *priv, enum vertoEvPriority priority,
                    vertoCallback callback, void *data, int signal)
{
    struct teventEv *ev = talloc_zero(tctx(priv), struct teventEv);
    if (ev) {
        ev->tev.signal = tevent_add_signal(tctx(priv), tctx(priv), signal,
                                           0, signal_cb, ev);
        if (!ev->tev.signal) {
            talloc_free(ev);
            ev = NULL;
        }
    }
    return &ev->ev;
}

static struct vertoEv *
tevent_ctx_add_child(void *priv, enum vertoEvPriority priority,
                                    vertoCallback callback, void *data,
                                    pid_t pid)
{
    return NULL; /* Not currently supported */
}

static void
tevent_ctx_del(void *priv, struct vertoEv *ev)
{
    talloc_free(ev);
}

VERTO_MODULE(tevent, g_main_context_default);

struct vertoEvCtx *
verto_convert_tevent(struct tevent_context *ctx)
{
    return verto_convert_funcs(&tevent_funcs, ctx);
}
