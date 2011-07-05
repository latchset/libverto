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
#include <verto-module.h>

typedef struct {
    GMainContext *context;
    GMainLoop *loop;
} glib_ev_ctx;

typedef struct {
    GSource *src;
    GIOChannel *chan;
} glib_ev;

static void *
glib_convert_(GMainContext *mc, GMainLoop *ml)
{
    glib_ev_ctx *l = NULL;

    l = g_new0(glib_ev_ctx, 1);
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

static void
glib_ctx_free(void *lp)
{
    g_main_loop_unref(((glib_ev_ctx*) lp)->loop);
    g_main_context_unref(((glib_ev_ctx*) lp)->context);
    g_free(lp);
}

static void
glib_ctx_run(void *lp)
{
    g_main_loop_run(((glib_ev_ctx*) lp)->loop);
}

static void
glib_ctx_run_once(void *lp)
{
    g_main_context_iteration(((glib_ev_ctx*) lp)->context, TRUE);
}

static void
glib_ctx_break(void *lp)
{
    g_main_loop_quit(((glib_ev_ctx*) lp)->loop);
}

static gboolean
glib_callback(gpointer data)
{
    gboolean persists = verto_get_flags(data) & VERTO_EV_FLAG_PERSIST;
    verto_fire(data);
    return persists;
}

gboolean
glib_callback_io(GIOChannel *source, GIOCondition condition, gpointer data)
{
    return glib_callback(data);
}

static void
glib_callback_child(GPid pid, gint status, gpointer data)
{
    verto_set_pid_status(data, status);
    verto_fire(data);
}

static void *
glib_ctx_add(void *ctx, const verto_ev *ev, bool *persists)
{
    glib_ev *gev = NULL;
    verto_ev_type type = verto_get_type(ev);
    verto_ev_flag flags = verto_get_flags(ev);

    *persists = flags & VERTO_EV_FLAG_PERSIST;

    gev = g_new0(glib_ev, 1);
    if (!gev)
        return NULL;

    switch (type) {
        case VERTO_EV_TYPE_IO:
            gev->chan = g_io_channel_unix_new(verto_get_fd(ev));
            if (!gev->chan)
                goto error;
            g_io_channel_set_close_on_unref(gev->chan, FALSE);

            GIOCondition cond = 0;
            if (flags & VERTO_EV_FLAG_IO_READ)
                cond |= G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
            if (flags & VERTO_EV_FLAG_IO_WRITE)
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
            gev->src = g_child_watch_source_new(verto_get_pid(ev));
            *persists = false;
            break;
        case VERTO_EV_TYPE_SIGNAL:
#if GLIB_MAJOR_VERSION >= 2
#if GLIB_MINOR_VERSION >= 29
            gev->src = g_unix_signal_source_new(verto_get_signal(ev));
            break;
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

    if (flags & VERTO_EV_FLAG_PRIORITY_HIGH)
        g_source_set_priority(gev->src, G_PRIORITY_HIGH);
    else if (flags & VERTO_EV_FLAG_PRIORITY_MEDIUM)
        g_source_set_priority(gev->src, G_PRIORITY_DEFAULT_IDLE);
    else if (flags & VERTO_EV_FLAG_PRIORITY_LOW)
        g_source_set_priority(gev->src, G_PRIORITY_LOW);

    g_source_set_can_recurse(gev->src, FALSE);
    if (g_source_attach(gev->src, ((glib_ev_ctx*) ctx)->context) == 0)
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
glib_ctx_del(void *lp, const verto_ev *ev, void *evpriv)
{
    if (!ev)
        return;

    if (((glib_ev *) evpriv)->chan)
        g_io_channel_unref(((glib_ev *) evpriv)->chan);
    if (((glib_ev *) evpriv)->src) {
        g_source_destroy(((glib_ev *) evpriv)->src);
        g_source_unref(((glib_ev *) evpriv)->src);
    }

    g_free(evpriv);
}

VERTO_MODULE(glib, g_main_context_default);

verto_ev_ctx *
verto_new_glib() {
    return verto_convert_glib(g_main_context_new(), NULL);
}

verto_ev_ctx *
verto_default_glib() {
    return verto_convert_glib(g_main_context_default(), NULL);
}

verto_ev_ctx *
verto_convert_glib(GMainContext *mc, GMainLoop *ml)
{
    return verto_convert(glib, glib_convert_(mc, ml));
}
