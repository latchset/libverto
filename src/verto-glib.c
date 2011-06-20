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

/* DEFAULT, LOW, MEDIUM, HIGH */
static gint priority_map[4] = {
        G_PRIORITY_DEFAULT,
        G_PRIORITY_LOW,
        G_PRIORITY_DEFAULT_IDLE,
        G_PRIORITY_HIGH };

struct glibEvCtx {
    GMainContext *context;
    GMainLoop *loop;
};

struct glibEv {
    GSource *src;
    guint tag;
    GIOChannel *chan;
};

static void *
glib_convert_(GMainContext *mc, GMainLoop *ml)
{
    struct glibEvCtx *l = NULL;

    l = g_new0(struct glibEvCtx, 1);
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

static void *
glib_ctx_new()
{
    GMainContext *mc = g_main_context_new();
    return glib_convert_(mc, NULL);
}

static void *
glib_ctx_default()
{
    return glib_convert_(g_main_context_default(), NULL);
}

static void
glib_ctx_free(void *lp)
{
    g_main_loop_unref(((struct glibEvCtx*) lp)->loop);
    g_main_context_unref(((struct glibEvCtx*) lp)->context);
    g_free(lp);
}

static void
glib_ctx_run(void *lp)
{
    g_main_loop_run(((struct glibEvCtx*) lp)->loop);
}

static void
glib_ctx_run_once(void *lp)
{
    g_main_context_iteration(((struct glibEvCtx*) lp)->context, TRUE);
}

static void
glib_ctx_break(void *lp)
{
    g_main_loop_quit(((struct glibEvCtx*) lp)->loop);
}

static gboolean
glib_callback(gpointer data)
{
    verto_call(data);
    return FALSE;
}

static void
glib_callback_child(GPid pid, gint status, gpointer data)
{
    verto_set_pid_status(data, status);
    verto_call(data);
}

static int
glib_ctx_add(void *ctx, struct vertoEv *ev)
{
    struct glibEv *gev = NULL;

    gev = g_new0(struct glibEv, 1);
    if (!gev)
        return ENOMEM;

    switch (verto_get_type(ev)) {
        case VERTO_EV_TYPE_READ:
        case VERTO_EV_TYPE_WRITE:
            gev->chan = g_io_channel_unix_new(verto_get_fd(ev));
            if (!gev->chan)
                goto error;
            g_io_channel_set_close_on_unref(gev->chan, FALSE);

            gev->src = g_io_create_watch(gev->chan,
                    verto_get_type(ev) == VERTO_EV_TYPE_READ
                        ? G_IO_IN
                        : G_IO_OUT);
            break;
        case VERTO_EV_TYPE_TIMEOUT:
            gev->src = g_timeout_source_new(verto_get_interval(ev));
            break;
        case VERTO_EV_TYPE_IDLE:
            gev->src = g_idle_source_new();
            break;
        case VERTO_EV_TYPE_CHILD:
            gev->src = g_child_watch_source_new(verto_get_pid(ev));
            break;
        case VERTO_EV_TYPE_SIGNAL:
#if GLIB_MAJOR_VERSION >= 2
#if GLIB_MINOR_VERSION >= 29
            gev->src = g_unix_signal_source_new(verto_get_signal(ev));
            break;
#endif /* GLIB_MINOR_VERSION >= 29 */
#endif /* GLIB_MAJOR_VERSION >= 2 */
        default:
            return -1; /* Not supported */
    }


    if (!gev->src)
        goto error;

    g_source_set_can_recurse(gev->src, FALSE);
    g_source_set_priority(gev->src, priority_map[verto_get_priority(ev)]);
    g_source_set_callback(gev->src, verto_get_type(ev) == VERTO_EV_TYPE_CHILD
                                    ? (GSourceFunc) glib_callback_child
                                    : glib_callback, ev, NULL);
    gev->tag = g_source_attach(gev->src, ((struct glibEvCtx*) ctx)->context);
    if (gev->tag == 0)
        goto error;

    verto_set_module_private(ev, gev);
    return 0;
    error:
        if (gev) {
            if (gev->chan)
                g_io_channel_unref(gev->chan);
            g_free(gev);
        }
        return ENOMEM;
}

static void
glib_ctx_del(void *lp, struct vertoEv *ev)
{
    if (!ev)
        return;

    struct glibEv *gev = verto_get_module_private(ev);
    if (gev) {
        if (gev->tag > 0)
            g_source_remove(gev->tag);
        if (gev->src)
            g_source_unref(gev->src);
        if (gev->chan)
            g_io_channel_unref(gev->chan);
    }

    g_free(ev);
}

VERTO_MODULE(glib, g_main_context_default);

struct vertoEvCtx *
verto_convert_glib(GMainContext *mc, GMainLoop *ml)
{
    return verto_convert_funcs(&glib_funcs, glib_convert_(mc, ml));
}
