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

#include <verto-libevent.h>
#include <verto-module.h>

static struct event_base *defctx;

struct libeventEv {
    struct vertoEv ev;
    struct event *event;
};

static void *
libevent_ctx_new()
{
    return event_base_new();
}

static void *
libevent_ctx_default()
{
    if (!defctx)
        defctx = event_base_new();
    return defctx;
}

static void
libevent_ctx_free(void *priv)
{
    if (priv != defctx)
        event_base_free(priv);
}

static void
libevent_ctx_run(void *priv)
{
    event_base_dispatch(priv);
}

static void
libevent_ctx_run_once(void *priv)
{
    event_base_loop(priv, EVLOOP_ONCE);
}

static void
libevent_ctx_break(void *priv)
{
    event_base_loopbreak(priv);
}

static void
libevent_callback(evutil_socket_t socket, short type, void *data)
{
    struct libeventEv *ev = (struct libeventEv*) data;
    ev->ev.callback(&ev->ev);
}

#define add_ev(tv, evnew) \
    struct libeventEv *ev = malloc(sizeof(struct libeventEv)); \
    if (ev) { \
        memset(ev, 0, sizeof(struct libeventEv)); \
        ev->event = evnew; \
        if (!ev->event) { \
            free(ev); \
            ev = NULL; \
        } else { \
            event_add(ev->event, tv); \
        } \
    } \
    return &ev->ev

static struct vertoEv *
libevent_ctx_add_read(void *priv, enum vertoEvPriority priority,
                  vertoCallback callback, void *data, int fd)
{
    add_ev(NULL, event_new(priv, fd, EV_READ | EV_PERSIST, libevent_callback, ev));
}

static struct vertoEv *
libevent_ctx_add_write(void *priv, enum vertoEvPriority priority,
                   vertoCallback callback, void *data, int fd)
{
    add_ev(NULL, event_new(priv, fd, EV_WRITE | EV_PERSIST, libevent_callback, ev));
}

static struct vertoEv *
libevent_ctx_add_timeout(void *priv, enum vertoEvPriority priority,
                     vertoCallback callback, void *data, time_t interval)
{
    struct timeval tv;
    tv.tv_sec = interval / 1000;
    tv.tv_usec = interval % 1000 * 1000;
    add_ev(&tv, event_new(priv, -1, EV_TIMEOUT | EV_PERSIST, libevent_callback, ev));
}

static struct vertoEv *
libevent_ctx_add_idle(void *priv, enum vertoEvPriority priority,
                  vertoCallback callback, void *data)
{
    return NULL;
}

static struct vertoEv *
libevent_ctx_add_signal(void *priv, enum vertoEvPriority priority,
                    vertoCallback callback, void *data, int signal)
{
    add_ev(NULL, event_new(priv, -1, EV_SIGNAL | EV_PERSIST, libevent_callback, ev));
}

static struct vertoEv *
libevent_ctx_add_child(void *priv, enum vertoEvPriority priority,
                                    vertoCallback callback, void *data,
                                    pid_t pid)
{
    return NULL;
}

static void
libevent_ctx_del(void *priv, struct vertoEv *ev)
{
    event_del(((struct libeventEv*) ev)->event);
    event_free(((struct libeventEv*) ev)->event);
    free(ev);
}

VERTO_MODULE(libevent, event_base_init);

struct vertoEvCtx *verto_convert_libevent(struct event_base* base)
{
    return verto_convert_funcs(&libevent_funcs, base);
}
