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
#define VERTO_MODULE_TYPES
typedef struct event_base verto_mod_ctx;
typedef struct event verto_mod_ev;
#include <verto-module.h>

#include <event2/event_compat.h>

/* This is technically not exposed in any headers, but it is exported from
 * the binary. Without it, we can't provide compatibility with libevent's
 * sense of "global." */
extern struct event_base *event_global_current_base_;

static verto_mod_ctx *
libevent_ctx_new(void)
{
    struct event_base *eb;

    eb = event_base_new();
    event_base_priority_init(eb, 3);
    return eb;
}

static verto_mod_ctx *
libevent_ctx_default(void)
{
    if (!event_global_current_base_)
        event_global_current_base_ = event_init();

    event_base_priority_init(event_global_current_base_, 3);
    return event_global_current_base_;
}

static void
libevent_ctx_free(verto_mod_ctx *ctx)
{
    event_base_free(ctx);
}

static void
libevent_ctx_run(verto_mod_ctx *ctx)
{
    event_base_dispatch(ctx);
}

static void
libevent_ctx_run_once(verto_mod_ctx *ctx)
{
    event_base_loop(ctx, EVLOOP_ONCE);
}

static void
libevent_ctx_break(verto_mod_ctx *ctx)
{
    event_base_loopbreak(ctx);
}

static void
libevent_ctx_reinitialize(verto_mod_ctx *ctx)
{
    event_reinit(ctx);
}

static void
libevent_callback(evutil_socket_t socket, short type, void *data)
{
    verto_ev_flag state = VERTO_EV_FLAG_NONE;

    if (type & EV_READ)
        state |= VERTO_EV_FLAG_IO_READ;
    if (type & EV_WRITE)
        state |= VERTO_EV_FLAG_IO_WRITE;
#ifdef EV_ERROR
    if (type & EV_ERROR)
        state |= VERTO_EV_FLAG_IO_ERROR;
#endif

    verto_set_fd_state(data, state);
    verto_fire(data);
}

static verto_mod_ev *
libevent_ctx_add(verto_mod_ctx *ctx, const verto_ev *ev, verto_ev_flag *flags)
{
    struct event *priv = NULL;
    struct timeval *timeout = NULL;
    struct timeval tv;
    int libeventflags = 0;

    *flags |= verto_get_flags(ev) & VERTO_EV_FLAG_PERSIST;
    if (verto_get_flags(ev) & VERTO_EV_FLAG_PERSIST)
        libeventflags |= EV_PERSIST;

    switch (verto_get_type(ev)) {
    case VERTO_EV_TYPE_IO:
        if (verto_get_flags(ev) & VERTO_EV_FLAG_IO_READ)
            libeventflags |= EV_READ;
        if (verto_get_flags(ev) & VERTO_EV_FLAG_IO_WRITE)
            libeventflags |= EV_WRITE;
        priv = event_new(ctx, verto_get_fd(ev), libeventflags,
                         libevent_callback, (void *) ev);
        break;
    case VERTO_EV_TYPE_TIMEOUT:
        timeout = &tv;
        tv.tv_sec = verto_get_interval(ev) / 1000;
        tv.tv_usec = verto_get_interval(ev) % 1000 * 1000;
        priv = event_new(ctx, -1, EV_TIMEOUT | libeventflags,
                         libevent_callback, (void *) ev);
        break;
    case VERTO_EV_TYPE_SIGNAL:
        priv = event_new(ctx, verto_get_signal(ev),
                         EV_SIGNAL | libeventflags,
                         libevent_callback, (void *) ev);
        break;
    case VERTO_EV_TYPE_IDLE:
    case VERTO_EV_TYPE_CHILD:
    default:
        return NULL; /* Not supported */
    }

    if (!priv)
        return NULL;

    if (verto_get_flags(ev) & VERTO_EV_FLAG_PRIORITY_HIGH)
        event_priority_set(priv, 0);
    else if (verto_get_flags(ev) & VERTO_EV_FLAG_PRIORITY_MEDIUM)
        event_priority_set(priv, 1);
    else if (verto_get_flags(ev) & VERTO_EV_FLAG_PRIORITY_LOW)
        event_priority_set(priv, 2);

    event_add(priv, timeout);
    return priv;
}

static void
libevent_ctx_del(verto_mod_ctx *ctx, const verto_ev *ev, verto_mod_ev *evpriv)
{
    event_del(evpriv);
    event_free(evpriv);
}

#define libevent_ctx_set_flags NULL
VERTO_MODULE(libevent, event_base_init,
             VERTO_EV_TYPE_IO |
             VERTO_EV_TYPE_TIMEOUT |
             VERTO_EV_TYPE_SIGNAL);

verto_ctx *
verto_convert_libevent(struct event_base* base)
{
    event_base_priority_init(base, 3);
    return verto_convert(libevent, 0, base);
}
