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

#include <verto-libev.h>
#include <verto-module.h>

struct libevEv {
    struct vertoEv ev;
    void *watcher;
};

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
libev_ctx_free(void *priv)
{
    if (priv != EV_DEFAULT)
        ev_loop_destroy(priv);
}

static void
libev_ctx_run(void *priv)
{
    ev_run(priv, 0);
}

static void
libev_ctx_run_once(void *priv)
{
    ev_run(priv, EVRUN_ONCE);
}

static void
libev_ctx_break(void *priv)
{
    ev_break(priv, EVBREAK_ONE);
}

#define setuptype(type, ...) \
    struct libevEv *ev = malloc(sizeof(struct libevEv)); \
    if (ev) { \
    	memset(ev, 0, sizeof(struct libevEv)); \
        ev->watcher = malloc(sizeof(ev_ ## type)); \
        if (!ev->watcher) goto error; \
        ev_ ## type ## _init((ev_ ## type *) ev->watcher, __VA_ARGS__); \
        ((ev_ ## type *) ev->watcher)->data = ev; \
        ev_ ## type ## _start(priv, ev->watcher); \
        return &ev->ev; \
    } \
    error: \
        free(ev); \
        return NULL

#define setupcb(type) \
    static void type ## _cb (EV_P_ ev_ ## type *w, int revents) { \
        struct libevEv *ev = ((struct libevEv*) w->data); \
        ev->ev.callback(&ev->ev); \
    }

setupcb(io)
setupcb(idle)
setupcb(timer)
setupcb(signal)
setupcb(child)

static struct vertoEv *
libev_ctx_add_read(void *priv, enum vertoEvPriority priority,
                  vertoCallback callback, void *data, int fd)
{
    setuptype(io, io_cb, fd, EV_READ);
}

static struct vertoEv *
libev_ctx_add_write(void *priv, enum vertoEvPriority priority,
                   vertoCallback callback, void *data, int fd)
{
    setuptype(io, io_cb, fd, EV_WRITE);
}

static struct vertoEv *
libev_ctx_add_timeout(void *priv, enum vertoEvPriority priority,
                     vertoCallback callback, void *data, time_t interval)
{
    setuptype(timer, timer_cb, ((ev_tstamp) interval) / 1000.0, ((ev_tstamp) interval) / 1000.0);
}

static struct vertoEv *
libev_ctx_add_idle(void *priv, enum vertoEvPriority priority,
                  vertoCallback callback, void *data)
{
    setuptype(idle, idle_cb);
}

static struct vertoEv *
libev_ctx_add_signal(void *priv, enum vertoEvPriority priority,
                    vertoCallback callback, void *data, int signal)
{
    setuptype(signal, signal_cb, signal);
}

static struct vertoEv *
libev_ctx_add_child(void *priv, enum vertoEvPriority priority,
                                    vertoCallback callback, void *data,
                                    pid_t pid)
{
    setuptype(child, child_cb, pid, 0);
}

static void
libev_ctx_del(void *priv, struct vertoEv *ev)
{
    if (!priv || !ev)
        return;

    switch (ev->type) {
    case VERTO_EV_TYPE_READ:
        ev_io_stop(priv, ((struct libevEv*) ev)->watcher);
    case VERTO_EV_TYPE_WRITE:
        ev_io_stop(priv, ((struct libevEv*) ev)->watcher);
    case VERTO_EV_TYPE_TIMEOUT:
        ev_timer_stop(priv, ((struct libevEv*) ev)->watcher);
    case VERTO_EV_TYPE_IDLE:
        ev_idle_stop(priv, ((struct libevEv*) ev)->watcher);
    case VERTO_EV_TYPE_SIGNAL:
        ev_signal_stop(priv, ((struct libevEv*) ev)->watcher);
    case VERTO_EV_TYPE_CHILD:
        ev_child_stop(priv, ((struct libevEv*) ev)->watcher);
    }

    free(ev);
}

VERTO_MODULE(libev, ev_loop_new);

struct vertoEvCtx *verto_convert_libev(struct ev_loop* loop)
{
    return verto_convert_funcs(&libev_funcs, loop);
}
