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

#include <verto-libev.h>
#include <verto-module.h>

static void *
libev_ctx_new()
{
    return ev_loop_new(EVFLAG_AUTO);
}

static void *
libev_ctx_default()
{
    return ev_default_loop(EVFLAG_AUTO);
}

static void
libev_ctx_free(void *ctx)
{
    if (ctx != EV_DEFAULT)
        ev_loop_destroy(ctx);
}

static void
libev_ctx_run(void *ctx)
{
    ev_run(ctx, 0);
}

static void
libev_ctx_run_once(void *ctx)
{
    ev_run(ctx, EVRUN_ONCE);
}

static void
libev_ctx_break(void *ctx)
{
    ev_break(ctx, EVBREAK_ONE);
}

static void
libev_callback(EV_P_ ev_watcher *w, int revents)
{
    if (verto_get_type(w->data) == VERTO_EV_TYPE_CHILD)
        verto_set_pid_status(w->data, ((ev_child*) w)->rstatus);

    verto_call(w->data);

    switch (verto_get_type(w->data)) {
        case VERTO_EV_TYPE_READ:
        case VERTO_EV_TYPE_WRITE:
            ev_io_stop(loop, (ev_io*) w);
        case VERTO_EV_TYPE_TIMEOUT:
            ev_timer_stop(loop, (ev_timer*) w);
        case VERTO_EV_TYPE_IDLE:
            ev_idle_stop(loop, (ev_idle*) w);
        case VERTO_EV_TYPE_CHILD:
            ev_child_stop(loop, (ev_child*) w);
        case VERTO_EV_TYPE_SIGNAL: /* Signal events are persistant */
        default:
            break;
    }
}

#define setuptype(type, priv, ...) \
    type ## w = malloc(sizeof(ev_ ## type)); \
    if (!type ## w) \
        return ENOMEM; \
    ev_ ## type ## _init(type ## w, (EV_CB(type, (*))) __VA_ARGS__); \
    type ## w->data = priv; \
    ev_ ## type ## _start(ctx, type ## w); \
    verto_set_module_private(ev, type ## w)

static int
libev_ctx_add(void *ctx, struct vertoEv *ev)
{
    ev_io *iow = NULL;
    ev_timer *timerw = NULL;
    ev_idle *idlew = NULL;
    ev_signal *signalw = NULL;
    ev_child *childw = NULL;
    ev_tstamp interval;

    switch (verto_get_type(ev)) {
        case VERTO_EV_TYPE_READ:
            setuptype(io, ev, libev_callback, verto_get_fd(ev), EV_READ);
            break;
        case VERTO_EV_TYPE_WRITE:
            setuptype(io, ev, libev_callback, verto_get_fd(ev), EV_WRITE);
            break;
        case VERTO_EV_TYPE_TIMEOUT:
            interval = ((ev_tstamp) verto_get_interval(ev)) / 1000.0;
            setuptype(timer, ev, libev_callback, interval, interval);
            break;
        case VERTO_EV_TYPE_IDLE:
            setuptype(idle, ev, libev_callback);
            break;
        case VERTO_EV_TYPE_SIGNAL:
            setuptype(signal, ev, libev_callback, verto_get_signal(ev));
            break;
        case VERTO_EV_TYPE_CHILD:
            setuptype(child, ev, libev_callback, verto_get_pid(ev), 0);
            break;
        default:
            return -1; /* Not supported */
    }

    return 0;
}

static void
libev_ctx_del(void *ctx, struct vertoEv *ev)
{
    void *priv = verto_get_module_private(ev);
    if (!priv)
        return;

    switch (verto_get_type(ev)) {
        case VERTO_EV_TYPE_READ:
        case VERTO_EV_TYPE_WRITE:
            ev_io_stop(ctx, priv);
        case VERTO_EV_TYPE_TIMEOUT:
            ev_timer_stop(ctx, priv);
        case VERTO_EV_TYPE_IDLE:
            ev_idle_stop(ctx, priv);
        case VERTO_EV_TYPE_SIGNAL:
            ev_signal_stop(ctx, priv);
        case VERTO_EV_TYPE_CHILD:
            ev_child_stop(ctx, priv);
        default:
            break;
    }

    free(priv);
}

VERTO_MODULE(libev, ev_loop_new);

struct vertoEvCtx *verto_convert_libev(struct ev_loop* loop)
{
    return verto_convert_funcs(&libev_funcs, loop);
}
