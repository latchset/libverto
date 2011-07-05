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

    verto_fire(w->data);
}

#define setuptype(type, priv, ...) \
    type ## w = malloc(sizeof(ev_ ## type)); \
    if (!type ## w) \
        return NULL; \
    ev_ ## type ## _init(type ## w, (EV_CB(type, (*))) __VA_ARGS__); \
    type ## w->data = (void *) priv; \
    ev_ ## type ## _start(ctx, type ## w); \
    return type ## w

static void *
libev_ctx_add(void *ctx, const verto_ev *ev, bool *persists)
{
    ev_io *iow = NULL;
    ev_timer *timerw = NULL;
    ev_idle *idlew = NULL;
    ev_signal *signalw = NULL;
    ev_child *childw = NULL;
    ev_tstamp interval;
    int events = EV_NONE;

    *persists = true;
    switch (verto_get_type(ev)) {
        case VERTO_EV_TYPE_IO:
            if (verto_get_flags(ev) & VERTO_EV_FLAG_IO_READ)
                events |= EV_READ;
            if (verto_get_flags(ev) & VERTO_EV_FLAG_IO_WRITE)
                events |= EV_WRITE;
            setuptype(io, ev, libev_callback, verto_get_fd(ev), events);
        case VERTO_EV_TYPE_TIMEOUT:
            interval = ((ev_tstamp) verto_get_interval(ev)) / 1000.0;
            setuptype(timer, ev, libev_callback, interval, interval);
        case VERTO_EV_TYPE_IDLE:
            setuptype(idle, ev, libev_callback);
        case VERTO_EV_TYPE_SIGNAL:
            setuptype(signal, ev, libev_callback, verto_get_signal(ev));
        case VERTO_EV_TYPE_CHILD:
            *persists = false;
            setuptype(child, ev, libev_callback, verto_get_pid(ev), 0);
        default:
            return NULL; /* Not supported */
    }
}

static void
libev_ctx_del(void *ctx, const verto_ev *ev, void *evpriv)
{
    switch (verto_get_type(ev)) {
        case VERTO_EV_TYPE_IO:
            ev_io_stop(ctx, evpriv);
        case VERTO_EV_TYPE_TIMEOUT:
            ev_timer_stop(ctx, evpriv);
        case VERTO_EV_TYPE_IDLE:
            ev_idle_stop(ctx, evpriv);
        case VERTO_EV_TYPE_SIGNAL:
            ev_signal_stop(ctx, evpriv);
        case VERTO_EV_TYPE_CHILD:
            ev_child_stop(ctx, evpriv);
        default:
            break;
    }

    free(evpriv);
}

VERTO_MODULE(libev, ev_loop_new);

verto_ev_ctx *
verto_new_libev()
{
    return verto_convert_libev(ev_loop_new(EVFLAG_AUTO));
}

verto_ev_ctx *
verto_default_libev()
{
    return verto_convert_libev(ev_default_loop(EVFLAG_AUTO));
}

verto_ev_ctx *
verto_convert_libev(struct ev_loop* loop)
{
    return verto_convert(libev, loop);
}
