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

#define VERTO_MODULE_TYPES
typedef struct tevent_context verto_mod_ctx;
typedef void verto_mod_ev;
#include <verto-module.h>

#ifndef TEVENT_FD_ERROR
#define TEVENT_FD_ERROR 0
#endif /* TEVENT_FD_ERROR */

static verto_mod_ctx *
tevent_ctx_new(void)
{
    return tevent_context_init(NULL);
}

static void
tevent_ctx_free(verto_mod_ctx *ctx)
{
    talloc_free(ctx);
}

static void
tevent_ctx_run_once(verto_mod_ctx *ctx)
{
    tevent_loop_once(ctx);
}

static void
tevent_ctx_reinitialize(verto_mod_ctx *ctx)
{
    tevent_re_initialise(ctx);
}

static void
tevent_fd_cb(struct tevent_context *c, struct tevent_fd *e,
             uint16_t fl, void *data)
{
    verto_ev_flag state = VERTO_EV_FLAG_NONE;

    if (fl & TEVENT_FD_READ)
        state |= VERTO_EV_FLAG_IO_READ;
    if (fl & TEVENT_FD_WRITE)
        state |= VERTO_EV_FLAG_IO_WRITE;
    if (fl & TEVENT_FD_ERROR)
        state |= VERTO_EV_FLAG_IO_ERROR;

    verto_set_fd_state(data, state);
    verto_fire(data);
}

static void
tevent_timer_cb(struct tevent_context *c, struct tevent_timer *e,
                struct timeval ct, void *data)
{
    verto_fire(data);
}

static void
tevent_signal_cb(struct tevent_context *c, struct tevent_signal *e,
                 int signum, int count, void *siginfo, void *data)
{
    verto_fire(data);
}

static void
tevent_ctx_set_flags(verto_mod_ctx *ctx, const verto_ev *ev,
                     verto_mod_ev *evpriv)
{
    if (verto_get_type(ev) == VERTO_EV_TYPE_IO) {
        uint16_t teventflags = TEVENT_FD_ERROR;
        if (verto_get_flags(ev) & VERTO_EV_FLAG_IO_READ)
            teventflags |= TEVENT_FD_READ;
        if (verto_get_flags(ev) & VERTO_EV_FLAG_IO_WRITE)
            teventflags |= TEVENT_FD_WRITE;
        tevent_fd_set_flags(evpriv, teventflags);
    }
}

static verto_mod_ev *
tevent_ctx_add(verto_mod_ctx *ctx, const verto_ev *ev, verto_ev_flag *flags)

{
    time_t interval;
    struct timeval tv;
    struct tevent_fd *tfde;

    *flags |= VERTO_EV_FLAG_PERSIST;
    switch (verto_get_type(ev)) {
    case VERTO_EV_TYPE_IO:
        tfde = tevent_add_fd(ctx, ctx, verto_get_fd(ev), TEVENT_FD_ERROR,
                             tevent_fd_cb, (void *) ev);
        if (tfde) {
            tevent_ctx_set_flags(ctx, ev, tfde);
            if (verto_get_flags(ev) & VERTO_EV_FLAG_IO_CLOSE_FD) {
                *flags |= VERTO_EV_FLAG_IO_CLOSE_FD;
                tevent_fd_set_auto_close(tfde);
            }
        }
        return tfde;
    case VERTO_EV_TYPE_TIMEOUT:
        *flags &= ~VERTO_EV_FLAG_PERSIST; /* Timeout events don't persist */
        interval = verto_get_interval(ev);
        tv = tevent_timeval_current_ofs(interval / 1000, interval % 1000 * 1000);
        return tevent_add_timer(ctx, ctx, tv,
                                tevent_timer_cb, (void *) ev);
    case VERTO_EV_TYPE_SIGNAL:
        return tevent_add_signal(ctx, ctx, verto_get_signal(ev),
                                 0, tevent_signal_cb, (void *) ev);
    case VERTO_EV_TYPE_IDLE:
    case VERTO_EV_TYPE_CHILD:
    default:
        return NULL; /* Not supported */
    }
}

static void
tevent_ctx_del(verto_mod_ctx *priv, const verto_ev *ev, verto_mod_ev *evpriv)
{
    talloc_free(evpriv);
}

#define tevent_ctx_break NULL
#define tevent_ctx_run NULL
#define tevent_ctx_default NULL
VERTO_MODULE(tevent, g_main_context_default,
             VERTO_EV_TYPE_IO |
             VERTO_EV_TYPE_TIMEOUT |
             VERTO_EV_TYPE_SIGNAL);

verto_ctx *
verto_convert_tevent(struct tevent_context *context)
{
    return verto_convert(tevent, 0, context);
}
