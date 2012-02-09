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

#include <errno.h>

#include <verto-glib.h>
#define VERTO_MODULE_TYPES
typedef struct {
    GMainContext *context;
    GMainLoop *loop;
} verto_mod_ctx;
typedef struct {
    GSource *src;
    GIOChannel *chan;
} verto_mod_ev;
#include <verto-module.h>

/* While glib has signal support in >=2.29, it does not support many
   common signals (like USR*). Therefore, signal support is disabled
   until they support them (should be soonish) */
#if GLIB_MAJOR_VERSION >= 999
#if GLIB_MINOR_VERSION >= 29
#ifdef G_OS_UNIX /* Not supported on Windows */
#include <glib-unix.h>
#define HAS_SIGNAL VERTO_EV_TYPE_SIGNAL
#endif
#endif /* GLIB_MINOR_VERSION >= 29 */
#endif /* GLIB_MAJOR_VERSION >= 2 */
#ifndef HAS_SIGNAL
#define HAS_SIGNAL 0
#endif

#define VERTO_GLIB_SUPPORTED_TYPES (VERTO_EV_TYPE_IO \
                                    | VERTO_EV_TYPE_TIMEOUT \
                                    | VERTO_EV_TYPE_IDLE \
                                    | HAS_SIGNAL \
                                    | VERTO_EV_TYPE_CHILD)

static void *
glib_convert_(GMainContext *mc, GMainLoop *ml)
{
    verto_mod_ctx *l = NULL;

    l = g_new0(verto_mod_ctx, 1);
    if (l) {
        if (mc) {
            /* Steal references */
            l->context = mc;
            l->loop = ml ? ml : g_main_loop_new(l->context, FALSE);

            if (g_main_context_default() == mc)
                g_main_context_ref(mc);
        } else {
            l->context = g_main_context_ref(g_main_context_default());
            l->loop = g_main_loop_new(l->context, FALSE);
        }
    } else {
        g_main_loop_unref(ml);
        g_main_context_unref(mc);
    }

    return l;
}

static verto_mod_ctx *
glib_ctx_new(void) {
    return glib_convert_(g_main_context_new(), NULL);
}

static verto_mod_ctx *
glib_ctx_default(void) {
    return glib_convert_(g_main_context_default(), NULL);
}

static void
glib_ctx_free(verto_mod_ctx *ctx)
{
    g_main_loop_unref(ctx->loop);
    g_main_context_unref(ctx->context);
    g_free(ctx);
}

static void
glib_ctx_run(verto_mod_ctx *ctx)
{
    g_main_loop_run(ctx->loop);
}

static void
glib_ctx_run_once(verto_mod_ctx *ctx)
{
    g_main_context_iteration(ctx->context, TRUE);
}

static gboolean
break_callback(gpointer loop)
{
    g_main_loop_quit(loop);
    return FALSE;
}

static void
glib_ctx_break(verto_mod_ctx *ctx)
{
    GSource *src = g_timeout_source_new(0);
    g_assert(src);
    g_source_set_callback(src, break_callback, ctx->loop, NULL);
    g_source_set_priority(src, G_PRIORITY_HIGH);
    g_assert(g_source_attach(src, ctx->context) != 0);
    g_source_unref(src);
}

static gboolean
glib_callback(gpointer data)
{
    gboolean persists = verto_get_flags(data) & VERTO_EV_FLAG_PERSIST;
    verto_fire(data);
    return persists;
}

