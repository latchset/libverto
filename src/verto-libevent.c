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

#include <verto-libevent.h>
#include <verto-module.h>

static struct event_base *defctx;

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
    verto_call(data);
}

static void *
libevent_ctx_add(void *ctx, const struct vertoEv *ev)
{
    struct event *priv = NULL;
    struct timeval *timeout = NULL;
    struct timeval tv;

    switch (verto_get_type(ev)) {
    case VERTO_EV_TYPE_READ:
        priv = event_new(ctx, verto_get_fd(ev), EV_READ, libevent_callback, (void *) ev);
        break;
    case VERTO_EV_TYPE_WRITE:
        priv = event_new(ctx, verto_get_fd(ev), EV_WRITE, libevent_callback, (void *) ev);
        break;
    case VERTO_EV_TYPE_TIMEOUT:
        timeout = &tv;
        tv.tv_sec = verto_get_interval(ev) / 1000;
        tv.tv_usec = verto_get_interval(ev) % 1000 * 1000;
        priv = event_new(ctx, -1, EV_TIMEOUT, libevent_callback, (void *) ev);
        break;
    case VERTO_EV_TYPE_SIGNAL:
        priv = event_new(ctx, verto_get_signal(ev), EV_SIGNAL | EV_PERSIST,
                         libevent_callback, (void *) ev);
        break;
    case VERTO_EV_TYPE_IDLE:
    case VERTO_EV_TYPE_CHILD:
    default:
        return NULL; /* Not supported */
    }

    if (!priv)
        return NULL;

    event_add(priv, timeout);
    return priv;
}

static void
libevent_ctx_del(void *ctx, const struct vertoEv *ev, void *evpriv)
{
    event_del(evpriv);
    event_free(evpriv);
}

VERTO_MODULE(libevent, event_base_init);

struct vertoEvCtx *verto_convert_libevent(struct event_base* base)
{
    return verto_convert_funcs(&libevent_funcs, base);
}