static gboolean
glib_callback_io(GIOChannel *source, GIOCondition condition, gpointer data)
{
    verto_ev_flag state = VERTO_EV_FLAG_NONE;

    if (condition & (G_IO_IN | G_IO_PRI))
        state |= VERTO_EV_FLAG_IO_READ;
    if (condition & G_IO_OUT)
        state |= VERTO_EV_FLAG_IO_WRITE;
    if (condition & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
        state |= VERTO_EV_FLAG_IO_ERROR;

    verto_set_fd_state(data, state);
    return glib_callback(data);
}

static void
glib_callback_child(GPid pid, gint status, gpointer data)
{
    verto_set_proc_status(data, status);
    verto_fire(data);
}

static verto_mod_ev *
glib_ctx_add(verto_mod_ctx *ctx, const verto_ev *ev, verto_ev_flag *flags)
{
    verto_mod_ev *gev = NULL;
    GIOCondition cond = 0;
    verto_ev_type type = verto_get_type(ev);

    *flags |= verto_get_flags(ev) & VERTO_EV_FLAG_PERSIST;
    *flags |= verto_get_flags(ev) & VERTO_EV_FLAG_IO_CLOSE_FD;

    gev = g_new0(verto_mod_ev, 1);
    if (!gev)
        return NULL;

    switch (type) {
        case VERTO_EV_TYPE_IO:
#ifdef WIN32
            gev->chan = g_io_channel_win32_new_socket(verto_get_fd(ev));
#else
            gev->chan = g_io_channel_unix_new(verto_get_fd(ev));
#endif
            if (!gev->chan)
                goto error;
            g_io_channel_set_close_on_unref(gev->chan,
                                            *flags & VERTO_EV_FLAG_IO_CLOSE_FD);

            if (verto_get_flags(ev) & VERTO_EV_FLAG_IO_READ)
                cond |= G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
            if (verto_get_flags(ev) & VERTO_EV_FLAG_IO_WRITE)
                cond |= G_IO_OUT | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
            gev->src = g_io_create_watch(gev->chan, cond);
            break;
        case VERTO_EV_TYPE_TIMEOUT:
            gev->src = g_timeout_source_new(verto_get_interval(ev));
            break;
        case VERTO_EV_TYPE_IDLE:
            gev->src = g_idle_source_new();
            break;
        case VERTO_EV_TYPE_CHILD:
            gev->src = g_child_watch_source_new(verto_get_proc(ev));
            break;
        case VERTO_EV_TYPE_SIGNAL:
/* While glib has signal support in >=2.29, it does not support many
   common signals (like USR*). Therefore, signal support is disabled
   until they support them (should be soonish) */
#if GLIB_MAJOR_VERSION >= 999
#if GLIB_MINOR_VERSION >= 29
#ifdef G_OS_UNIX /* Not supported on Windows */
            gev->src = g_unix_signal_source_new(verto_get_signal(ev));
            break;
#endif
#endif /* GLIB_MINOR_VERSION >= 29 */
#endif /* GLIB_MAJOR_VERSION >= 2 */
        default:
            return NULL; /* Not supported */
    }

    if (!gev->src)
        goto error;

    if (type == VERTO_EV_TYPE_IO)
        g_source_set_callback(gev->src, (GSourceFunc) glib_callback_io, (void *) ev, NULL);
    else if (type == VERTO_EV_TYPE_CHILD)
        g_source_set_callback(gev->src, (GSourceFunc) glib_callback_child, (void *) ev, NULL);
    else
        g_source_set_callback(gev->src, glib_callback, (void *) ev, NULL);

    if (verto_get_flags(ev) & VERTO_EV_FLAG_PRIORITY_HIGH)
        g_source_set_priority(gev->src, G_PRIORITY_HIGH);
    else if (verto_get_flags(ev) & VERTO_EV_FLAG_PRIORITY_MEDIUM)
        g_source_set_priority(gev->src, G_PRIORITY_DEFAULT_IDLE);
    else if (verto_get_flags(ev) & VERTO_EV_FLAG_PRIORITY_LOW)
        g_source_set_priority(gev->src, G_PRIORITY_LOW);

    g_source_set_can_recurse(gev->src, FALSE);
    if (g_source_attach(gev->src, ctx->context) == 0)
        goto error;

    return gev;

    error:
        if (gev) {
            if (gev->chan)
                g_io_channel_unref(gev->chan);
            if (gev->src) {
                g_source_destroy(gev->src);
                g_source_unref(gev->src);
            }
            g_free(gev);
        }
        return NULL;
}

static void
glib_ctx_del(verto_mod_ctx *ctx, const verto_ev *ev, verto_mod_ev *evpriv)
{
    if (!ev)
        return;

    if (evpriv->chan)
        g_io_channel_unref(evpriv->chan);
    if (evpriv->src) {
        g_source_destroy(evpriv->src);
        g_source_unref(evpriv->src);
    }

    g_free(evpriv);
}

#define glib_ctx_reinitialize NULL
#define glib_ctx_set_flags NULL
VERTO_MODULE(glib, g_main_context_default, VERTO_GLIB_SUPPORTED_TYPES);

verto_ctx *
verto_convert_glib(GMainContext *mc, GMainLoop *ml)
{
    return verto_convert(glib, 0, glib_convert_(mc, ml));
}
